/*
 * scanfilter.c: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * wirbelscan-0.0.5
 *
 * $Id$ v20101210
 */

/* cNitScanner, cPatScanner, cSdtScanner, cEitScanner are modified versions of core VDR's
 * own classes cEIT, cNitFilter, cSdtFilter.
 * License GPL, original Copyright (C) Klaus Schmidinger, see www.tvdr.de/vdr
 */

#include <libsi/section.h>
#include <libsi/descriptor.h>
#include <vdr/filter.h>
#include <vdr/device.h>
#include <vdr/channels.h>
#include <vdr/sources.h>
#include <vdr/tools.h>
#include <vdr/config.h>
#include "scanfilter.h"
#include "common.h"
#include "dvb_wrapper.h"
#include "menusetup.h"
#include "si_ext.h"

using namespace SI_EXT;

cChannels     NewChannels;
cTransponders NewTransponders;
cTransponders ScannedTransponders;
int           nextTransponders;

void resetLists() {
  NewChannels.Load(NULL, false, false);
  NewTransponders.Load(NULL, false, false);
  ScannedTransponders.Load(NULL, false, false);
  nextTransponders = 0;
  }

int FormatFreq(int f) {
  if (f < 1000) {
    f *= 1000;
    }
  if (f > 999999) {
    f /= 1000;
    }
  return (f);
  }

bool is_known_initial_transponder(cChannel * newChannel, bool auto_allowed, cChannels * list) {
  dlog(4, "%s", __FUNCTION__);
  if (list == NULL) {
    return (is_known_initial_transponder(newChannel, auto_allowed, &NewTransponders) ||
            is_known_initial_transponder(newChannel, auto_allowed, &ScannedTransponders));
    }

  for (cChannel * channel = list->First(); channel; channel = list->Next(channel)) {
    if (channel->GroupSep()) {
      continue;
      }
    if (newChannel->IsTerr()) {
      if ((channel->Source() == newChannel->Source()) && is_nearly_same_frequency(channel, newChannel)) {
        return (true);
        }
      }
    if (newChannel->IsCable()) {
      if (newChannel->Ca() != 0xA1) {
        if ((channel->Source() == newChannel->Source()) && is_nearly_same_frequency(channel, newChannel)) {
          return (true);
          }
        }
      else {
        if ((channel->Ca() == newChannel->Ca()) && (channel->Source() == newChannel->Source()) &&
            is_nearly_same_frequency(channel, newChannel, 50)) {
          return (true);
          }
        }
      }
#if VDRVERSNUM > 10713
    if (newChannel->IsAtsc()) {
      cDvbTransponderParameters * p  = new cDvbTransponderParameters(channel->Parameters());
      cDvbTransponderParameters * pn = new cDvbTransponderParameters(newChannel->Parameters());
      if ((channel->Source() == newChannel->Source()) && is_nearly_same_frequency(channel, newChannel) &&
          (p->Modulation() == pn->Modulation())) {
        return (true);
        }
      }
#endif
    if (IsPvrinput(newChannel)) {
      if ((channel->Source() == newChannel->Source()) && is_nearly_same_frequency(channel, newChannel) &&
          #if VDRVERSNUM > 10712
          ! strcmp(channel->Parameters(), newChannel->Parameters())) {
          #elif defined (PLUGINPARAMPATCHVERSNUM)
          ! strcmp(channel->PluginParam(), newChannel->PluginParam())) {
          #else
          // remove "true) {", as soon as plugin param patch removed.
          true) {
          #endif
        return (true);
        }
      }
    if (newChannel->IsSat()) {
      if (!is_different_transponder_deep_scan(newChannel, channel, auto_allowed)) {
        return (true);
        }
      }
    }
  return (false);
  }

bool is_nearly_same_frequency(const cChannel * chan_a, const cChannel * chan_b, uint delta) {
  uint32_t diff;
  int      f1 = FormatFreq(chan_a->Frequency());
  int      f2 = FormatFreq(chan_b->Frequency());

  dlog(4, "%s, f1=%d, f2=%d", __FUNCTION__, f1, f2);
  if (f1 == f2) {
    return (true);
    }
  diff = (f1 > f2) ? (f1 - f2) : (f2 - f1);
  //FIXME: use symbolrate etc. to estimate bandwidth
  if (diff < delta) {
    dlog(4, "f1 = %u is same TP as f2 = %u", f1, f2);
    return (true);
    }
  return (false);
  }

