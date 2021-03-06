/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "NIT.h"
#include "SectionSyncer.h"
#include "channels/Channel.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"

#include <assert.h>
#include <libsi/si_ext.h>
#include <libsi/descriptor.h>
#include <libsi/section.h>
#include <libsi/util.h>
#include <map>
#include <string>

using namespace SI;
using namespace SI_EXT;
using namespace std;

#define UNKNOWN_NETWORK_NAME  "" // Network name if NetworkNameDescriptorTag is absent (TODO)

// From UTF8Utils.h
#ifndef Utf8BufSize
#define Utf8BufSize(s)  ((s) * 4)
#endif

#define MAXNETWORKNAME  Utf8BufSize(256)

namespace VDR
{

// Left over from VDR
enum eSystemType
{
  DVB_SYSTEM_1 = 0, // SYS_DVBS or SYS_DVBT
  DVB_SYSTEM_2 = 1  // SYS_DVBS2 or SYS_DVBT2
};

static const cScanReceiver::filter_properties nit_pids[] =
{
  { PID_NIT, TableIdNIT,       0xFF },
  { PID_NIT, TableIdNIT_other, 0xFF },
};

cNit::cNit(cDevice* device)
 : cScanReceiver(device, "NIT", ARRAY_SIZE(nit_pids), nit_pids),
   m_networkId(NETWORK_ID_UNKNOWN)
 {
//  OpenResource(PID_NIT, TableIdNIT);
//  OpenResource(PID_NIT, TableIdNIT_other);
}

void cNit::ReceivePacket(uint16_t pid, const uint8_t* data)
{
  ChannelVector          transponders; // Return value
  cSectionSyncer         syncNit;
  map<uint16_t, Network> networks; // Network ID -> Network

  SI::NIT nit(data);
    if (nit.CheckAndParse())
    {
      // TODO: Handle TableIdNIT_other/
      SI::TableId tid = nit.getTableId();
      if (tid != TableIdNIT && tid != TableIdNIT_other)
        return;

      filter_properties filter = { pid, tid, 0xFF };
      if (!Sync(filter, nit.getVersionNumber(), nit.getSectionNumber(), nit.getLastSectionNumber()))
        return;

      // Add the network if we aren't already tracking it
      if (networks.find(nit.getNetworkId()) == networks.end())
      {
        string strName(UNKNOWN_NETWORK_NAME);

        SI::Descriptor* d;
        for (SI::Loop::Iterator it; (d = nit.commonDescriptors.getNext(it)); )
        {
          switch (d->getDescriptorTag())
          {
          case SI::NetworkNameDescriptorTag:
          {
            SI::NetworkNameDescriptor* nnd = (SI::NetworkNameDescriptor*)d;
            char buffer[MAXNETWORKNAME] = { };
            nnd->name.getText(buffer, sizeof(buffer));
            strName = buffer;
            break;
          }
          default:
            break;
          }
          delete d;
        }

        dsyslog("NIT[%u] %5d '%s'", networks.size(), nit.getNetworkId(), strName.c_str());
        networks[nit.getNetworkId()] = Network(nit.getNetworkId(), strName);
      }

      assert(networks.find(nit.getNetworkId()) != networks.end());

      Network& thisNetwork = networks[nit.getNetworkId()];

      // Some broadcasters send more than one NIT, with no apparent way of telling
      // which one is the right one to use. This is an attempt to find the NIT that
      // contains the transponder it was transmitted on and use only that one.
      if (m_networkId == NETWORK_ID_UNKNOWN && nit.getSectionNumber() == 0)
      {
        for (std::map<uint16_t, Network>::const_iterator it = networks.begin(); it != networks.end(); ++it)
        {
          if (it->second.bHasTransponder)
          {
            m_networkId = it->second.nid;
            dsyslog("Using NIT with network ID %u", m_networkId);
            //XXX what if more than one NIT contains this transponder???
            break;
          }
        }
      }

      if (m_networkId != NETWORK_ID_UNKNOWN && m_networkId != nit.getNetworkId())
        return; // Ignore all other NITs

      SI::NIT::TransportStream ts;
      for (SI::Loop::Iterator it; nit.transportStreamLoop.getNext(ts, it); )
      {
        SI::Descriptor* d;
        SI::Loop::Iterator it2;
        SI::FrequencyListDescriptor* fld = (SI::FrequencyListDescriptor*)ts.transportStreamDescriptors.getNext(it2, SI::FrequencyListDescriptorTag);
        vector<uint32_t> frequenciesKHz; // I have no fucking clue if this is actually KHz
        frequenciesKHz.resize(1); // frequencies[0] is determined later
        if (fld)
        {
          int iCodingType = fld->getCodingType();
          if (iCodingType > 0)
          {
            for (SI::Loop::Iterator it3; fld->frequencies.hasNext(it3);)
            {
              uint32_t iFrequencyKHz = fld->frequencies.getNext(it3);
              switch (iCodingType)
              {
              // Satellite
              case 1: iFrequencyKHz = BCD2INT(iFrequencyKHz) / 100;
                break;

              // Cable
              case 2: iFrequencyKHz = BCD2INT(iFrequencyKHz) / 10;
                break;

              // Terrestrial
              case 3: iFrequencyKHz *= 10;
                break;

              default:
                break;
              }
              frequenciesKHz.push_back(iFrequencyKHz);
            }
          }
        }
        DELETENULL(fld);

        for (SI::Loop::Iterator it2; (d = ts.transportStreamDescriptors.getNext(it2)); )
        {
          switch (d->getDescriptorTag())
          {
            case SI::SatelliteDeliverySystemDescriptorTag:
            {
              SI::SatelliteDeliverySystemDescriptor* sd = (SI::SatelliteDeliverySystemDescriptor*)d;

              cTransponder transponder(TRANSPONDER_SATELLITE);

              // Symbol rate
              transponder.SatelliteParams().SetSymbolRateHz(BCD2INT(sd->getSymbolRate()) * 100);

              // Polarization
              transponder.SatelliteParams().SetPolarization((fe_polarization)sd->getPolarization());

              // Rolloff
              //fe_rolloff_t rollOff = ROLLOFF_35; // Implied value in DVB-S, default for DVB-S2 (per frontend.h)
              static fe_rolloff_t rollOffs[] = { ROLLOFF_35, ROLLOFF_25, ROLLOFF_20, ROLLOFF_AUTO };
              transponder.SatelliteParams().SetRollOff(sd->getModulationSystem() == DVB_SYSTEM_2 ? rollOffs[sd->getRollOff()] : ROLLOFF_AUTO);

              // Code rate
              static fe_code_rate_t codeRates[] =
              {
                FEC_NONE, FEC_1_2,  FEC_2_3,  FEC_3_4,  FEC_5_6,  FEC_7_8,  FEC_8_9,
                FEC_3_5,  FEC_4_5,  FEC_9_10, FEC_AUTO, FEC_AUTO, FEC_AUTO,
                FEC_AUTO, FEC_AUTO, FEC_NONE
              };
              transponder.SatelliteParams().SetCoderateH(codeRates[sd->getFecInner()]);

              // Frequency
              transponder.SetFrequencyHz(frequenciesKHz[0] = BCD2INT(sd->getFrequency()) / 100);

              // Modulation
              //fe_modulation_t modulationType = QPSK;
              static fe_modulation_t modulations[] = { QAM_AUTO, QPSK, PSK_8, QAM_16 };
              transponder.SetModulation(modulations[sd->getModulationType()]);

              // System
              transponder.SetDeliverySystem(sd->getModulationSystem() ? SYS_DVBS2 : SYS_DVBS);

              for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
              {
                if (ISTRANSPONDER(*itKHz / 1000, m_device->Channel()->GetCurrentlyTunedTransponder().FrequencyMHz())) // TODO: ???
                {
                  thisNetwork.bHasTransponder = true;
                  break;
                }
              }

              // Source (TODO: Do something with position)
              //cChannelSource source(SOURCE_TYPE_SATELLITE, BCD2INT(sd->getOrbitalPosition()), sd->getWestEastFlag() ? DIRECTION_EAST : DIRECTION_WEST);

              for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
              {
                ChannelPtr channel = ChannelPtr(new cChannel);
                channel->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0);
                transponder.SetFrequencyKHz(*itKHz);
                channel->SetTransponder(transponder);
                transponders.push_back(channel);
              }

              /* TODO
              if (cSettings::Get().m_iUpdateChannels)
              {
                // Source
                int source = cSource::FromData(cSource::stSat, BCD2INT(sd->getOrbitalPosition()), sd->getWestEastFlag() ? cSource::sdEast : cSource::sdWest);

                bool bFound = false;
                bool bForceTransponderUpdate = false;

                ChannelVector& channels = m_callback->GetChannels();
                for (ChannelVector::iterator it = channels.begin(); it != channels.end(); ++it)
                {
                  ChannelPtr& channel = *it;
                  if (channel->Source() == source &&
                      channel->ID().Nid() == ts.getOriginalNetworkId() &&
                      channel->ID().Tsid() == ts.getTransportStreamId())
                  {
                    int transponder = channel->TransponderFrequencyMHz();
                    bFound = true;
                    if (!ISTRANSPONDER(cChannel::TransponderWTF(iFrequencyKHz, dtp.Polarization()), transponder))
                    {
                      for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
                      {
                        if (ISTRANSPONDER(cChannel::TransponderWTF(*itKHz, dtp.Polarization()), transponder))
                        {
                          iFrequencyKHz = *itKHz;
                          break;
                        }
                      }
                    }

                    // Only modify channels if we're actually receiving this transponder
                    assert(GetCurrentlyTunedTransponder().get() != NULL); // TODO
                    if (ISTRANSPONDER(cChannel::TransponderWTF(iFrequencyKHz, dtp.Polarization()), GetCurrentlyTunedTransponder()->TransponderFrequencyMHz()))
                      channel->SetTransponderData(source, iFrequencyKHz, symbolRate, dtp);
                    else
                      bForceTransponderUpdate = true; // Get us receiving this transponder
                  }
                }

                if (!bFound || bForceTransponderUpdate)
                {
                  for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
                  {
                    ChannelPtr transponder = ChannelPtr(new cChannel);
                    transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                    if (transponder->SetTransponderData(source, frequenciesKHz[0], symbolRate, dtp))
                      m_callback->NitFoundTransponder(transponder, m_tableId);
                  }
                }
              }
              */
              break;
            }
            case SI::S2SatelliteDeliverySystemDescriptorTag:
            {
              // only interesting if NBC-BS
              SI::S2SatelliteDeliverySystemDescriptor* sd = (SI::S2SatelliteDeliverySystemDescriptor*)d;
              int DVBS_backward_compatibility = sd->getBackwardsCompatibilityIndicator();
              if (DVBS_backward_compatibility)
              {
                // Okay: we should add a dvb-s transponder with same source, same
                // polarization and QPSK to list of transponders if this transponder
                // isn't already marked as known.
              }

              /* TODO
              if (cSettings::Get().m_iUpdateChannels >= 5)
              {
                ChannelVector& channels = m_callback->GetChannels();
                for (ChannelVector::iterator it = channels.begin(); it != channels.end(); ++it)
                {
                  ChannelPtr& channel = *it;
                  if (cSource::IsSat(channel->Source()) &&
                      channel->ID().Nid()    == ts.getOriginalNetworkId() &&
                      channel->ID().Tsid()    == ts.getTransportStreamId())
                  {
                    SI::S2SatelliteDeliverySystemDescriptor* sd = (SI::S2SatelliteDeliverySystemDescriptor*)d;
                    cDvbTransponder dtp(channel->Parameters());
                    dtp.SetSystem(DVB_SYSTEM_2);
                    dtp.SetStreamId(sd->getInputStreamIdentifier());
                    channel->SetTransponderData(channel->Source(), channel->FrequencyHz(), channel->SymbolRate(), dtp);
                    channel->NotifyObservers(ObservableMessageChannelChanged);
                    break;
                  }
                }
              }
              */
              break;
            }
            case SI::CableDeliverySystemDescriptorTag:
            {
              SI::CableDeliverySystemDescriptor* sd = (SI::CableDeliverySystemDescriptor*)d;

              cTransponder transponder(TRANSPONDER_CABLE);

              // Frequency
              transponder.SetFrequencyKHz(frequenciesKHz[0] = BCD2INT(sd->getFrequency()) / 10);

              //XXX FEC_outer???
              static fe_code_rate_t codeRates[] =
              {
                FEC_NONE, FEC_1_2,  FEC_2_3,  FEC_3_4,  FEC_5_6,  FEC_7_8,  FEC_8_9,
                FEC_3_5,  FEC_4_5,  FEC_9_10, FEC_AUTO, FEC_AUTO, FEC_AUTO,
                FEC_AUTO, FEC_AUTO, FEC_NONE
              };
              transponder.CableParams().SetCoderateH(codeRates[sd->getFecInner()]);

              // Modulation
              static fe_modulation_t modulations[] =
                { QPSK, QAM_16, QAM_32, QAM_64, QAM_128, QAM_256, QAM_AUTO };
              transponder.SetModulation(modulations[std::min(sd->getModulation(), 6)]);

              // Symbol rate
              transponder.CableParams().SetSymbolRateHz(BCD2INT(sd->getSymbolRate()) * 100);

              for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
              {
                if (*itKHz / 1000 == m_device->Channel()->GetCurrentlyTunedTransponder().FrequencyMHz()) // TODO
                {
                  thisNetwork.bHasTransponder = true;
                  break;
                }
              }

              for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
              {
                ChannelPtr channel = ChannelPtr(new cChannel);
                channel->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0);
                transponder.SetFrequencyKHz(*itKHz);
                channel->SetTransponder(transponder);
                transponders.push_back(channel);
              }

              /* TODO
              if (cSettings::Get().m_iUpdateChannels >= 5)
              {
                bool bFound = false;
                bool bForceTransponderUpdate = false;

                ChannelVector channels = m_callback->GetChannels();
                for (ChannelVector::iterator it = channels.begin(); it != channels.end(); ++it)
                {
                  ChannelPtr& channel = *it;
                  if (channel->Source() == source &&
                      channel->ID().Nid() == ts.getOriginalNetworkId() &&
                      channel->ID().Tsid() == ts.getTransportStreamId())
                  {
                    int transponder = channel->TransponderFrequencyMHz();

                    bFound = true;

                    if (!ISTRANSPONDER(iFrequencyKHz / 1000, transponder))
                    {
                      for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
                      {
                        if (ISTRANSPONDER(*itKHz / 1000, transponder))
                        {
                          iFrequencyKHz = *itKHz;
                          break;
                        }
                      }
                    }

                    // Only modify channels if we're actually receiving this transponder
                    assert(GetCurrentlyTunedTransponder().get() != NULL); // TODO
                    if (ISTRANSPONDER(iFrequencyKHz / 1000, GetCurrentlyTunedTransponder()->TransponderFrequencyMHz()))
                      channel->SetTransponderData(source, iFrequencyKHz, symbolRate, dtp);
                    else if (channel->SymbolRate() != symbolRate || channel->Parameters() != dtp)
                      bForceTransponderUpdate = true; // get us receiving this transponder
                  }
                }

                if (!bFound || bForceTransponderUpdate)
                {
                  for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
                  {
                    ChannelPtr transponder = ChannelPtr(new cChannel);
                    transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                    if (transponder->SetTransponderData(source, frequenciesKHz[0], symbolRate, dtp))
                      m_callback->NitFoundTransponder(transponder, m_tableId);
                  }
                }
              }
              */

              break;
            }
            case SI::TerrestrialDeliverySystemDescriptorTag:
            {
              SI::TerrestrialDeliverySystemDescriptor* sd = (SI::TerrestrialDeliverySystemDescriptor*)d;

              cTransponder transponder(TRANSPONDER_TERRESTRIAL);

              transponder.SetFrequencyKHz(frequenciesKHz[0] = sd->getFrequency() * 10);

              // Bandwidth
              static fe_bandwidth_t bandwidths[] = { BANDWIDTH_8_MHZ, BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ, BANDWIDTH_5_MHZ };
              if (sd->getBandwidth() < ARRAY_SIZE(bandwidths))
                transponder.TerrestrialParams().SetBandwidth(bandwidths[sd->getBandwidth()]);
              else
                transponder.TerrestrialParams().SetBandwidth(BANDWIDTH_8_MHZ); // Note: Default value was 0 Hz ??? Changed to 8 MHz instead

              // Modulation
              static fe_modulation_t constellations[] = { QPSK, QAM_16, QAM_64, QAM_AUTO };
              transponder.SetModulation(constellations[sd->getConstellation()]);

              // System
              transponder.SetDeliverySystem(SYS_DVBT);

              // Hierarchy
              static fe_hierarchy_t hierarchies[] =
              {
                HIERARCHY_NONE, HIERARCHY_1,    HIERARCHY_2,    HIERARCHY_4,
                HIERARCHY_AUTO, HIERARCHY_AUTO, HIERARCHY_AUTO, HIERARCHY_AUTO
              };
              transponder.TerrestrialParams().SetHierarchy(hierarchies[sd->getHierarchy()]);

              // Code rates
              static fe_code_rate_t codeRates[] =
              {
                  FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6, FEC_7_8, FEC_AUTO, FEC_AUTO,
                  FEC_AUTO
              };
              transponder.TerrestrialParams().SetCoderateH(codeRates[sd->getCodeRateHP()]);
              transponder.TerrestrialParams().SetCoderateL(codeRates[sd->getCodeRateLP()]);

              // Guard interval
              static fe_guard_interval_t guardIntervals[] =
              {
                  GUARD_INTERVAL_1_32, GUARD_INTERVAL_1_16, GUARD_INTERVAL_1_8,
                  GUARD_INTERVAL_1_4
              };
              transponder.TerrestrialParams().SetGuard(guardIntervals[sd->getGuardInterval()]);

              // Transmission mode
              static fe_transmit_mode_t transmissionModes[] =
              {
                  TRANSMISSION_MODE_2K, TRANSMISSION_MODE_8K, TRANSMISSION_MODE_4K,
                  TRANSMISSION_MODE_AUTO
              };
              transponder.TerrestrialParams().SetTransmission(transmissionModes[sd->getTransmissionMode()]);

              for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
              {
                if (*itKHz / (1000 * 1000) == m_device->Channel()->GetCurrentlyTunedTransponder().FrequencyMHz()) // TODO
                {
                  thisNetwork.bHasTransponder = true;
                  break;
                }
              }

              for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
              {
                ChannelPtr channel = ChannelPtr(new cChannel);
                channel->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0);
                transponder.SetFrequencyKHz(*itKHz);
                channel->SetTransponder(transponder);
                transponders.push_back(channel);
              }

              /* TODO
              if (cSettings::Get().m_iUpdateChannels >= 5)
              {
                bool bFound = false;
                bool bForceTransponderUpdate = false;
                ChannelVector& channels = m_callback->GetChannels();
                for (ChannelVector::iterator it = channels.begin(); it != channels.end(); ++it)
                {
                  ChannelPtr& channel = *it;
                  if (channel->Source() == source &&
                      channel->ID().Nid() == ts.getOriginalNetworkId() &&
                      channel->ID().Tsid() == ts.getTransportStreamId())
                  {
                    int transponder = channel->TransponderFrequencyMHz();
                    bFound = true;
                    if (!ISTRANSPONDER(iFrequencyKHz / (1000 * 1000), transponder))
                    {
                      for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
                      {
                        assert(GetCurrentlyTunedTransponder().get() != NULL); // TODO
                        if (ISTRANSPONDER(*itKHz / (1000 * 1000), GetCurrentlyTunedTransponder()->TransponderFrequencyMHz()))
                        {
                          iFrequencyKHz = *itKHz;
                          break;
                        }
                      }
                    }

                    // Only modify channels if we're actually receiving this transponder
                    assert(GetCurrentlyTunedTransponder().get() != NULL); // TODO
                    if (ISTRANSPONDER(iFrequencyKHz / (1000 * 1000), GetCurrentlyTunedTransponder()->TransponderFrequencyMHz()))
                      channel->SetTransponderData(source, iFrequencyKHz, 0, dtp);
                    else if (channel->Parameters() != dtp)
                      bForceTransponderUpdate = true; // get us receiving this transponder
                  }
                }

                if (!bFound || bForceTransponderUpdate)
                {
                  for (vector<uint32_t>::const_iterator itKHz = frequenciesKHz.begin(); itKHz != frequenciesKHz.end(); ++itKHz)
                  {
                    ChannelPtr transponder = ChannelPtr(new cChannel);
                    transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                    if (transponder->SetTransponderData(source, *itKHz, 0, dtp))
                      m_callback->NitFoundTransponder(transponder, m_tableId);
                  }
                }
              }
              */

              break;
            }
            case SI::ExtensionDescriptorTag:
            {
              SI::ExtensionDescriptor* sd = (SI::ExtensionDescriptor*)d;
              switch (sd->getExtensionDescriptorTag())
              {
              case SI::T2DeliverySystemDescriptorTag:
              {
                /* TODO
                if (cSettings::Get().m_iUpdateChannels >= 5)
                {
                  ChannelVector& channels = m_callback->GetChannels();
                  for (ChannelVector::iterator it = channels.begin(); it != channels.end(); ++it)
                  {
                    ChannelPtr& channel = *it;

                    int source = cSource::FromData(cSource::stTerr);

                    if (channel->Source() == source &&
                        channel->ID().Nid() == ts.getOriginalNetworkId() &&
                        channel->ID().Tsid() == ts.getTransportStreamId())
                    {
                      SI::T2DeliverySystemDescriptor* td = (SI::T2DeliverySystemDescriptor*)d;

                      int iFrequencyHz = channel->FrequencyHz();
                      int symbolRate = channel->SymbolRate();

                      //int SystemId = td->getSystemId();

                      cDvbTransponder dtp(channel->Parameters());

                      dtp.SetSystem(DVB_SYSTEM_2);
                      dtp.SetStreamId(td->getPlpId());

                      if (td->getExtendedDataFlag())
                      {
                        // Bandwidth
                        static fe_bandwidth_t T2Bandwidths[] =
                        {
                          BANDWIDTH_8_MHZ, BANDWIDTH_7_MHZ,  BANDWIDTH_6_MHZ,
                          BANDWIDTH_5_MHZ, BANDWIDTH_10_MHZ, BANDWIDTH_1_712_MHZ
                        };
                        if (td->getBandwidth() < ARRAY_SIZE(T2Bandwidths))
                          dtp.SetBandwidth(T2Bandwidths[td->getBandwidth()]);
                        else
                          dtp.SetBandwidth(BANDWIDTH_8_MHZ); // Note: Default value was 0 Hz ??? Changed to 8 MHz instead

                        // Guard interval
                        static fe_guard_interval_t T2GuardIntervals[] =
                        {
                            GUARD_INTERVAL_1_32,  GUARD_INTERVAL_1_16,
                            GUARD_INTERVAL_1_8,   GUARD_INTERVAL_1_4,
                            GUARD_INTERVAL_1_128, GUARD_INTERVAL_19_128,
                            GUARD_INTERVAL_19_256
                        };
                        if (td->getGuardInterval() < ARRAY_SIZE(T2GuardIntervals))
                          dtp.SetGuard(T2GuardIntervals[td->getGuardInterval()]);
                        else
                          dtp.SetGuard(GUARD_INTERVAL_AUTO); // Note: Default value was 0 ??? Changed to GUARD_INTERVAL_AUTO instead

                        // Transmission mode
                        static fe_transmit_mode_t T2TransmissionModes[] =
                        {
                          TRANSMISSION_MODE_2K,   TRANSMISSION_MODE_8K,
                          TRANSMISSION_MODE_4K,   TRANSMISSION_MODE_1K,
                          TRANSMISSION_MODE_16K,  TRANSMISSION_MODE_32K,
                          TRANSMISSION_MODE_AUTO, TRANSMISSION_MODE_AUTO
                        };
                        dtp.SetTransmission(T2TransmissionModes[td->getTransmissionMode()]);
                        //TODO add parsing of frequencies
                      }
                      channel->SetTransponderData(source, iFrequencyHz, symbolRate, dtp);
                      channel->NotifyObservers(ObservableMessageChannelChanged);
                    }
                  }
                }
                */

                break;
              }
              default:
                break;
              }
              break;
            }
            case SI::ServiceListDescriptorTag:
            {
              SI::ServiceListDescriptor* sd = (SI::ServiceListDescriptor*)d;
              SI::ServiceListDescriptor::Service service;
              for (SI::Loop::Iterator it; sd->serviceLoop.getNext(service, it); )
              {
                if (service.getServiceType() == 0x04 ||            // NVOD reference service
                    service.getServiceType() == 0x18 ||            // advanced codec SD NVOD reference service
                    service.getServiceType() == 0x1B)              // advanced codec HD NVOD reference service
                {
                  continue;
                }
                if (service.getServiceType() == 0x03 ||            // Teletext service
                    service.getServiceType() == 0x04 ||            // NVOD reference service (see note 1)
                    service.getServiceType() == 0x05 ||            // NVOD time-shifted service (see note 1)
                    service.getServiceType() == 0x06 ||            // mosaic service
                    service.getServiceType() == 0x0C ||            // data broadcast service
                    service.getServiceType() == 0x0D ||            // reserved for Common Interface Usage (EN 50221 [39])
                    service.getServiceType() == 0x0E ||            // RCS Map (see EN 301 790 [7])
                    service.getServiceType() == 0x0F ||            // RCS FLS (see EN 301 790 [7])
                    service.getServiceType() == 0x10 ||            // DVB MHP service
                    service.getServiceType() == 0x17 ||            // advanced codec SD NVOD time-shifted service
                    service.getServiceType() == 0x18 ||            // advanced codec SD NVOD reference service
                    service.getServiceType() == 0x1A ||            // advanced codec HD NVOD time-shifted service
                    service.getServiceType() == 0x1B)              // advanced codec HD NVOD reference service
                {
                  // TODO: Do we need a callback for updating discovered channels?

                  /*
                  ChannelPtr channel = cChannelManager::GetByServiceID(m_newChannels, Source(), GetCurrentlyTunedTransponder()->TransponderFrequencyMHz(), service.getServiceId());
                  if (channel && !(channel->Rid() & INVALID_CHANNEL))
                  {
                    dsyslog("   NIT: invalid (service_type=0x%.2x)", service.getServiceType());
                    channel->SetId(channel->ID().Nid(), channel->ID().Tsid(), channel->ID().Sid(), INVALID_CHANNEL);
                  }
                  */
                }
              }
              break;
            }
            case SI::CellFrequencyLinkDescriptorTag: break;         // not implemented in libsi
            case SI::FrequencyListDescriptorTag: break;             // already handled
            case SI::PrivateDataSpecifierDescriptorTag: break;      // not usable
            case 0x80 ... 0xFE: break;                              // user defined
            default:
              dsyslog("NIT: unknown descriptor tag 0x%.2x", d->getDescriptorTag());
              break;
          }
          DELETENULL(d);
        }
      }
    }
  //TODO do something useful with transponders
}

}
