/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "NITScanner.h"
#include "scan/dvb_wrapper.h"
#include "sources/Source.h"
#include "utils/StringUtils.h"

#include <libsi/si_ext.h>
#include <libsi/descriptor.h>
#include <libsi/section.h>
#include <libsi/util.h>
#include <string>

using namespace SI_EXT;
using namespace std;

cNitScanner::cNitScanner(iNitScannerCallback* callback, bool bUseOtherTable /* = false */)
 : m_tableId(bUseOtherTable ? TABLE_ID_NIT_OTHER : TABLE_ID_NIT_ACTUAL),
   m_callback(callback)
 {
  assert(m_callback);

  // Network information section, actual or other network
  Set(PID_NIT, m_tableId, 0xFF);
}

void cNitScanner::ProcessData(u_short Pid, u_char Tid, const u_char * Data, int Length)
{
  SI::NIT nit(Data, false);
  if (!nit.CheckCRCAndParse() || !m_syncNit.Sync(nit.getVersionNumber(), nit.getSectionNumber(), nit.getLastSectionNumber()))
    return;

  //HEXDUMP(Data, Length);

  SI::NIT::TransportStream ts;

  for (SI::Loop::Iterator it; nit.transportStreamLoop.getNext(ts, it); )
  {
    SI::Descriptor *              d;
    SI::Loop::Iterator            it2;
    SI::FrequencyListDescriptor * fld = (SI::FrequencyListDescriptor *) ts.transportStreamDescriptors.getNext(it2, SI::FrequencyListDescriptorTag);
    int NumFrequencies = fld ? fld->frequencies.getCount() + 1 : 1;
    int Frequencies[NumFrequencies];

    if (fld)
    {
      dsyslog("   NIT: has fld");
      int ct = fld->getCodingType();
      if (ct > 0)
      {
        int n = 1;
        for (SI::Loop::Iterator it3; fld->frequencies.hasNext(it3);)
        {
          int iFrequencyHz = fld->frequencies.getNext(it3);
          switch (ct)
          {
          // Satellite
          case 1: iFrequencyHz = BCD2INT(iFrequencyHz) / 100;
            break;

          // Cable
          case 2: iFrequencyHz = BCD2INT(iFrequencyHz) / 10;
            break;

          // Terrestrial
          case 3: iFrequencyHz *= 10;
            break;

          default:
            break;
          }
          Frequencies[n++] = iFrequencyHz;
        }
      }
      else
        NumFrequencies = 1;
    }

    DELETENULL(fld);
    // dirty hack because libsi is missing needed cell_frequency_link_descriptor
    // and support is only possible with patching libsi :-((
    //  -> has to be removed as soon libsi supports cell_frequency_link_descriptor

    int    offset = 16 + (((*(Data + 8) << 8) & 0x0F00) | *(Data + 9));
    int    stop   = ((*(Data + offset) << 8) & 0x0F00) | *(Data + offset + 1);
    int    cellFrequencies[255];
    int    NumCellFrequencies = 0;

    offset += 2;         // Transport_descriptor_length
    stop   += offset;

    while (offset < stop)
    {
      int len = *(Data + offset + 1);
      switch (*(Data + offset))
      {
        // cell_frequency_list_descriptor, DVB-T only.
        case 0x6D:
        {
          dsyslog("   NIT: cell_frequency_list_descriptor -> NOT HANDLED BY LIBSI UP TO NOW :-((");
          int descriptor_length = *(Data + ++ offset);
          while (descriptor_length >= 7)
          {
            descriptor_length -= 7;
            offset            += 2; // cell_id
            int frequency_hi_hi = *(Data + ++ offset);
            int frequency_hi_lo = *(Data + ++ offset);
            int frequency_lo_hi = *(Data + ++ offset);
            int frequency_lo_lo = *(Data + ++ offset);
            cellFrequencies[NumCellFrequencies++] = 10 * ((HILO(frequency_hi) << 16) | HILO(frequency_lo));

            // subcell_info_loop_length -> skipped
            if (*(Data + ++ offset))
            {
              descriptor_length -= *(Data + offset);
              offset            += *(Data + offset);
            }
          }
          offset++;
          break;
        }
        default:
          // all other descriptors handled regularly //
          offset += len + 2;
          break;
      }
      if (!len)
        break;
    }
    // end dirty hack

    for (SI::Loop::Iterator it2; (d = ts.transportStreamDescriptors.getNext(it2));)
    {
      switch ((unsigned) d->getDescriptorTag())
      {
        case SI::SatelliteDeliverySystemDescriptorTag:
        {
          SI::SatelliteDeliverySystemDescriptor * sd = (SI::SatelliteDeliverySystemDescriptor *) d;
          int Source  = cSource::FromData(cSource::stSat, BCD2INT(sd->getOrbitalPosition()), sd->getWestEastFlag() ? cSource::sdEast : cSource::sdWest);
          fe_rolloff_t RollOff = ROLLOFF_35; // Implied value in DVB-S, default for DVB-S2 (per frontend.h)
          fe_modulation ModulationType = QPSK;
          int System = 0;

          if ((System = sd->getModulationSystem())) // {DVB-S}
          {
            ModulationType = SatModulationTypes((eSatModulationTypes) sd->getModulationType()); // {AUTO, QPSK, 8PSK, 16-QAM}
            RollOff = SatRollOffs((eSatRollOffs) sd->getRollOff());
          }

          Frequencies[0] = BCD2INT(sd->getFrequency()) / 100;
          fe_polarization Polarization = SatPolarizations((eSatPolarizations) sd->getPolarization());
          fe_code_rate_t CodeRate     = CableSatCodeRates((eCableSatCodeRates) sd->getFecInner());
          int  SymbolRate   = BCD2INT(sd->getSymbolRate()) / 10;
          dsyslog("%s: System=%d ModulationType=%d sd->getPolarization()=%d SatPolarizations(sd->getPolarization())=%d Polarization=%c CodeRate=%d SymbolRate=%d",
                  __FUNCTION__, System, ModulationType, sd->getPolarization(), SatPolarizations((eSatPolarizations) sd->getPolarization()), Polarization, CodeRate, SymbolRate);

          for (int n = 0; n < NumFrequencies; n++)
          {
            ChannelPtr transponder = ChannelPtr(new cChannel);
            transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);

            cDvbTransponderParams params;
            params.SetPolarization(Polarization);
            params.SetCoderateH(CodeRate);
            params.SetModulation(ModulationType);
            params.SetSystem(System == SYS_DVBS ? DVB_SYSTEM_1 : DVB_SYSTEM_2);
            params.SetRollOff(RollOff);

            if (transponder->SetTransponderData(Source, Frequencies[n], SymbolRate, params, true))
              m_callback->NitFoundTransponder(transponder, m_tableId == TABLE_ID_NIT_OTHER, ts.getOriginalNetworkId(), ts.getTransportStreamId());
          }
          break;
        }
        case SI::S2SatelliteDeliverySystemDescriptorTag:
        {
          // only interesting if NBC-BS
          SI::S2SatelliteDeliverySystemDescriptor * sd = (SI::S2SatelliteDeliverySystemDescriptor *) d;
          #if 0                                                               //i have no idea wether i need the scrambling index and if so for what.., 0 is default anyway.
          int scrambling_sequence_index = (sd->getScramblingSequenceSelector()) ? sd->getScramblingSequenceIndex() : 0;
          #endif
          int DVBS_backward_compatibility = sd->getBackwardsCompatibilityIndicator();
          if (DVBS_backward_compatibility)
          {
            // okay: we should add a dvb-s transponder
            // with same source, same polarization and QPSK to list of transponders
            // if this transponder isn't already marked as known.
            //
          }
          // now we should re-check wether this s2 transponder is really known//
          break;
        }
        case SI::CableDeliverySystemDescriptorTag:
        {
          SI::CableDeliverySystemDescriptor * sd = (SI::CableDeliverySystemDescriptor *) d;
          int Source = cSource::FromData(cSource::stCable);
          Frequencies[0] = BCD2INT(sd->getFrequency()) / 10;
          fe_code_rate_t CodeRate   = CableSatCodeRates((eCableSatCodeRates) sd->getFecInner());
          fe_modulation_t Modulation = CableModulations((eCableModulations) std::min(sd->getModulation(), 6));
          int SymbolRate = BCD2INT(sd->getSymbolRate()) / 10;
          for (int n = 0; n < NumFrequencies; n++)
          {
            ChannelPtr transponder = ChannelPtr(new cChannel);
            transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);

            cDvbTransponderParams params;
            params.SetPolarization(POLARIZATION_VERTICAL); // TODO: Is this necessary?
            params.SetInversion(INVERSION_AUTO); // TODO: Necessary?
            params.SetBandwidth(BANDWIDTH_8_MHZ); // TODO: Necessary?
            params.SetCoderateH(CodeRate);
            params.SetModulation(Modulation);
            params.SetSystem((eSystemType)SYS_DVBC_ANNEX_AC); // TODO: ???

            if (transponder->SetTransponderData(Source, Frequencies[n], SymbolRate, params, true))
              m_callback->NitFoundTransponder(transponder, m_tableId == TABLE_ID_NIT_OTHER, ts.getOriginalNetworkId(), ts.getTransportStreamId());
          }
          break;
        }
        case SI::TerrestrialDeliverySystemDescriptorTag:
        {
          SI::TerrestrialDeliverySystemDescriptor * sd = (SI::TerrestrialDeliverySystemDescriptor *) d;
          int Source = cSource::FromData(cSource::stTerr);
          Frequencies[0] = sd->getFrequency() * 10;
          int Bandwidth        = TerrBandwidths((eTerrBandwidths) sd->getBandwidth());
          fe_modulation_t Constellation    = TerrConstellations((eTerrConstellations) sd->getConstellation());
          fe_hierarchy_t Hierarchy        = TerrHierarchies((eTerrHierarchies) sd->getHierarchy());
          fe_code_rate_t CodeRateHP       = TerrCodeRates((eTerrCodeRates) sd->getCodeRateHP());
          fe_code_rate_t CodeRateLP       = TerrCodeRates((eTerrCodeRates) sd->getCodeRateLP());
          fe_guard_interval_t GuardInterval    = TerrGuardIntervals((eTerrGuardIntervals) sd->getGuardInterval());
          fe_transmit_mode_t TransmissionMode = TerrTransmissionModes((eTerrTransmissionModes) sd->getTransmissionMode());

          for (int n = 0; n < NumFrequencies; n++)
          {
            ChannelPtr transponder = ChannelPtr(new cChannel);
            transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);

            cDvbTransponderParams params;
            params.SetInversion(INVERSION_AUTO); // TODO: Necessary?
            params.SetCoderateH(CodeRateHP);
            params.SetCoderateL(CodeRateLP);
            params.SetModulation(Constellation);
            params.SetSystem(DVB_SYSTEM_1); // SYS_DVBT
            params.SetTransmission(TransmissionMode);
            params.SetGuard(GuardInterval);
            params.SetHierarchy(Hierarchy);

            if (transponder->SetTransponderData(Source, Frequencies[n], 27500, params, true))
              m_callback->NitFoundTransponder(transponder, m_tableId == TABLE_ID_NIT_OTHER, ts.getOriginalNetworkId(), ts.getTransportStreamId());
          }
///
 //         for (int n = 0; n < NumCellFrequencies; n++)
 //         {
 //           cChannel * transponder = new cChannel;
 //           transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
 //           if (transponder->SetTransponderData(Source, cellFrequencies[n], 27500, ParamsToString('T', 0, CableTerrInversions(eInversionAuto), Bandwidth, CodeRateHP, CodeRateLP, Constellation, SYS_DVBT, TransmissionMode, GuardInterval, Hierarchy, 0), true))
 //           {
 //             if (GetByParams(m_scannedTransponders, newTransponder) == cChannel::EmptyChannel) // add only unknown transponders
 //             {
 //               dsyslog("   Add: %s", PrintTransponder(transponder).c_str());
 //               m_transponders.push_back(transponder);
 //             }
 //             else
 //               DELETENULL(transponder);
 //           }
 //         }
 ///
          break;
        }
        case SI::ServiceListDescriptorTag:
        {
          SI::ServiceListDescriptor *        sd = (SI::ServiceListDescriptor *) d;
          SI::ServiceListDescriptor::Service Service;
          for (SI::Loop::Iterator it; sd->serviceLoop.getNext(Service, it);)
          {
            if (Service.getServiceType() == 0x04 ||            // NVOD reference service
                Service.getServiceType() == 0x18 ||            // advanced codec SD NVOD reference service
                Service.getServiceType() == 0x1B)              // advanced codec HD NVOD reference service
            {
              continue;
            }
            if (Service.getServiceType() == 0x03 ||            // Teletext service
                Service.getServiceType() == 0x04 ||            // NVOD reference service (see note 1)
                Service.getServiceType() == 0x05 ||            // NVOD time-shifted service (see note 1)
                Service.getServiceType() == 0x06 ||            // mosaic service
                Service.getServiceType() == 0x0C ||            // data broadcast service
                Service.getServiceType() == 0x0D ||            // reserved for Common Interface Usage (EN 50221 [39])
                Service.getServiceType() == 0x0E ||            // RCS Map (see EN 301 790 [7])
                Service.getServiceType() == 0x0F ||            // RCS FLS (see EN 301 790 [7])
                Service.getServiceType() == 0x10 ||            // DVB MHP service
                Service.getServiceType() == 0x17 ||            // advanced codec SD NVOD time-shifted service
                Service.getServiceType() == 0x18 ||            // advanced codec SD NVOD reference service
                Service.getServiceType() == 0x1A ||            // advanced codec HD NVOD time-shifted service
                Service.getServiceType() == 0x1B)              // advanced codec HD NVOD reference service
            {
              // TODO: Do we need a callback for updating discovered channels?

              /*
              ChannelPtr channel = cChannelManager::GetByServiceID(m_newChannels, Source(), Transponder(), Service.getServiceId());
              if (channel && !(channel->Rid() & INVALID_CHANNEL))
              {
                dsyslog("   NIT: invalid (service_type=0x%.2x)", Service.getServiceType());
                channel->SetId(channel->Nid(), channel->Tid(), channel->Sid(), INVALID_CHANNEL);
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
          dsyslog("   NIT: unknown descriptor tag 0x%.2x", d->getDescriptorTag());
          break;
      }
      DELETENULL(d);
    }
  }
}
