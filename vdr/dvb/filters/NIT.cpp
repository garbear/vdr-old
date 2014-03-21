/*
 * nit.c: NIT section filter
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: nit.c 2.10 2013/03/07 09:42:29 kls Exp $
 */

#include "NIT.h"
#include "channels/ChannelManager.h"
#include "dvb/EITScan.h"
#include "sources/linux/DVBSourceParams.h"
#include "utils/Tools.h"
#include "settings/Settings.h"

#include <algorithm>
#include <linux/dvb/frontend.h>
#include <libsi/descriptor.h>
#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

namespace VDR
{

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))
#endif

cNitFilter::cNitFilter(cDevice* device)
 : cFilter(device),
   m_networkId(0)
{
  Set(PID_NIT, TableIdNIT);
}

void cNitFilter::Enable(bool bEnabled)
{
  cFilter::Enable(bEnabled);
  m_networkId = 0;
  m_nits.clear();
  m_sectionSyncer.Reset();
}

void cNitFilter::ProcessData(u_short pid, u_char tid, const std::vector<uint8_t>& data)
{
  SI::NIT nit(data.data(), false);
  if (!nit.CheckCRCAndParse())
    return;

  // Some broadcasters send more than one NIT, with no apparent way of telling which
  // one is the right one to use. This is an attempt to find the NIT that contains
  // the transponder it was transmitted on and use only that one:
  bool bFound = false;
  vector<cNit>::iterator thisNit = m_nits.end();
  if (!m_networkId)
  {
    for (vector<cNit>::iterator it = m_nits.begin(); it != m_nits.end(); ++it)
    {
      if (it->networkId == nit.getNetworkId())
      {
        if (nit.getSectionNumber() == 0)
        {
          // all NITs have passed by
          //for (int j = 0; j < m_numNits; j++)
          for (vector<cNit>::const_iterator it2 = m_nits.begin(); it2 != m_nits.end(); ++it2)
          {
            if (it2->hasTransponder)
            {
              m_networkId = it2->networkId;
              //printf("taking NIT with network ID %d\n", m_networkId);
              //XXX what if more than one NIT contains this transponder???
              break;
            }
          }
          if (!m_networkId)
          {
            //printf("none of the NITs contains transponder %d\n", Transponder());
            return;
          }
        }
        else
        {
          bFound = true;
          thisNit = it;
          break;
        }
      }
    }

    if (!m_networkId && bFound)
    {
      if (nit.getSectionNumber() == 0)
      {
        cNit newNit = { };
        SI::Descriptor *d;
        for (SI::Loop::Iterator it; (d = nit.commonDescriptors.getNext(it));)
        {
          switch (d->getDescriptorTag())
          {
          case SI::NetworkNameDescriptorTag:
          {
            SI::NetworkNameDescriptor *nnd = (SI::NetworkNameDescriptor *) d;
            nnd->name.getText(newNit.name, MAXNETWORKNAME);
            break;
          }
          default:
            break;
          }
          delete d;
        }
        newNit.networkId = nit.getNetworkId();
        newNit.hasTransponder = false;
        dsyslog("NIT[%u] %5d '%s'\n", m_nits.size(), newNit.networkId, newNit.name);
        m_nits.push_back(newNit);
        thisNit = m_nits.end() - 1;
      }
    }
  }
  else if (m_networkId != nit.getNetworkId())
    return; // ignore all other NITs
  else if (!m_sectionSyncer.Sync(nit.getVersionNumber(), nit.getSectionNumber(), nit.getLastSectionNumber()))
    return;
//  XXX if (!Channels.Lock(true, 10))
//     return;

  SI::NIT::TransportStream ts;
  for (SI::Loop::Iterator it; nit.transportStreamLoop.getNext(ts, it);)
  {
    SI::Descriptor *d;

    SI::Loop::Iterator it2;
    SI::FrequencyListDescriptor *fld = (SI::FrequencyListDescriptor *) ts.transportStreamDescriptors.getNext(it2, SI::FrequencyListDescriptorTag);
    int NumFrequencies = fld ? fld->frequencies.getCount() + 1 : 1;
    int FrequenciesHz[NumFrequencies];
    if (fld)
    {
      int iCodingType = fld->getCodingType();
      if (iCodingType > 0)
      {
        int n = 1;
        for (SI::Loop::Iterator it3; fld->frequencies.hasNext(it3);)
        {
          int iFrequencyHz = fld->frequencies.getNext(it3);
          switch (iCodingType)
            {
          case 1:
            iFrequencyHz = BCD2INT(iFrequencyHz) / 100;
            break;
          case 2:
            iFrequencyHz = BCD2INT(iFrequencyHz) / 10;
            break;
          case 3:
            iFrequencyHz = iFrequencyHz * 10;
            break;
          default:
            ;
            }
          FrequenciesHz[n++] = iFrequencyHz;
        }
      }
      else
        NumFrequencies = 1;
    }
    delete fld;

    for (SI::Loop::Iterator it2; (d = ts.transportStreamDescriptors.getNext(it2));)
    {
      switch (d->getDescriptorTag())
      {
      case SI::SatelliteDeliverySystemDescriptorTag:
        {
          SI::SatelliteDeliverySystemDescriptor *sd = (SI::SatelliteDeliverySystemDescriptor *) d;
          cDvbTransponderParams dtp;
          int Source = cSource::FromData(cSource::stSat, BCD2INT(sd->getOrbitalPosition()) /*, sd->getWestEastFlag()  TODO*/);
          int iFrequencyMHz = FrequenciesHz[0] = BCD2INT(sd->getFrequency()) / 100000;
          dtp.SetPolarization((fe_polarization)sd->getPolarization());
          static fe_code_rate CodeRates[] =
            { FEC_NONE, FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6, FEC_7_8, FEC_8_9,
                FEC_3_5, FEC_4_5, FEC_9_10, FEC_AUTO, FEC_AUTO, FEC_AUTO,
                FEC_AUTO, FEC_AUTO, FEC_NONE };
          dtp.SetCoderateH(CodeRates[sd->getFecInner()]);
          static fe_modulation Modulations[] = { QAM_AUTO, QPSK, PSK_8, QAM_16 };
          dtp.SetModulation(Modulations[sd->getModulationType()]);
          dtp.SetSystem(sd->getModulationSystem() ? DVB_SYSTEM_2 : DVB_SYSTEM_1);
          static fe_rolloff RollOffs[] = { ROLLOFF_35, ROLLOFF_25, ROLLOFF_20, ROLLOFF_AUTO };
          dtp.SetRollOff( sd->getModulationSystem() ? RollOffs[sd->getRollOff()] : ROLLOFF_AUTO);
          int SymbolRate = BCD2INT(sd->getSymbolRate()) / 10;

          if (thisNit != m_nits.end())
          {
            for (int n = 0; n < NumFrequencies; n++)
            {
              if (ISTRANSPONDER(cChannel::Transponder(iFrequencyMHz, dtp.Polarization()), Transponder()))
              {
                thisNit->hasTransponder = true;
                //printf("has transponder %d\n", Transponder());
                break;
              }
            }
            break;
          }
          if (cSettings::Get().m_iUpdateChannels >= 5)
          {
            bool found = false;
            bool forceTransponderUpdate = false;
            std::vector<ChannelPtr> channels =
                cChannelManager::Get().GetCurrent();
            for (std::vector<ChannelPtr>::const_iterator it = channels.begin();
                it != channels.end(); ++it)
            {
              ChannelPtr Channel = (*it);
              if (!Channel->GroupSep() && Channel->Source() == Source
                  && Channel->Nid() == ts.getOriginalNetworkId()
                  && Channel->Tid() == ts.getTransportStreamId())
              {
                int transponder = Channel->Transponder();
                found = true;
                if (!ISTRANSPONDER(cChannel::Transponder(iFrequencyMHz, dtp.Polarization()), transponder))
                {
                  for (int n = 0; n < NumFrequencies; n++)
                  {
                    if (ISTRANSPONDER(cChannel::Transponder(FrequenciesHz[n], dtp.Polarization()), transponder))
                    {
                      iFrequencyMHz = FrequenciesHz[n] / 100000;
                      break;
                    }
                  }
                }
                if (ISTRANSPONDER(cChannel::Transponder(iFrequencyMHz, dtp.Polarization()), Transponder())) // only modify channels if we're actually receiving this transponder
                  Channel->SetTransponderData(Source, FrequenciesHz[0], SymbolRate, dtp);
                else if (Channel->Srate() != SymbolRate || Channel->Parameters() != dtp)
                  forceTransponderUpdate = true; // get us receiving this transponder
                Channel->NotifyObservers(ObservableMessageChannelChanged);
              }
            }
            if (!found || forceTransponderUpdate)
            {
              for (int n = 0; n < NumFrequencies; n++)
              {
                ChannelPtr Channel = ChannelPtr(new cChannel);
                Channel->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                if (Channel->SetTransponderData(Source, FrequenciesHz[0], SymbolRate, dtp))
                {
                  EITScanner.AddTransponder(Channel);
                  Channel->NotifyObservers(ObservableMessageChannelChanged);
                }
              }
            }
          }
        }
        break;
      case SI::S2SatelliteDeliverySystemDescriptorTag:
        {
          if (cSettings::Get().m_iUpdateChannels >= 5)
          {
            std::vector<ChannelPtr> channels =
                cChannelManager::Get().GetCurrent();
            for (std::vector<ChannelPtr>::const_iterator it = channels.begin();
                it != channels.end(); ++it)
            {
              ChannelPtr Channel = (*it);
              if (!Channel->GroupSep() && cSource::IsSat(Channel->Source())
                  && Channel->Nid() == ts.getOriginalNetworkId()
                  && Channel->Tid() == ts.getTransportStreamId())
              {
                SI::S2SatelliteDeliverySystemDescriptor *sd =
                    (SI::S2SatelliteDeliverySystemDescriptor *) d;
                cDvbTransponderParams dtp(Channel->Parameters());
                dtp.SetSystem(DVB_SYSTEM_2);
                dtp.SetStreamId(sd->getInputStreamIdentifier());
                Channel->SetTransponderData(Channel->Source(), Channel->FrequencyHz(), Channel->Srate(), dtp);
                Channel->NotifyObservers(ObservableMessageChannelChanged);
                break;
              }
            }
          }
        }
        break;
      case SI::CableDeliverySystemDescriptorTag:
        {
          SI::CableDeliverySystemDescriptor *sd =
              (SI::CableDeliverySystemDescriptor *) d;
          cDvbTransponderParams dtp;
          int Source = cSource::FromData(cSource::stCable);
          int iFrequencyHz = FrequenciesHz[0] = BCD2INT(sd->getFrequency()) / 10;
          //XXX FEC_outer???
          static fe_code_rate CodeRates[] =
            { FEC_NONE, FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6, FEC_7_8, FEC_8_9,
                FEC_3_5, FEC_4_5, FEC_9_10, FEC_AUTO, FEC_AUTO, FEC_AUTO,
                FEC_AUTO, FEC_AUTO, FEC_NONE };
          dtp.SetCoderateH(CodeRates[sd->getFecInner()]);
          static fe_modulation Modulations[] =
            { QPSK, QAM_16, QAM_32, QAM_64, QAM_128, QAM_256, QAM_AUTO };
          dtp.SetModulation(Modulations[std::min(sd->getModulation(), 6)]);
          int SymbolRate = BCD2INT(sd->getSymbolRate()) / 10;
          if (thisNit != m_nits.end())
          {
            for (int n = 0; n < NumFrequencies; n++)
            {
              if (ISTRANSPONDER(FrequenciesHz[n] / 1000, Transponder()))
              {
                thisNit->hasTransponder = true;
                //printf("has transponder %d\n", Transponder());
                break;
              }
            }
            break;
          }
          if (cSettings::Get().m_iUpdateChannels >= 5)
          {
            bool found = false;
            bool forceTransponderUpdate = false;
            std::vector<ChannelPtr> channels =
                cChannelManager::Get().GetCurrent();
            for (std::vector<ChannelPtr>::const_iterator it = channels.begin();
                it != channels.end(); ++it)
            {
              ChannelPtr Channel = (*it);
              if (!Channel->GroupSep() && Channel->Source() == Source
                  && Channel->Nid() == ts.getOriginalNetworkId()
                  && Channel->Tid() == ts.getTransportStreamId())
              {
                int transponder = Channel->Transponder();
                found = true;
                if (!ISTRANSPONDER(iFrequencyHz / 1000, transponder))
                {
                  for (int n = 0; n < NumFrequencies; n++)
                  {
                    if (ISTRANSPONDER(FrequenciesHz[n] / 1000, transponder))
                    {
                      iFrequencyHz = FrequenciesHz[n];
                      break;
                    }
                  }
                }
                if (ISTRANSPONDER(iFrequencyHz / 1000, Transponder())) // only modify channels if we're actually receiving this transponder
                  Channel->SetTransponderData(Source, iFrequencyHz, SymbolRate, dtp);
                else if (Channel->Srate() != SymbolRate || Channel->Parameters() != dtp)
                  forceTransponderUpdate = true; // get us receiving this transponder
              }
            }
            if (!found || forceTransponderUpdate)
            {
              for (int n = 0; n < NumFrequencies; n++)
              {
                ChannelPtr Channel = ChannelPtr(new cChannel);
                Channel->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                if (Channel->SetTransponderData(Source, FrequenciesHz[n], SymbolRate, dtp))
                {
                  EITScanner.AddTransponder(Channel);
                  Channel->NotifyObservers(ObservableMessageChannelChanged);
                }
              }
            }
          }
        }
        break;
      case SI::TerrestrialDeliverySystemDescriptorTag:
        {
          SI::TerrestrialDeliverySystemDescriptor *sd =
              (SI::TerrestrialDeliverySystemDescriptor *) d;
          cDvbTransponderParams dtp;
          int Source = cSource::FromData(cSource::stTerr);
          int iFrequencyHz = FrequenciesHz[0] = sd->getFrequency() * 10;
          static fe_bandwidth Bandwidths[] = { BANDWIDTH_8_MHZ, BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ, BANDWIDTH_5_MHZ };
          if (sd->getBandwidth() < ARRAY_SIZE(Bandwidths))
            dtp.SetBandwidth(Bandwidths[sd->getBandwidth()]);
          else
            dtp.SetBandwidth(BANDWIDTH_8_MHZ); // Note: Default value was 0 Hz ??? Changed to 8 MHz instead
          static fe_modulation Constellations[] = { QPSK, QAM_16, QAM_64, QAM_AUTO };
          dtp.SetModulation(Constellations[sd->getConstellation()]);
          dtp.SetSystem(DVB_SYSTEM_1);
          static fe_hierarchy Hierarchies[] =
            { HIERARCHY_NONE, HIERARCHY_1, HIERARCHY_2, HIERARCHY_4,
                HIERARCHY_AUTO, HIERARCHY_AUTO, HIERARCHY_AUTO, HIERARCHY_AUTO };
          dtp.SetHierarchy(Hierarchies[sd->getHierarchy()]);
          static fe_code_rate CodeRates[] =
            { FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6, FEC_7_8, FEC_AUTO, FEC_AUTO,
                FEC_AUTO };
          dtp.SetCoderateH(CodeRates[sd->getCodeRateHP()]);
          dtp.SetCoderateL(CodeRates[sd->getCodeRateLP()]);
          static fe_guard_interval GuardIntervals[] =
            { GUARD_INTERVAL_1_32, GUARD_INTERVAL_1_16, GUARD_INTERVAL_1_8,
                GUARD_INTERVAL_1_4 };
          dtp.SetGuard(GuardIntervals[sd->getGuardInterval()]);
          static fe_transmit_mode TransmissionModes[] =
            { TRANSMISSION_MODE_2K, TRANSMISSION_MODE_8K, TRANSMISSION_MODE_4K,
                TRANSMISSION_MODE_AUTO };
          dtp.SetTransmission(TransmissionModes[sd->getTransmissionMode()]);
          if (thisNit != m_nits.end())
          {
            for (int n = 0; n < NumFrequencies; n++)
            {
              if (ISTRANSPONDER(FrequenciesHz[n] / 1000000, Transponder()))
              {
                thisNit->hasTransponder = true;
                //printf("has transponder %d\n", Transponder());
                break;
              }
            }
            break;
          }
          if (cSettings::Get().m_iUpdateChannels >= 5)
          {
            bool found = false;
            bool forceTransponderUpdate = false;
            std::vector<ChannelPtr> channels =
                cChannelManager::Get().GetCurrent();
            for (std::vector<ChannelPtr>::const_iterator it = channels.begin();
                it != channels.end(); ++it)
            {
              ChannelPtr Channel = (*it);
              if (!Channel->GroupSep() && Channel->Source() == Source
                  && Channel->Nid() == ts.getOriginalNetworkId()
                  && Channel->Tid() == ts.getTransportStreamId())
              {
                int transponder = Channel->Transponder();
                found = true;
                if (!ISTRANSPONDER(iFrequencyHz / 1000000, transponder))
                {
                  for (int n = 0; n < NumFrequencies; n++)
                  {
                    if (ISTRANSPONDER(FrequenciesHz[n] / 1000000, transponder))
                    {
                      iFrequencyHz = FrequenciesHz[n];
                      break;
                    }
                  }
                }
                if (ISTRANSPONDER(iFrequencyHz / 1000000, Transponder())) // only modify channels if we're actually receiving this transponder
                  Channel->SetTransponderData(Source, iFrequencyHz, 0, dtp);
                else if (Channel->Parameters() != dtp)
                  forceTransponderUpdate = true; // get us receiving this transponder
              }
            }
            if (!found || forceTransponderUpdate)
            {
              for (int n = 0; n < NumFrequencies; n++)
              {
                ChannelPtr Channel = ChannelPtr(new cChannel);
                Channel->SetId(ts.getOriginalNetworkId(),
                    ts.getTransportStreamId(), 0, 0);
                if (Channel->SetTransponderData(Source, FrequenciesHz[n], 0, dtp))
                  EITScanner.AddTransponder(Channel);
              }
            }
          }
        }
        break;
      case SI::ExtensionDescriptorTag:
        {
          SI::ExtensionDescriptor *sd = (SI::ExtensionDescriptor *) d;
          switch (sd->getExtensionDescriptorTag())
            {
          case SI::T2DeliverySystemDescriptorTag:
            {
              if (cSettings::Get().m_iUpdateChannels >= 5)
              {
                std::vector<ChannelPtr> channels =
                    cChannelManager::Get().GetCurrent();
                for (std::vector<ChannelPtr>::const_iterator it =
                    channels.begin(); it != channels.end(); ++it)
                {
                  ChannelPtr Channel = (*it);
                  int Source = cSource::FromData(cSource::stTerr);
                  if (!Channel->GroupSep() && Channel->Source() == Source
                      && Channel->Nid() == ts.getOriginalNetworkId()
                      && Channel->Tid() == ts.getTransportStreamId())
                  {
                    SI::T2DeliverySystemDescriptor *td =
                        (SI::T2DeliverySystemDescriptor *) d;
                    int FrequencyHz = Channel->FrequencyHz();
                    int SymbolRate = Channel->Srate();
                    //int SystemId = td->getSystemId();
                    cDvbTransponderParams dtp(Channel->Parameters());
                    dtp.SetSystem(DVB_SYSTEM_2);
                    dtp.SetStreamId(td->getPlpId());
                    if (td->getExtendedDataFlag())
                    {
                      static fe_bandwidth T2Bandwidths[] =
                        { BANDWIDTH_8_MHZ, BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ, BANDWIDTH_5_MHZ, BANDWIDTH_10_MHZ, BANDWIDTH_1_712_MHZ };
                      if (td->getBandwidth() < ARRAY_SIZE(T2Bandwidths))
                        dtp.SetBandwidth(T2Bandwidths[td->getBandwidth()]);
                      else
                        dtp.SetBandwidth(BANDWIDTH_8_MHZ); // Note: Default value was 0 Hz ??? Changed to 8 MHz instead
                      static fe_guard_interval T2GuardIntervals[] =
                        { GUARD_INTERVAL_1_32, GUARD_INTERVAL_1_16,
                            GUARD_INTERVAL_1_8, GUARD_INTERVAL_1_4,
                            GUARD_INTERVAL_1_128, GUARD_INTERVAL_19_128,
                            GUARD_INTERVAL_19_256 };
                      if (td->getGuardInterval() < ARRAY_SIZE(T2GuardIntervals))
                        dtp.SetGuard(T2GuardIntervals[td->getGuardInterval()]);
                      else
                        dtp.SetGuard(GUARD_INTERVAL_AUTO); // Note: Default value was 0 ??? Changed to GUARD_INTERVAL_AUTO instead
                      static fe_transmit_mode T2TransmissionModes[] =
                        { TRANSMISSION_MODE_2K, TRANSMISSION_MODE_8K,
                            TRANSMISSION_MODE_4K, TRANSMISSION_MODE_1K,
                            TRANSMISSION_MODE_16K, TRANSMISSION_MODE_32K,
                            TRANSMISSION_MODE_AUTO, TRANSMISSION_MODE_AUTO };
                      dtp.SetTransmission(
                          T2TransmissionModes[td->getTransmissionMode()]);
                      //TODO add parsing of frequencies
                    }
                    Channel->SetTransponderData(Source, FrequencyHz, SymbolRate, dtp);
                    Channel->NotifyObservers(ObservableMessageChannelChanged);
                  }
                }
              }
            }
            break;
          default:
            ;
            }
        }
        break;
      default:
        ;
        }
      delete d;
    }
  }
  //XXX Channels.Unlock();
}

}