bool is_different_transponder_deep_scan(const cChannel * a, const cChannel * b, bool auto_allowed) {
#define IS_DIFFERENT(A, B, _ALLOW_AUTO_, _AUTO_)    ((A != B) && (!_ALLOW_AUTO_ || (_ALLOW_AUTO_ && (A != _AUTO_) && (B != _AUTO_))))

  int maxdelta = 2001;

  if (a->IsSat()) maxdelta = 2;

  dlog(4, "%s", __FUNCTION__);
  if (a->Source() != b->Source()) {
    return (true);
    }

  if (!is_nearly_same_frequency(a, b, maxdelta)) {
    return (true);
    }
#if VDRVERSNUM < 10713
  if (a->IsTerr()) {
    if (IS_DIFFERENT(a->Modulation(), b->Modulation(), auto_allowed, QAM_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(a->Bandwidth(), b->Bandwidth(), auto_allowed, BANDWIDTH_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(a->CoderateH(), b->CoderateH(), auto_allowed, FEC_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(a->Hierarchy(), b->Hierarchy(), auto_allowed, HIERARCHY_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(a->CoderateL(), b->CoderateL(), auto_allowed, FEC_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(a->Transmission(), b->Transmission(), auto_allowed, TRANSMISSION_MODE_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(a->Guard(), b->Guard(), auto_allowed, GUARD_INTERVAL_AUTO)) {
      return (true);
      }
    return (false);
    }
  if (a->IsCable()) {
    if (IS_DIFFERENT(a->Modulation(), b->Modulation(), auto_allowed, QAM_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(a->Srate(), b->Srate(), false, 6900)) {
      return (true);
      }
    if (IS_DIFFERENT(a->CoderateH(), b->CoderateH(), auto_allowed, FEC_AUTO)) {
      return (true);
      }
    return (false);
    }
  if (a->IsSat()) {
    if (IS_DIFFERENT(a->Srate(), b->Srate(), false, 27500)) {
      return (true);
      }
    if (IS_DIFFERENT(a->Polarization(), b->Polarization(), false, 0)) {
      return (true);
      }
    if (IS_DIFFERENT(a->CoderateH(), b->CoderateH(), auto_allowed, FEC_AUTO)) {
      return (true);
      }
#if VDRVERSNUM >= 10700
    if (IS_DIFFERENT(a->System(), b->System(), false, 0)) {
      return (true);
      }
    if (IS_DIFFERENT(a->RollOff(), b->RollOff(), auto_allowed, ROLLOFF_35)) {
      return (true);
      }
    if (IS_DIFFERENT(a->Modulation(), b->Modulation(), auto_allowed, QPSK)) {
      return (true);
      }
#endif
    return (false);
    }
#else
  cDvbTransponderParameters * ap = new cDvbTransponderParameters(a->Parameters());
  cDvbTransponderParameters * bp = new cDvbTransponderParameters(b->Parameters());
  if (a->IsTerr()) {
    if (IS_DIFFERENT(ap->Modulation(), bp->Modulation(), auto_allowed, QAM_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->Bandwidth(), bp->Bandwidth(), auto_allowed, BANDWIDTH_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->CoderateH(), bp->CoderateH(), auto_allowed, FEC_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->Hierarchy(), bp->Hierarchy(), auto_allowed, HIERARCHY_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->CoderateL(), bp->CoderateL(), auto_allowed, FEC_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->Transmission(), bp->Transmission(), auto_allowed, TRANSMISSION_MODE_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->Guard(), bp->Guard(), auto_allowed, GUARD_INTERVAL_AUTO)) {
      return (true);
      }
    return (false);
    }
#if VDRVERSNUM > 10713
  if (a->IsAtsc()) {
    if (IS_DIFFERENT(ap->Modulation(), bp->Modulation(), auto_allowed, QAM_AUTO)) {
      return (true);
      }
    return (false);
    }
#endif
  if (a->IsCable()) {
    if (IS_DIFFERENT(ap->Modulation(), bp->Modulation(), auto_allowed, QAM_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(a->Srate(), b->Srate(), false, 6900)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->CoderateH(), bp->CoderateH(), auto_allowed, FEC_AUTO)) {
      return (true);
      }
    return (false);
    }
  if (a->IsSat()) {
    if (IS_DIFFERENT(a->Srate(), b->Srate(), false, 27500)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->Polarization(), bp->Polarization(), false, 0)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->CoderateH(), bp->CoderateH(), auto_allowed, FEC_AUTO)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->System(), bp->System(), false, 0)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->RollOff(), bp->RollOff(), auto_allowed, ROLLOFF_35)) {
      return (true);
      }
    if (IS_DIFFERENT(ap->Modulation(), bp->Modulation(), auto_allowed, QPSK)) {
      return (true);
      }
    return (false);
    }

#endif
  dlog(0, "%s: unknown source type", __FUNCTION__);
  return (true);
  }

//--------cTransponder-------------------------------------------------------------------------

cChannel * cTransponders::GetByParams(const cChannel * NewTransponder) {
  dlog(4, "%s(%s)", __FUNCTION__, *PrintTransponder(NewTransponder));
  if (Count() > 0) {
    for (cChannel * tr = First(); tr; tr = Next(tr)) {
      if (!is_different_transponder_deep_scan(tr, NewTransponder, true)) {
        return (tr);
        }
      }
    }
  return (NULL);
  }

bool cTransponders::IsUniqueTransponder(const cChannel * NewTransponder) {
  return (GetByParams(NewTransponder) == NULL);
  }

//--------cNitScanner-------------------------------------------------------------------------
// basically this is cNitFilter from vdr/nit.{h,c} with some changes //

cNitScanner::cNitScanner(int TableId) {
  active    = true;
  tableId   = TableId;
  numNits   = 0;
  networkId = 0;
  syncNit.Reset();
  Set(PID_NIT, tableId, 0xFF);     //network information section, actual or other network
  Start();
  }

void cNitScanner::Process(u_short Pid, u_char Tid, const u_char * Data, int Length) {
  SI::NIT nit(Data, false);
  if (!nit.CheckCRCAndParse() ||
      !syncNit.Sync(nit.getVersionNumber(), nit.getSectionNumber(), nit.getLastSectionNumber())) {
    return;
    }

  HEXDUMP(Data, Length);
  SI::NIT::TransportStream ts;
  for (SI::Loop::Iterator it; nit.transportStreamLoop.getNext(ts, it);) {
    SI::Descriptor *              d;
    SI::Loop::Iterator            it2;
    SI::FrequencyListDescriptor * fld = (SI::FrequencyListDescriptor *) ts.transportStreamDescriptors.getNext(it2, SI::FrequencyListDescriptorTag);
    int NumFrequencies = fld ? fld->frequencies.getCount() + 1 : 1;
    int Frequencies[NumFrequencies];
    if (fld) {
      dlog(4, "   NIT: has fld");
      int ct = fld->getCodingType();
      if (ct > 0) {
        int n = 1;
        for (SI::Loop::Iterator it3; fld->frequencies.hasNext(it3);) {
          int f = fld->frequencies.getNext(it3);
          switch (ct) {
             case 1: f = BCD2INT(f) / 100;
               break;                                  //satellite
             case 2: f = BCD2INT(f) / 10;
               break;                                  //cable
             case 3: f *= 10;
               break;                                  //terrestrial
             default:;
            }
          Frequencies[n++] = f;
          }
        }
      else {
        NumFrequencies = 1;
        }
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

    while (offset < stop) {
      int len = *(Data + offset + 1);
      switch (*(Data + offset)) {
         case 0x6D: {         // cell_frequency_list_descriptor, DVB-T only.
           dlog(4, "   NIT: cell_frequency_list_descriptor -> NOT HANDLED BY LIBSI UP TO NOW :-((");
           int descriptor_length = *(Data + ++ offset);
           while (descriptor_length >= 7) {
             descriptor_length -= 7;
             offset            += 2;            // cell_id
             int frequency_hi_hi = *(Data + ++ offset);
             int frequency_hi_lo = *(Data + ++ offset);
             int frequency_lo_hi = *(Data + ++ offset);
             int frequency_lo_lo = *(Data + ++ offset);
             cellFrequencies[NumCellFrequencies++] = 10 * ((HILO(frequency_hi) << 16) | HILO(frequency_lo));
             if (*(Data + ++ offset)) {            // subcell_info_loop_length -> skipped
               descriptor_length -= *(Data + offset);
               offset            += *(Data + offset);
               }
             }
           offset++;
           break;
           }
         default: offset += len + 2;         // all other descriptors handled regularly //
        }
      if (!len) {
        break;
        }
      }
    // end dirty hack

    for (SI::Loop::Iterator it2; (d = ts.transportStreamDescriptors.getNext(it2));) {
      switch ((unsigned) d->getDescriptorTag()) {
         case SI::SatelliteDeliverySystemDescriptorTag: {
           SI::SatelliteDeliverySystemDescriptor * sd = (SI::SatelliteDeliverySystemDescriptor *) d;
           int Source  = cSource::FromData(cSource::stSat, BCD2INT(sd->getOrbitalPosition()), sd->getWestEastFlag());
           int RollOff = 0;
           int ModulationType = QPSK;
           int System = 0;
           #if VDRVERSNUM >= 10700
           if ((System = sd->getModulationSystem())) { //{DVB-S}
             ModulationType = SatModulationTypes((eSatModulationTypes) sd->getModulationType()); // {AUTO, QPSK, 8PSK, 16-QAM}
             RollOff = SatRollOffs((eSatRollOffs) sd->getRollOff());
             }
           #endif
           Frequencies[0] = BCD2INT(sd->getFrequency()) / 100;
           char Polarization = GetVDRPolarizationFromDVB(SatPolarizations((eSatPolarizations) sd->getPolarization()));
           int  CodeRate     = CableSatCodeRates((eCableSatCodeRates) sd->getFecInner());
           int  SymbolRate   = BCD2INT(sd->getSymbolRate()) / 10;
           dlog(4,"%s: System=%d ModulationType=%d sd->getPolarization()=%d SatPolarizations(sd->getPolarization())=%d Polarization=%c CodeRate=%d SymbolRate=%d",
                   __FUNCTION__,
                   System, ModulationType, sd->getPolarization(), SatPolarizations((eSatPolarizations) sd->getPolarization()), Polarization, CodeRate, SymbolRate);

           for (int n = 0; n < NumFrequencies; n++) {
             cChannel * transponder = new cChannel;
             transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
             if (SetSatTransponderDataFromDVB(transponder, Source, Frequencies[n], Polarization, SymbolRate, CodeRate, ModulationType, System, RollOff)) {
               if (!is_known_initial_transponder(transponder, true)) {
                 if (System && !wSetup.enable_s2) {
                   dlog(3, "   s2 disabled: %s", *PrintTransponder(transponder));
                   DELETENULL(transponder);
                   continue;
                   }
                 dlog(3, "   Add: %s -> NID = %d, TID = %d", *PrintTransponder(transponder), transponder->Nid(), transponder->Tid());
                 NewTransponders.Add(transponder);
                 nextTransponders = NewTransponders.Count() - ScannedTransponders.Count();
                 }
               else {
                 // we already know this transponder, lets update these channels
                 cChannel * update_transponder = ScannedTransponders.GetByParams(transponder);
                 if ((tableId == TABLE_ID_NIT_ACTUAL) && (update_transponder != NULL)) {
                   // only NIT_actual should update existing channels
                   if (is_different_transponder_deep_scan(transponder, update_transponder, false)) {
                     SetSatTransponderDataFromDVB(update_transponder, Source, Frequencies[n], Polarization, SymbolRate, CodeRate, ModulationType, System, RollOff);
                     dlog(2, "   Upd: %s", *PrintTransponder(update_transponder));
                     }
                   if ((ts.getOriginalNetworkId() != update_transponder->Nid()) ||
                       (ts.getTransportStreamId() != update_transponder->Tid())) {
                     update_transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                     dlog(2, "   Upd: %s -> NID = %d, TID = %d", *PrintTransponder(update_transponder), update_transponder->Nid(), update_transponder->Tid());
                     }
                   }
                 DELETENULL(transponder);
                 }
               }
             }
           }
           break;
         case SI::S2SatelliteDeliverySystemDescriptorTag:
           #if VDRVERSNUM >= 10703
           {         // only interesting if NBC-BS
           SI::S2SatelliteDeliverySystemDescriptor * sd = (SI::S2SatelliteDeliverySystemDescriptor *) d;
           #if 0                                                               //i have no idea wether i need the scrambling index and if so for what.., 0 is default anyway.
           int scrambling_sequence_index = (sd->getScramblingSequenceSelector()) ? sd->getScramblingSequenceIndex() : 0;
           #endif
           int DVBS_backward_compatibility = sd->getBackwardsCompatibilityIndicator();
           if (DVBS_backward_compatibility) {
             // okay: we should add a dvb-s transponder
             // with same source, same polarization and QPSK to list of transponders
             // if this transponder isn't already marked as known.
             //
             }
             // now we should re-check wether this s2 transponder is really known//
           }
           #endif
           break;
         case SI::CableDeliverySystemDescriptorTag: {
           SI::CableDeliverySystemDescriptor * sd = (SI::CableDeliverySystemDescriptor *) d;
           int Source = cSource::FromData(cSource::stCable);
           Frequencies[0] = BCD2INT(sd->getFrequency()) / 10;
           int CodeRate   = CableSatCodeRates((eCableSatCodeRates) sd->getFecInner());
           int Modulation = CableModulations((eCableModulations) min(sd->getModulation(), 6));
           int SymbolRate = BCD2INT(sd->getSymbolRate()) / 10;
           for (int n = 0; n < NumFrequencies; n++) {
             cChannel * transponder = new cChannel;
             transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
             if (SetCableTransponderDataFromDVB(transponder, Source, Frequencies[n], Modulation, SymbolRate, CodeRate, CableTerrInversions(eInversionAuto))) {
               if (!is_known_initial_transponder(transponder, true)) {
                 dlog(3, "   Add: %s -> NID = %d, TID = %d", *PrintTransponder(transponder), transponder->Nid(), transponder->Tid());
                 NewTransponders.Add(transponder);
                 nextTransponders = NewTransponders.Count() - ScannedTransponders.Count();
                 }
               else {
                 // we already know this transponder, lets update these channels
                 cChannel * update_transponder = ScannedTransponders.GetByParams(transponder);
                 if ((tableId == TABLE_ID_NIT_ACTUAL) && (update_transponder != NULL)) {
                   // only NIT_actual should update existing channels
                   if (is_different_transponder_deep_scan(transponder, update_transponder, false)) {
                     SetCableTransponderDataFromDVB(update_transponder, Source, Frequencies[n], Modulation, SymbolRate, CodeRate, 999);
                     dlog(2, "   Upd: %s", *PrintTransponder(update_transponder));
                     }
                   if ((ts.getOriginalNetworkId() != update_transponder->Nid()) ||
                       (ts.getTransportStreamId() != update_transponder->Tid())) {
                     update_transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                     dlog(2, "   Upd: %s -> NID = %d, TID = %d", *PrintTransponder(update_transponder), update_transponder->Nid(), update_transponder->Tid());
                     }
                   }
                 DELETENULL(transponder);
                 }
               }
             }
           }
           break;
         case SI::TerrestrialDeliverySystemDescriptorTag: {
           SI::TerrestrialDeliverySystemDescriptor * sd = (SI::TerrestrialDeliverySystemDescriptor *) d;
           int Source = cSource::FromData(cSource::stTerr);
           Frequencies[0] = sd->getFrequency() * 10;
           int Bandwidth        = TerrBandwidths((eTerrBandwidths) sd->getBandwidth());
           int Constellation    = TerrConstellations((eTerrConstellations) sd->getConstellation());
           int Hierarchy        = TerrHierarchies((eTerrHierarchies) sd->getHierarchy());
           int CodeRateHP       = TerrCodeRates((eTerrCodeRates) sd->getCodeRateHP());
           int CodeRateLP       = TerrCodeRates((eTerrCodeRates) sd->getCodeRateLP());
           int GuardInterval    = TerrGuardIntervals((eTerrGuardIntervals) sd->getGuardInterval());
           int TransmissionMode = TerrTransmissionModes((eTerrTransmissionModes) sd->getTransmissionMode());
           for (int n = 0; n < NumFrequencies; n++) {
             cChannel * transponder = new cChannel;
             transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
             if (SetTerrTransponderDataFromDVB(transponder, Source, Frequencies[n], Bandwidth, Constellation, Hierarchy, CodeRateHP, CodeRateLP, GuardInterval, TransmissionMode, CableTerrInversions(eInversionAuto))) {
               if (!is_known_initial_transponder(transponder, true)) {
                 dlog(3, "   Add: %s -> NID = %d, TID = %d", *PrintTransponder(transponder), transponder->Nid(), transponder->Tid());
                 NewTransponders.Add(transponder);
                 nextTransponders = NewTransponders.Count() - ScannedTransponders.Count();
                 }
               else {
                 // we already know this transponder, lets update these channels
                 cChannel * update_transponder = ScannedTransponders.GetByParams(transponder);
                 if ((tableId == TABLE_ID_NIT_ACTUAL) && (update_transponder != NULL)) {
                   // only NIT_actual should update existing channels
                   if (is_different_transponder_deep_scan(transponder, update_transponder, false)) {
                     SetTerrTransponderDataFromDVB(update_transponder, Source, Frequencies[n], Bandwidth, Constellation, Hierarchy, CodeRateHP, CodeRateLP, GuardInterval, TransmissionMode, 999);
                     dlog(2, "   Upd: %s", *PrintTransponder(update_transponder));
                     }
                   if ((ts.getOriginalNetworkId() != update_transponder->Nid()) ||
                       (ts.getTransportStreamId() != update_transponder->Tid())) {
                     update_transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
                     dlog(2, "   Upd: %s -> NID = %d, TID = %d", *PrintTransponder(update_transponder), update_transponder->Nid(), update_transponder->Tid());
                     }
                   }
                 DELETENULL(transponder);
                 }
               }
             }
///
 //         for (int n = 0; n < NumCellFrequencies; n++) {
 //           cChannel * transponder = new cChannel;
 //           transponder->SetId(ts.getOriginalNetworkId(), ts.getTransportStreamId(), 0, 0);
 //           if (SetTerrTransponderDataFromDVB(transponder, Source, cellFrequencies[n], Bandwidth, Constellation, Hierarchy, CodeRateHP, CodeRateLP, GuardInterval, TransmissionMode, CableTerrInversions(eInversionAuto))) {
 //             if (ScannedTransponders.IsUniqueTransponder(transponder)) {     //add only unknown transponders
 //               dlog(3, "   Add: %s", *PrintTransponder(transponder));
 //               NewTransponders.Add(transponder);
 //               }
 //             else {
 //               DELETENULL(transponder);
 //               }
 //             }
 //           }
 ///
           }
           break;
         case SI::ServiceListDescriptorTag: {
           SI::ServiceListDescriptor *        sd = (SI::ServiceListDescriptor *) d;
           SI::ServiceListDescriptor::Service Service;
           for (SI::Loop::Iterator it; sd->serviceLoop.getNext(Service, it);) {
             if (Service.getServiceType() == 0x04 ||            //NVOD reference service
                 Service.getServiceType() == 0x18 ||            //advanced codec SD NVOD reference service
                 Service.getServiceType() == 0x1B) {            //advanced codec HD NVOD reference service
               continue;
               }
             if (Service.getServiceType() == 0x03 ||            //Teletext service
                 Service.getServiceType() == 0x04 ||            //NVOD reference service (see note 1)
                 Service.getServiceType() == 0x05 ||            //NVOD time-shifted service (see note 1)
                 Service.getServiceType() == 0x06 ||            //mosaic service
                 Service.getServiceType() == 0x0C ||            //data broadcast service
                 Service.getServiceType() == 0x0D ||            //reserved for Common Interface Usage (EN 50221 [39])
                 Service.getServiceType() == 0x0E ||            //RCS Map (see EN 301 790 [7])
                 Service.getServiceType() == 0x0F ||            //RCS FLS (see EN 301 790 [7])
                 Service.getServiceType() == 0x10 ||            //DVB MHP service
                 Service.getServiceType() == 0x17 ||            //advanced codec SD NVOD time-shifted service
                 Service.getServiceType() == 0x18 ||            //advanced codec SD NVOD reference service
                 Service.getServiceType() == 0x1A ||            //advanced codec HD NVOD time-shifted service
                 Service.getServiceType() == 0x1B) {            //advanced codec HD NVOD reference service
               cChannel * ch = NewChannels.GetByServiceID(Source(), Transponder(), Service.getServiceId());
               if (ch && !(ch->Rid() & INVALID_CHANNEL)) {
                 dlog(0, "   NIT: invalid (service_type=0x%.2x): %s", Service.getServiceType(), *PrintChannel(ch));
                 ch->SetId(ch->Nid(), ch->Tid(), ch->Sid(), INVALID_CHANNEL);
                 }
               }
             }
           }
           break;
         case SI::CellFrequencyLinkDescriptorTag: break;         //not implemented in libsi
         case SI::FrequencyListDescriptorTag: break;             //already handled
         case SI::PrivateDataSpecifierDescriptorTag: break;      //not usable
         case 0x80 ... 0xFE: break;                              //user defined 
         default: dlog(0, "   NIT: unknown descriptor tag 0x%.2x", d->getDescriptorTag());
        }
      DELETENULL(d);
      }
    }
  }

void cNitScanner::Action(void) {
  int count = 0;

  while (Running() && active) {
    cCondWait::SleepMs(10);
    if (count++ > 1200) {     // 1200 x 10msec = 12sec
      active = false;
      }
    }
  active = false;
  Del(PID_NIT, tableId);
  Cancel();
  }

cNitScanner::~cNitScanner() {
  while (active) cCondWait::SleepMs(50);
  dlog(4, "   ~cNITscanner");
  Cancel(10);
  }

cChannel * GetByTransponder(const cChannel * Transponder) {
  int maxdelta = 2001;

  if (Transponder->IsSat()) maxdelta = 2;

  if (NewChannels.Count()) {
    for (cChannel * ch = NewChannels.First(); ch; ch = NewChannels.Next(ch)) {
      if (is_nearly_same_frequency(ch, Transponder, maxdelta) &&
          ch->Source() == Transponder->Source() &&
          ch->Tid() == Transponder->Tid() &&
          ch->Sid() == Transponder->Sid()) {
        dlog(4, "   GetByTransponder: known channel %s", *PrintChannel(Transponder));
        return (ch);
        }
      }
    }
  return (NULL);
  }

//--------cPatScanner-------------------------------------------------------------------------

cPatScanner::cPatScanner(cDevice * Parent) {
  parent = Parent;
  active = true;
  memset(&cPmtScanners, 0, sizeof(cPmtScanners));
  Set(PID_PAT, TABLE_ID_PAT);
  Start();
  }

cPatScanner::~cPatScanner() {
  active = false;
  for (int i = 0; i < MAX_PMTS; i++)
    if (cPmtScanners[i]) {
      parent->Detach(cPmtScanners[i]);
      }
  Cancel();
  }

void cPatScanner::Process(u_short Pid, u_char Tid, const u_char * Data, int Length) {
  if (!active) {
    return;
    }
  SI::PAT tsPAT(Data, false);
  if (!tsPAT.CheckCRCAndParse()) {
    return;
    }

  HEXDUMP(Data, Length);

  SI::PAT::Association assoc;
  for (SI::Loop::Iterator it; tsPAT.associationLoop.getNext(assoc, it);) {
    if (!assoc.getServiceId()) {
      continue;
      }

    cChannel * ch;
    cChannel * scanned = ScannedTransponders.GetByParams(Channel());
    if (scanned != NULL) {
      ch = new cChannel(* scanned);
      ch->SetId(scanned->Nid(), tsPAT.getTransportStreamId(), assoc.getServiceId());
      }
    else {
      ch = new cChannel();
      ch->CopyTransponderData(Channel());
      ch->SetId(0, tsPAT.getTransportStreamId(), assoc.getServiceId());
      }
    ch->SetName("???", "", "");
    if (!GetByTransponder(ch)) {
      NewChannels.Lock(true, 100);
      NewChannels.Add(ch);
      NewChannels.Unlock();
      dlog(1, "      Add: %s", *PrintChannel(ch));
      for (int i = 0; i < MAX_PMTS; i++) {
        if (!cPmtScanners[i]) {
          cPmtScanners[i] = new cPmtScanner(ch, assoc.getServiceId(), assoc.getPid());
          parent->AttachFilter(cPmtScanners[i]);
          break;
          }
        }
      }
    else {
      free(ch);
      }
    }
  }

void cPatScanner::Action(void) {
  int count = 0;

  while (Running() && active) {
    cCondWait::SleepMs(10);
    if (count++ > 100) { //1sec
      active = false;
      break;
      }
    }
  Del(PID_PAT, TABLE_ID_PAT);
  }

//--------cPmtScanner-------------------------------------------------------------------------
// basically this is cPatFilter from vdr/pat.{h,c} with some changes in Process()

cPmtScanner::cPmtScanner(cChannel * channel, u_short Sid, u_short PmtPid) {
  pmtPid  = PmtPid;
  pmtSid  = Sid;
  Channel = channel;
  Set(pmtPid, TABLE_ID_PMT);     // PMT
  }

void cPmtScanner::Process(u_short Pid, u_char Tid, const u_char * Data, int Length) {
  SI::PMT pmt(Data, false);
  if (!pmt.CheckCRCAndParse() ||
      (pmt.getServiceId() != pmtSid)) {
    return;
    }
  HEXDUMP(Data, Length);

  if (Channel->Sid() != pmtSid) {
    dlog(0, "ERROR: Channel->Sid(%d) != pmtSid(%d)", Channel->Sid(), pmtSid);
    return;
    }
  SI::CaDescriptor * d;
  cCaDescriptors *   CaDescriptors = new cCaDescriptors(Channel->Source(), Channel->Transponder(), Channel->Sid());
  // Scan the common loop:
  for (SI::Loop::Iterator it; (d = (SI::CaDescriptor *) pmt.commonDescriptors.getNext(it, SI::CaDescriptorTag));) {
    CaDescriptors->AddCaDescriptor(d, false);
    DELETENULL(d);
    }
  // Scan the stream-specific loop:
  SI::PMT::Stream stream;
  int             Vpid                           = 0;
  int             Vtype                          = 0;
  int             Ppid                           = 0;
  int             StreamType                     = STREAMTYPE_UNDEFINED;
  int             Apids[MAXAPIDS + 1]            = {0}; // these lists are zero-terminated
  int             Atypes[MAXAPIDS + 1]           = {0};
  int             Dpids[MAXDPIDS + 1]            = {0};
  int             Dtypes[MAXDPIDS + 1]           = {0};
  int             Spids[MAXSPIDS + 1]            = {0};
  char            ALangs[MAXAPIDS][MAXLANGCODE2] = {""};
  char            DLangs[MAXDPIDS][MAXLANGCODE2] = {""};
  char            SLangs[MAXSPIDS][MAXLANGCODE2] = {""};
  int             Tpid                           = 0;
  int             NumApids                       = 0;
  int             NumDpids                       = 0;
  int             NumSpids                       = 0;

  for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it);) {
    StreamType = stream.getStreamType();
    switch (StreamType) {
       case STREAMTYPE_11172_VIDEO:
       case STREAMTYPE_13818_VIDEO:
       case STREAMTYPE_14496_H264_VIDEO:
         Vpid  = stream.getPid();
         Vtype = StreamType;
         Ppid  = pmt.getPCRPid();
         break;
       case STREAMTYPE_11172_AUDIO:
       case STREAMTYPE_13818_AUDIO:
       case STREAMTYPE_13818_AUDIO_ADTS: 
       case STREAMTYPE_14496_AUDIO_LATM:
         {
         if (NumApids < MAXAPIDS) {
           Apids[NumApids] = stream.getPid();
           Atypes[NumApids] = stream.getStreamType();
           SI::Descriptor * d;
           for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it));) {
             switch (d->getDescriptorTag()) {
                case SI::ISO639LanguageDescriptorTag: {
                  SI::ISO639LanguageDescriptor *         ld = (SI::ISO639LanguageDescriptor *) d;
                  SI::ISO639LanguageDescriptor::Language l;
                  char * s = ALangs[NumApids];
                  int    n = 0;
                  for (SI::Loop::Iterator it; ld->languageLoop.getNext(l, it);) {
                    if (*ld->languageCode != '-') {                 // some use "---" to indicate "none"
                      if (n > 0) {
                        *s++ = '+';
                        }
                      strn0cpy(s, I18nNormalizeLanguageCode(l.languageCode), MAXLANGCODE1);
                      s += strlen(s);
                      if (n++ > 1) {
                        break;
                        }
                      }
                    }
                  }
                  break;
                default:;
               }
             DELETENULL(d);
             }
           NumApids++;
           }
         }
         break;
       case STREAMTYPE_13818_PRIVATE:
       case STREAMTYPE_13818_PES_PRIVATE: {
         int              dpid = 0;
         int              dtype = 0;
         char             lang[MAXLANGCODE1] = {0};
         SI::Descriptor * d;
         for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it));) {
           switch (d->getDescriptorTag()) {
              case SI::AC3DescriptorTag:
              case EnhancedAC3DescriptorTag:
              case DTSDescriptorTag:
              case AACDescriptorTag:
                dpid = stream.getPid();
                dtype = d->getDescriptorTag();
                break;
              case SI::SubtitlingDescriptorTag:
                if (NumSpids < MAXSPIDS) {
                  Spids[NumSpids] = stream.getPid();
                  SI::SubtitlingDescriptor * sd = (SI::SubtitlingDescriptor *) d;
                  SI::SubtitlingDescriptor::Subtitling sub;
                  char * s = SLangs[NumSpids];
                  int    n = 0;
                  for (SI::Loop::Iterator it; sd->subtitlingLoop.getNext(sub, it);) {
                    if (sub.languageCode[0]) {
                      if (n > 0) {
                        *s++ = '+';
                        }
                      strn0cpy(s, I18nNormalizeLanguageCode(sub.languageCode), MAXLANGCODE1);
                      s += strlen(s);
                      if (n++ > 1) {
                        break;
                        }
                      }
                    }
                  NumSpids++;
                  }
                break;
              case SI::TeletextDescriptorTag:
                Tpid = stream.getPid();
                break;
              case SI::ISO639LanguageDescriptorTag: {
                SI::ISO639LanguageDescriptor * ld = (SI::ISO639LanguageDescriptor *) d;
                strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                }
                break;
              default:;
             }
           DELETENULL(d);
           }
         if (dpid) {
           if (NumDpids < MAXDPIDS) {
             Dpids[NumDpids] = dpid;
             Dtypes[NumDpids] = dtype;
             strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
             NumDpids++;
             }
           }
         }
         break;
       #if VDRVERSNUM >= 10714 /* atsc support in 1.7.14 */
       case STREAMTYPE_13818_USR_PRIVATE_81: {
         // ATSC AC-3; kls && Alex Lasnier
         if (Channel->IsAtsc()) {
             char dlang[MAXLANGCODE1] = { 0 };
             SI::Descriptor *d;
             for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                 switch (d->getDescriptorTag()) {
                      case SI::ISO639LanguageDescriptorTag: {
                           SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                               strn0cpy(dlang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                               }
                           break;
                      default: ;
                      }
                 DELETENULL(d);
                 }
             if (NumDpids < MAXDPIDS) {
                 Dpids[NumDpids] = stream.getPid();
                 Dtypes[NumDpids] = SI::AC3DescriptorTag;
                 strn0cpy(DLangs[NumDpids], dlang, MAXLANGCODE1);
                 NumDpids++;
                 }
             }
         }
         break;
       #endif /* atsc support in 1.7.14 */
       default:;
       }
    for (SI::Loop::Iterator it; (d = (SI::CaDescriptor *) stream.streamDescriptors.getNext(it, SI::CaDescriptorTag));) {
      CaDescriptors->AddCaDescriptor(d, true);
      DELETENULL(d);
      }
    }
  if (Vpid || Apids[0] || Dpids[0] || Tpid) {
    Del(pmtPid, TABLE_ID_PMT);
    dlog(4, "      Upd: old  %s", *PrintChannel(Channel));
    SetPids(Channel, Vpid, Vpid ? Ppid : 0, Vpid ? Vtype : 0, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
    Channel->SetCaIds(CaDescriptors->CaIds());
    Channel->SetCaDescriptors(CaDescriptorHandler.AddCaDescriptors(CaDescriptors));
    dlog(2, "      Upd: %s", *PrintChannel(Channel));
    }
  else {
    Del(pmtPid, TABLE_ID_PMT);
    dlog(4, "   PMT: PmtPid=%.5d Sid=%d is invalid (no audio/video)", Pid, pmt.getServiceId());
    Channel->SetId(Channel->Nid(), Channel->Tid(), Channel->Sid(), INVALID_CHANNEL);
    }
  active = false;
  return;
  }

// --- cSdtScanner ------------------------------------------------------------

cSdtScanner::cSdtScanner(int TableId) {
  active  = true;
  tableId = TableId;
  sectionSyncer.Reset();
  Set(PID_SDT, tableId, 0xFF);     // SDT
  Start();
  }

void cSdtScanner::Process(u_short Pid, u_char Tid, const u_char * Data, int Length) {
  if (!(Source() && Transponder())) {
    return;
    }
  SI::SDT sdt(Data, false);
  if (!sdt.CheckCRCAndParse()) {
    return;
    }
  if (!sectionSyncer.Sync(sdt.getVersionNumber(), sdt.getSectionNumber(), sdt.getLastSectionNumber())) {
    return;
    }
  dlog(2, "   Transponder %d", Transponder());

  HEXDUMP(Data, Length);

  SI::SDT::Service SiSdtService;
  for (SI::Loop::Iterator it; sdt.serviceLoop.getNext(SiSdtService, it);) {
    cChannel * channel = NewChannels.First();
    while (channel) {
      if (channel->Source() == Source() &&
          channel->Tid() == sdt.getTransportStreamId() &&
          channel->Sid() == SiSdtService.getServiceId()) {
        break;
        }
      channel = NewChannels.Next(channel);
      }
    if (!channel ||
        channel->Source() != Source() ||
        channel->Tid() != sdt.getTransportStreamId() ||
        channel->Sid() != SiSdtService.getServiceId()) {
      channel = new cChannel;
      channel->CopyTransponderData(Channel());
      if (!is_known_initial_transponder(channel, true)) {
        cChannel * transponder = new cChannel();
        transponder->CopyTransponderData(Channel());
        dlog(3, "   SDT: Add: %s", *PrintTransponder(transponder));
        NewTransponders.Add(transponder);
        nextTransponders = NewTransponders.Count() - ScannedTransponders.Count();
        }
      channel->SetId(sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
      channel->SetName("???", "", "");
      dlog(0, "   SDT: Add: %s", *PrintChannel(channel));
      NewChannels.Add(channel);
      continue;
      }
    else {
      channel->SetId(sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
      }
    SI::Descriptor * d;
    for (SI::Loop::Iterator it2; (d = SiSdtService.serviceDescriptors.getNext(it2));) {
      switch ((unsigned) d->getDescriptorTag()) {
         case SI::ServiceDescriptorTag: {
           SI::ServiceDescriptor * sd = (SI::ServiceDescriptor *) d;
           switch (sd->getServiceType()) {
                              //---television---
              case digital_television_service:
              case MPEG2_HD_digital_television_service:
              case advanced_codec_SD_digital_television_service:
              case advanced_codec_HD_digital_television_service:
                              //---radio---
              case digital_radio_sound_service:
              case advanced_codec_digital_radio_sound_service:
                              //---references---
              case digital_television_NVOD_reference_service:
              case advanced_codec_SD_NVOD_reference_service:
              case advanced_codec_HD_NVOD_reference_service:
                              //---time shifted services---
              case digital_television_NVOD_timeshifted_service:
              case advanced_codec_SD_NVOD_timeshifted_service:
              case advanced_codec_HD_NVOD_timeshifted_service:
                {
                #ifndef Utf8BufSize
                #define Utf8BufSize(s) ((s) * 4)
                #endif

                char   NameBuf[Utf8BufSize(1024)];
                char   ShortNameBuf[Utf8BufSize(1024)];
                char   ProviderNameBuf[Utf8BufSize(1024)];
                sd->serviceName.getText(NameBuf, ShortNameBuf, sizeof(NameBuf), sizeof(ShortNameBuf));
                char * pn = compactspace(NameBuf);
                char * ps = compactspace(ShortNameBuf);
                if (!*ps && cSource::IsCable(Source())) {
                                                  // Some cable providers don't mark short channel names according to the
                                                  // standard, but rather go their own way and use "name>short name" or
                                                  // "name, short name":
                  char * p = strchr(pn, '>');     // fix for UPC Wien
                  if (!p) {
                    p = strchr(pn, ',');          // fix for "Kabel Deutschland"
                    }
                  if (p && p > pn) {
                    *p++ = 0;
                    strcpy(ShortNameBuf, skipspace(p));
                    }
                  }
                sd->providerName.getText(ProviderNameBuf, sizeof(ProviderNameBuf));
                char * pp = compactspace(ProviderNameBuf);
                if (channel) {
                  if (0 == strcmp("???", channel->Name())) {
                    dlog(4, "      SDT: old %s", *PrintChannel(channel));
                    channel->SetName(pn, ps, pp);
                    dlog(2, "      Upd: %s", *PrintChannel(channel));
                    }
                  }
                else {
                  channel = NewChannels.NewChannel(Channel(), pn, ps, pp, sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
                  if (!is_known_initial_transponder(channel, true)) {
                    cChannel * transponder = new cChannel();
                    transponder->CopyTransponderData(Channel());
                    dlog(3, "   SDT: Add: %s", *PrintTransponder(transponder));
                    NewTransponders.Add(transponder);
                    nextTransponders = NewTransponders.Count() - ScannedTransponders.Count();
                    }
                  dlog(2, "   SDT: Add %s", *PrintChannel(channel));
                  }
                }
             default:;
             }
           }
           break;
         case SI::NVODReferenceDescriptorTag:
         case SI::BouquetNameDescriptorTag:
         case SI::MultilingualServiceNameDescriptorTag:
         case SI::ServiceIdentifierDescriptorTag:
         case SI::ServiceAvailabilityDescriptorTag:
         case SI::DefaultAuthorityDescriptorTag:
         case SI::AnnouncementSupportDescriptorTag:
         case SI::DataBroadcastDescriptorTag:
         case SI::TelephoneDescriptorTag:
         case SI::CaIdentifierDescriptorTag:
         case SI::PrivateDataSpecifierDescriptorTag:
         case SI::ContentDescriptorTag:
         case 0x80 ... 0xFE: // user defined //
           break;
         default: dlog(0, "SDT: unknown descriptor 0x%.2x", d->getDescriptorTag());
        }
      DELETENULL(d);
      }
    }
  }

void cSdtScanner::Action(void) {
  int count = 0;

  while (Running() && active) {
    cCondWait::SleepMs(10);
    if (count++ > 400) { //2sec
      active = false;
      break;
      }
    }
  active = false;
  Del(PID_SDT, tableId);
  Cancel(1);
  }

cSdtScanner::~cSdtScanner() {}

int AddChannels() {
  int count = 0;

  Channels.IncBeingEdited();

  for (cChannel * Channel = NewChannels.First(); Channel; Channel = NewChannels.Next(Channel)) {
    if (! Channel->Vpid() && ! Channel->Apid(0) && ! Channel->Dpid(0) && 
        ! Channel->Tpid() && ! Channel->Ca() &&
        ! strncasecmp(Channel->Name(),"???",3)) {
      dlog(3,"      skipped service %s", *PrintChannel(Channel));
      continue;
      }
    if (! Channels.HasUniqueChannelID(Channel)) {
      cChannel * ExistingChannel = Channels.GetByChannelID(Channel->GetChannelID(), false, false);
      if (ExistingChannel) {
         int i;
         char ALangs[MAXAPIDS][MAXLANGCODE2] = { "" };
         char DLangs[MAXDPIDS][MAXLANGCODE2] = { "" };
         char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
         int  Atypes[MAXAPIDS + 1]           = {0};
         int  Dtypes[MAXDPIDS + 1]           = {0};

         for (i = 0; i < MAXAPIDS; i++) {
            int len = strlen(Channel->Alang(i));
            if (len < 1) break;
            strncpy (ALangs[i], Channel->Alang(i), min(len,MAXLANGCODE2));
            }
         for (i = 0; i < MAXAPIDS; i++) {
            int len = strlen(Channel->Dlang(i));
            if (len < 1) break;
            strncpy (DLangs[i], Channel->Dlang(i), min(len,MAXLANGCODE2));
            }
         for (i = 0; i < MAXAPIDS; i++) {
            int len = strlen(Channel->Slang(i));
            if (len < 1) break;
            strncpy (SLangs[i], Channel->Slang(i), min(len,MAXLANGCODE2));
            }
         #if VDRVERSNUM > 10714
         for (i = 0; i < MAXAPIDS; i++) {
            Atypes[i] = Channel->Atype(i);
            }
         for (i = 0; i < MAXDPIDS; i++) {
            Dtypes[i] = Channel->Dtype(i);
            }
         #endif
         if (Channel->Vpid() || Channel->Apid(0) || Channel->Dpid(0))
            SetPids(ExistingChannel, Channel->Vpid(), Channel->Ppid(),
                    #if VDRVERSNUM > 10700
                    Channel->Vtype(),
                    #else
                    2,
                    #endif
                    (int *) Channel->Apids(), Atypes, ALangs, (int *) Channel->Dpids(), Dtypes, DLangs,
                    #if VDRVERSNUM >= 10600
                    (int *) Channel->Spids(), SLangs,
                    #else
                    NULL, NULL,
                    #endif
                    Channel->Tpid());
         if (strcmp("???", Channel->Name()))
            ExistingChannel->SetName(Channel->Name(), Channel->ShortName(), Channel->Provider());
         dlog(3,"      updated (existing): %s", *PrintChannel(ExistingChannel));
         }
      else
         dlog(3,"      skipped (existing): %s", *PrintChannel(Channel));
      continue;
      }
    if (Channel->Ca() && ! (wSetup.scanflags & SCAN_SCRAMBLED)) {
      dlog(3,"      skipped (no scrambled channels) %s", *PrintChannel(Channel));
      continue;
      }
    if (! Channel->Ca() && ! (wSetup.scanflags & SCAN_FTA)) {
      dlog(3,"      skipped (no FTA channels) %s", *PrintChannel(Channel));
      continue;
      }
    if (! Channel->Vpid() && (Channel->Apid(0) || Channel->Dpid(0)) && ! (wSetup.scanflags & SCAN_RADIO)) {
      dlog(3,"      skipped (no radio channels) %s", *PrintChannel(Channel));
      continue;
      }
    if (Channel->Vpid() && ! (wSetup.scanflags & SCAN_TV)) {
      dlog(3,"      skipped (no tv channels) %s", *PrintChannel(Channel));
      continue;
      }
    #if VDRVERSNUM > 10701
    if ((Channel->Vtype() > 2) && ! (wSetup.scanflags & SCAN_HD)) {
      dlog(3,"      skipped (no hdtv channels) %s", *PrintChannel(Channel));
      continue;
      }
    #endif
    cChannel * aChannel = new cChannel(* Channel);
    Channels.Add(aChannel);
    count++;
    }
  NewChannels.Load(NULL, false, false);
  Channels.DecBeingEdited();
  Channels.ReNumber();
  Channels.SetModified(true);
  return (count);
  }


// --- cEitParser ------------------------------------------------------------------

class cEitParser : public SI::EIT {
public:
  cEitParser(int Source, u_char Tid, const u_char *Data);
  };

cEitParser::cEitParser(int Source, u_char Tid, const u_char *Data)
:SI::EIT(Data, false)
{
  if (!CheckCRCAndParse())
     return;

  tChannelID channelID(Source, getOriginalNetworkId(), getTransportStreamId(), getServiceId());
  cChannel *channel = Channels.GetByChannelID(channelID, true);
  if (!channel) {
     return; // only collect data for known channels
     }

  SI::EIT::Event SiEitEvent;
  for (SI::Loop::Iterator it; eventLoop.getNext(SiEitEvent, it); ) {
      bool ExternalData = false;
      // Drop bogus events - but keep NVOD reference events, where all bits of the start time field are set to 1, resulting in a negative number.
      if ((SiEitEvent.getStartTime() == 0) || (SiEitEvent.getStartTime() > 0 && SiEitEvent.getDuration() == 0))
         continue;

      SI::Descriptor *d;
      cLinkChannels *LinkChannels = NULL;

      for (SI::Loop::Iterator it2; (d = SiEitEvent.eventDescriptors.getNext(it2)); ) {
          if (ExternalData && d->getDescriptorTag() != SI::ComponentDescriptorTag) {
             DELETENULL(d);
             continue;
             }
          switch (d->getDescriptorTag()) {
            case SI::LinkageDescriptorTag: {
                 dlog(4, "LinkageDescriptorTag @ %s", *PrintChannel(channel));
                 SI::LinkageDescriptor *ld = (SI::LinkageDescriptor *)d;
                 tChannelID linkID(Source, ld->getOriginalNetworkId(), ld->getTransportStreamId(), ld->getServiceId());
                 if (ld->getLinkageType() == 0xB0) { // Premiere World                    
                    time_t now = time(NULL);
                    bool hit = SiEitEvent.getStartTime() <= now && now < SiEitEvent.getStartTime() + SiEitEvent.getDuration();
                    if (hit) {
                       char linkName[ld->privateData.getLength() + 1];
                       strn0cpy(linkName, (const char *)ld->privateData.getData(), sizeof(linkName));
                       cChannel *link = Channels.GetByChannelID(linkID);
                       if (link != channel) { // only link to other channels, not the same one
                          if (link) {
                             link->SetName(linkName, "", "");
                             }
                          else {
                             link = Channels.NewChannel(channel, linkName, "", "", ld->getOriginalNetworkId(), ld->getTransportStreamId(), ld->getServiceId());
                             }
                          if (link) {
                             if (!LinkChannels)
                                LinkChannels = new cLinkChannels;
                             LinkChannels->Add(new cLinkChannel(link));
                             }
                          }
                       else
                          channel->SetPortalName(linkName);
                       }
                    }
                 }
                 break;
            default: ;
            }
          DELETENULL(d);
          }

      if (LinkChannels)
         channel->SetLinkChannels(LinkChannels);
      }
}



// --- cEitScanner ------------------------------------------------------------

cEitScanner::cEitScanner(void) {
  active  = true;
  Set(PID_EIT, TABLE_ID_EIT_ACTUAL_PRESENT,        0xFE);  // actual(0x4E)/other(0x4F) TS, present/following
  Set(PID_EIT, TABLE_ID_EIT_ACTUAL_SCHEDULE_START, 0xF0);  // actual TS, schedule(0x50)/schedule for future days(0x5X)
  Set(PID_EIT, TABLE_ID_EIT_OTHER_SCHEDULE_START,  0xF0);  // other  TS, schedule(0x60)/schedule for future days(0x6X)
  Start();
}

void cEitScanner::Process(u_short Pid, u_char Tid, const u_char *Data, int Length) {
  cEitParser EitParser(Source(), Tid, Data);
}

void cEitScanner::Action(void) {
  int count = 0;

  while (Running() && active) {
    cCondWait::SleepMs(10);
    if (count++ > 1000) { //10sec
      active = false;
      break;
      }
    }
  active = false;
  Del(PID_EIT, TABLE_ID_EIT_ACTUAL_PRESENT);
  Del(PID_EIT, TABLE_ID_EIT_ACTUAL_SCHEDULE_START);
  Del(PID_EIT, TABLE_ID_EIT_OTHER_SCHEDULE_START);
  Cancel(1);
  }

cEitScanner::~cEitScanner() {}
