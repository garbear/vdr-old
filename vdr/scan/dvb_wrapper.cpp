/*
 * dvb_wrapper.c: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * wirbelscan-0.0.5
 *
 * $Id$ v20101210
 */

#include <linux/dvb/frontend.h> //either API version 3.2 or 5.0
#include <linux/dvb/version.h>
#include <vdr/config.h>
#include <vdr/dvbdevice.h>
#include <vdr/channels.h>
#include <vdr/timers.h>
#include <vdr/diseqc.h>
#include "common.h"
#include "dvb_wrapper.h"
#include <ctype.h>

#if (DVB_API_VERSION < 5)
  #if ((DVB_API_VERSION == 3) && (DVB_API_VERSION_MINOR == 3))
     #error "*************************************************"
     #error "YOUR DVB DRIVER IS TOO OLD; OLD MULTIPROTO DVB DRIVERS ARE UNSUPPORTED. PLEASE UPGRADE"
     #error "*************************************************"
  #else
     #warning "*************************************************"
     #warning "YOUR DVB DRIVER IS MUCH TOO OLD - SOME FEATURES WILL BE DISABLED. PLEASE UPGRADE AND RECOMPILE PLUGIN."
     #warning "*************************************************"
  #endif
#endif

fe_code_rate_t CableSatCodeRates(eCableSatCodeRates cr) {
  switch(cr) {
    case eCoderateNone: return FEC_NONE;
    case eCoderate12  : return FEC_1_2;
    case eCoderate23  : return FEC_2_3;
    case eCoderate34  : return FEC_3_4;
    case eCoderate56  : return FEC_5_6;
    case eCoderate78  : return FEC_7_8;
    case eCoderate89  : return FEC_8_9;
    case eCoderateAuto: return FEC_AUTO;
    #if (DVB_API_VERSION >= 5)
    case eCoderate35  : return FEC_3_5;
    case eCoderate45  : return FEC_4_5;
    case eCoderate910 : return FEC_9_10;
    #endif
    default           : dlog(0, "%s, unknown coderate %u",
                           __FUNCTION__, cr);
                        return FEC_AUTO;
    }
}

fe_modulation_t CableModulations(eCableModulations cm) {
  switch(cm) {
    case eModulationQam16   : return QAM_16;
    case eModulationQam32   : return QAM_32;
    case eModulationQam64   : return QAM_64;
    case eModulationQam128  : return QAM_128;
    case eModulationQam256  : return QAM_256;
    case eModulationQamAuto : return QAM_AUTO;
    #if 0
    case eModulationQam512  : return QAM_512;
    case eModulationQam1024 : return QAM_1024;
    #endif
    default                 : dlog(0, "%s, unknown modulation %u",
                                 __FUNCTION__, cm);
                              return QAM_AUTO;
    }
}

fe_modulation_t AtscModulations(eAtscModulations am) {
  switch(am) {
    case eVsb8     : return VSB_8;
    case eVsb16    : return VSB_16;
    case eQam64    : return QAM_64;
    case eQam256   : return QAM_256;
    case eQamAuto  : return QAM_AUTO;
    default        : dlog(0, "%s, unknown modulation %u",
                        __FUNCTION__, am);
                     return QAM_AUTO;
    }
}

fe_polarization_t SatPolarizations(eSatPolarizations sp) {
  switch(sp) {
    case eHorizontal  : return POLARIZATION_HORIZONTAL;
    case eVertical    : return POLARIZATION_VERTICAL;
    case eLeft        : return POLARIZATION_CIRCULAR_LEFT;
    case eRight       : return POLARIZATION_CIRCULAR_RIGHT;
    default           : dlog(0, "%s, unknown polarization %u",
                           __FUNCTION__, sp);
                        return POLARIZATION_HORIZONTAL;
    }
}

fe_rolloff_t SatRollOffs(eSatRollOffs ro) {
#if (DVB_API_VERSION >= 5)
  switch(ro) {
    case eRolloff35   : return ROLLOFF_35;
    case eRolloff25   : return ROLLOFF_25;
    case eRolloff20   : return ROLLOFF_20;
    case eRolloffAuto : return ROLLOFF_35;
    default           : dlog(0, "%s, unknown rolloff %u",
                           __FUNCTION__, ro);
                        return ROLLOFF_35;
    }
#else
  return ROLLOFF_35;
#endif
}

fe_delivery_system_t SatSystems(eSatSystems ss) {
  switch(ss) {
    case eDvbs   : return SYS_DVBS;
#if (DVB_API_VERSION >= 5)
    case eDvbs2  : return SYS_DVBS2;
#endif
    default      : dlog(0, "%s, unsupported sat system %u",
                     __FUNCTION__, ss);
                   return SYS_DVBS;
    }
}

fe_modulation_t SatModulationTypes(eSatModulationTypes mt) {
  switch(mt) {
    case eSatModulationQpsk : return QPSK;
    case eSatModulationQam16: return QAM_16;
    #ifdef PSK_8
    case eSatModulation8psk : return PSK_8;
    #endif
    default                 : dlog(0, "%s, unknown modulation type %u",
                                __FUNCTION__, mt);
                              return QPSK;
    }
}

#if VDRVERSNUM >= 10700
int TerrBandwidths(eTerrBandwidths tb) {
  switch(tb) {
    case eBandwidth8Mhz  : return 8000000;
    case eBandwidth7Mhz  : return 7000000;
    case eBandwidth6Mhz  : return 6000000;
    case eBandwidth5Mhz  : return 5000000;
    default              : dlog(0, "%s, unknown bandwidth %u",
                                __FUNCTION__, tb);
                           return 8000000;
    }
}
#else
fe_bandwidth_t TerrBandwidths(eTerrBandwidths tb) {
  switch(tb) {
    case eBandwidth8Mhz  : return BANDWIDTH_8_MHZ;
    case eBandwidth7Mhz  : return BANDWIDTH_7_MHZ;
    case eBandwidth6Mhz  : return BANDWIDTH_6_MHZ;
    case eBandwidthAuto  : return BANDWIDTH_AUTO;
    default              : dlog(0, "%s, unknown bandwidth %u",
                                __FUNCTION__, tb);
                           return BANDWIDTH_AUTO;
    }
}
#endif


fe_modulation_t TerrConstellations(eTerrConstellations tc) {
  switch(tc) {
    case eModulationQpsk : return QPSK;
    case eModulation16   : return QAM_16;
    case eModulation64   : return QAM_64;
    case eModulationAuto : return QAM_AUTO;
    default              : dlog(0, "%s, unknown constellation %u",
                                __FUNCTION__, tc);
                           return QAM_AUTO;
    }
}

fe_hierarchy_t TerrHierarchies(eTerrHierarchies th) {
  switch(th) {
    case eHierarchyNone : return HIERARCHY_NONE;
    case eHierarchy1    : return HIERARCHY_1;
    case eHierarchy2    : return HIERARCHY_2;
    case eHierarchy4    : return HIERARCHY_4;
    case eHierarchyAuto : return HIERARCHY_AUTO;
    default             : dlog(0, "%s, unknown hierarchy %u",
                                __FUNCTION__, th);
                          return HIERARCHY_AUTO;
    }
}

fe_code_rate_t TerrCodeRates(eTerrCodeRates cr) {
  switch(cr) {
    case eTerrCoderateNone: return FEC_NONE;
    case eTerrCoderate12  : return FEC_1_2;
    case eTerrCoderate23  : return FEC_2_3;
    case eTerrCoderate34  : return FEC_3_4;
    case eTerrCoderate56  : return FEC_5_6;
    case eTerrCoderate78  : return FEC_7_8;
    case eTerrCoderateAuto: return FEC_AUTO;
    default               : dlog(0, "%s, unknown coderate %u",
                                __FUNCTION__, cr);
                            return FEC_AUTO;
    }
}

fe_guard_interval_t TerrGuardIntervals(eTerrGuardIntervals gi) {
  switch(gi) {
    case eGuardinterval32 : return GUARD_INTERVAL_1_32;
    case eGuardinterval16 : return GUARD_INTERVAL_1_16;
    case eGuardinterval8  : return GUARD_INTERVAL_1_8;
    case eGuardinterval4  : return GUARD_INTERVAL_1_4;
    case eGuardintervalAuto:return GUARD_INTERVAL_AUTO;
    default               : dlog(0, "%s, unknown guard interval %u",
                                __FUNCTION__, gi);
                            return GUARD_INTERVAL_AUTO;
    }
}

fe_transmit_mode_t TerrTransmissionModes (eTerrTransmissionModes tm) {
  switch(tm) {
    case eTransmissionmode2K  : return TRANSMISSION_MODE_2K;
    case eTransmissionmode8K  : return TRANSMISSION_MODE_8K;
    #ifdef TRANSMISSION_MODE_4K
    case eTransmissionmode4K  : return TRANSMISSION_MODE_4K;
    #endif
    case eTransmissionmodeAuto: return TRANSMISSION_MODE_AUTO;
    default                   : dlog(0, "%s, unknown transm mode %u",
                                __FUNCTION__, tm);
                                return TRANSMISSION_MODE_AUTO;
    }
}

fe_spectral_inversion_t CableTerrInversions(eCableTerrInversions in) {
  switch(in) {
    case eInversionOff : return INVERSION_OFF;
    case eInversionOn  : return INVERSION_ON;
    case eInversionAuto: return INVERSION_AUTO;
    default            : dlog(0, "%s, unknown inversion %u",
                            __FUNCTION__, in);
                         return INVERSION_AUTO;
    }
}

#if VDRVERSNUM >= 10713
cString ParamsToString(char Type, char Polarization, int Inversion, int Bandwidth, int CoderateH, int CoderateL, int Modulation, int System, int Transmission, int Guard, int Hierarchy, int RollOff) {
 cDvbTransponderParameters * p = new cDvbTransponderParameters;
 p->SetPolarization(Polarization);
 p->SetInversion(Inversion);
 p->SetBandwidth(Bandwidth);
 p->SetCoderateH(CoderateH);
 p->SetCoderateL(CoderateL);
 p->SetModulation(Modulation);
 p->SetSystem(System);
 p->SetTransmission(Transmission);
 p->SetGuard(Guard);
 p->SetHierarchy(Hierarchy);
 p->SetRollOff(RollOff);
 return p->ToString(Type);
}
#endif


bool SetSatTransponderDataFromDVB(cChannel * channel, int Source, int Frequency, char Polarization, int Srate, int CoderateH, int Modulation, int System, int RollOff) {
#if VDRVERSNUM >= 10700
  #if VDRVERSNUM >= 10713
  return channel->SetTransponderData(Source, Frequency, Srate, *ParamsToString('S', Polarization, 0, 0, CoderateH, 0, Modulation, System, 0, 0, 0, RollOff), true);
  #else
  return channel->SetSatTransponderData(Source, Frequency, Polarization, Srate, CoderateH, Modulation, System, RollOff);
  #endif
#else
  if ((System == 6)        || // SYS_DVBS2
      (CoderateH == 10)    || // FEC_3_5
      (CoderateH == 11)    || // FEC_9_10
      (RollOff == 1)       || // ROLLOFF_20
      (RollOff == 2)       || // ROLLOFF_25
      (Modulation > 8)) {     // PSK_8 .. DQPSK
        dlog(1, "DVB-S2 unsupported.");
        return false;
        }
  return channel->SetSatTransponderData(Source, Frequency, Polarization, Srate, CoderateH);
#endif
  }

#if VDRVERSNUM >= 10713
cDiseqc *GetDiseqc(int Source, int Frequency, char Polarization) {
  for (cDiseqc *p = Diseqcs.First(); p; p = Diseqcs.Next(p)) {
      if (p->Source() == Source && p->Slof() > Frequency && p->Polarization() == toupper(Polarization))
        return p;
      }
  return NULL;
}
#endif

bool ValidSatfreq(int f, const cChannel * Channel) {
  bool r = true;

  while (f > 999999) f /= 1000; 
  if (Setup.DiSEqC) {
     #if VDRVERSNUM >= 10713
     cDvbTransponderParameters * p = new cDvbTransponderParameters(Channel->Parameters());
     cDiseqc *diseqc = GetDiseqc(Channel->Source(), Channel->Frequency(), p->Polarization());
     free(p);
     #else
     cDiseqc *diseqc = Diseqcs.Get(Channel->Source(), Channel->Frequency(), Channel->Polarization());
     #endif
     if (diseqc)
        f -= diseqc->Lof();
     else {
        dlog(0, "no diseqc settings for %s", *PrintTransponder(Channel));
        return false;
        }
     }
  else {
     if (f < Setup.LnbSLOF)
        f -= Setup.LnbFrequLo;
     else
        f -= Setup.LnbFrequHi;
     }

  r = ((f >= 950) && (f <= 2150));
  if (!r)
     dlog(0, "transponder %s (freq %d -> out of tuning range)", *PrintTransponder(Channel), f);
  return r;
}

bool SetCableTransponderDataFromDVB(cChannel * channel, int Source, int Frequency, int Modulation, int Srate, int CoderateH, int Inversion) {
#if VDRVERSNUM >= 10713
 return channel->SetTransponderData(Source, Frequency, Srate, *ParamsToString('C', 'v', Inversion, 8000, CoderateH, 0, Modulation, SYS_DVBC_ANNEX_AC, 0, 0, 0, 0), true);
#else
 if (channel->Inversion() != Inversion) {// efforts because vdr doesnt support it. :(
    cChannel tmp;
    cString s = cString::sprintf("tmp:778:I%d:C:27500:"
               #if VDRVERSNUM < 10701
                "1:"
               #else
                "1=2:"    /* vdr 1.7.1 added Vtype */
               #endif
                "0:0:0:99999:87878:86868:565656", GetVDRInversionFromDVB((fe_spectral_inversion_t) Inversion));
    tmp.Parse(*s);
    channel->CopyTransponderData(&tmp);
    }
 return channel->SetCableTransponderData(Source, Frequency, Modulation, Srate, CoderateH);
#endif
}


bool SetTerrTransponderDataFromDVB(cChannel * channel, int Source, int Frequency, int Bandwidth, int Modulation, int Hierarchy, int CodeRateH, int CodeRateL, int Guard, int Transmission, int Inversion) {

#if DVB_API_VERSION > 3 
 switch (Bandwidth) {
    case BANDWIDTH_8_MHZ: Bandwidth=8000000; break;
    case BANDWIDTH_7_MHZ: Bandwidth=7000000; break;
    case BANDWIDTH_6_MHZ: Bandwidth=6000000; break;
    case BANDWIDTH_AUTO:  Bandwidth=8000000; /* oops.., what to do with auto?? */
    default:;
    }
#endif

#if VDRVERSNUM >= 10713
 return channel->SetTransponderData(Source, Frequency, 27500, *ParamsToString('T', 0, Inversion, Bandwidth, CodeRateH, CodeRateL, Modulation, SYS_DVBT, Transmission, Guard, Hierarchy, 0), true);
#else
  if (channel->Inversion() != Inversion) {// efforts because vdr doesnt support it. :(
    cChannel * tmp = new cChannel;
    cString s = cString::sprintf("tmp:778:I%d:T:27500:"
               #if VDRVERSNUM < 10701
                "1:"
               #else
                "1=2:" /* vdr 1.7.1 added Vtype */
               #endif
                "0:0:0:99999:87878:86868:565656", GetVDRInversionFromDVB((fe_spectral_inversion_t) Inversion));
    tmp->Parse(*s);
    channel->CopyTransponderData(tmp);
    }

 return channel->SetTerrTransponderData(Source, Frequency, Bandwidth, Modulation, Hierarchy, CodeRateH, CodeRateL, Guard, Transmission);
 #endif
}

void SetPids(cChannel * channel, int Vpid, int Ppid, int Vtype, int *Apids, int *Atypes, char ALangs[][MAXLANGCODE2], int *Dpids, int *Dtypes, char DLangs[][MAXLANGCODE2], int *Spids, char SLangs[][MAXLANGCODE2], int Tpid) {
 channel->SetPids(Vpid, Ppid,
                  #if VDRVERSNUM >= 10701 || defined EASYVDR80
                  Vtype, /* vdr 1.7.1 added Vtype */
                  #endif
                  Apids,
                  #if VDRVERSNUM > 10714
                  Atypes,
                  #endif
                  ALangs, Dpids,
                  #if VDRVERSNUM > 10714
                  Dtypes,
                  #endif
                  DLangs,
                  Spids, SLangs,
                  Tpid);
}
 
int GetVDRInversionFromDVB(fe_spectral_inversion_t Inversion) {
 switch (Inversion) {
   case INVERSION_OFF:  return 0;
   case INVERSION_ON:   return 1;
   case INVERSION_AUTO: return 999;
   default:             dlog(0, "%s, unknown inversion %d",
                         __FUNCTION__, Inversion);
                        return 999;
   } 
}

int GetVDRBandwidthFromDVB(fe_bandwidth_t Bandwidth) {
 switch (Bandwidth) {
   case BANDWIDTH_6_MHZ: return 6;
   case BANDWIDTH_7_MHZ: return 7;
   case BANDWIDTH_8_MHZ: return 8;
   case BANDWIDTH_AUTO:  return 999;
   default:              dlog(0, "%s, unknown Bandwidth %d",
                         __FUNCTION__, Bandwidth);
                         return 999;
   } 
}

int GetVDRBandwidthFromInt(int Bandwidth) {
 switch (Bandwidth) {
   case 6000000:         return 6;
   case 7000000:         return 7;
   case 8000000:         return 8;
   default:              dlog(0, "%s, unknown Bandwidth %d",
                         __FUNCTION__, Bandwidth);
                         return 999;
   } 
}

int GetVDRCoderateFromDVB(fe_code_rate_t Coderate) {
 switch (Coderate) {
   case FEC_NONE: return   0;
   case FEC_1_2:  return  12;
   case FEC_2_3:  return  23;
   case FEC_3_4:  return  34;
   case FEC_4_5:  return  45;
   case FEC_5_6:  return  56;
   case FEC_6_7:  return  67;
   case FEC_7_8:  return  78;
   case FEC_8_9:  return  89;
   case FEC_AUTO: return 999;
   #if VDRVERSNUM >= 10700
   case FEC_3_5:  return  35;
   case FEC_9_10: return 910;
   #endif
   default:       dlog(0, "%s, unknown Coderate %d",
                   __FUNCTION__, Coderate);
                  return 999;
   } 
}

int GetVDRModulationtypeFromDVB(fe_modulation_t Modulationtype) {
 switch (Modulationtype) {
   case QAM_16:   return   16;
   case QAM_32:   return   32;
   case QAM_64:   return   64;
   case QAM_128:  return  128;
   case QAM_256:  return  256;
#if VDRVERSNUM < 10700
   case QAM_AUTO: return  999;
   case QPSK:     return    0;
#else
   case QPSK:     return    2;
   #if 0
   case QAM_512:  return  512;
   case QAM_1024: return 1024;
   #endif
   case QAM_AUTO: return  998;
   case VSB_8:    return   10;
   case VSB_16:   return   11;
   #if (DVB_API_VERSION >= 5)   
   case PSK_8:    return    5;
   case APSK_16:  return    6;
   case APSK_32:  return    7;
   #endif
#endif
   default:       dlog(0, "%s, unknown Modulationtype %d",
                   __FUNCTION__, Modulationtype);
                  return  999;
   }
}

int GetVDRSatSystemFromDVB(fe_delivery_system_t SatSystem) {
 #if (DVB_API_VERSION >= 5)
 switch (SatSystem) {
   case SYS_DVBS2: return 1;
   case SYS_DVBS:  return 0;
   default:        dlog(0, "%s, unknown satsystem %d",
                   __FUNCTION__, SatSystem);
                   return 0;
   }
 #else
 return 0;
 #endif
}

char GetVDRPolarizationFromDVB(fe_polarization_t Polarization) {
 switch (Polarization) {
   case POLARIZATION_HORIZONTAL:     return 'h';
   case POLARIZATION_VERTICAL:       return 'v';
   case POLARIZATION_CIRCULAR_LEFT:  return 'l';
   case POLARIZATION_CIRCULAR_RIGHT: return 'r';
   default:                          dlog(0, "%s, unknown polarization %d",
                                        __FUNCTION__, Polarization);
                                     return 'h';
   }
}

int GetVDRTransmissionModeFromDVB(fe_transmit_mode_t TransmissionMode) {
 switch (TransmissionMode) {
   case TRANSMISSION_MODE_2K:   return   2;
   case TRANSMISSION_MODE_8K:   return   8;
   case TRANSMISSION_MODE_AUTO: return 999;
   default:                     dlog(0, "%s, unknown transmissionmode %d",
                                __FUNCTION__, TransmissionMode);
                                return 999;
   }
}

int GetVDRGuardFromDVB(fe_guard_interval_t Guard) {
 switch (Guard) {
   case GUARD_INTERVAL_1_4:  return   4;
   case GUARD_INTERVAL_1_8:  return   8;
   case GUARD_INTERVAL_1_16: return  16;
   case GUARD_INTERVAL_1_32: return  32;
   case GUARD_INTERVAL_AUTO: return 999;
   default:                  dlog(0, "%s, unknown guard %d",
                             __FUNCTION__, Guard);
                             return 999;
   }
}

int GetVDRHierarchyFromDVB(fe_hierarchy_t Hierarchy) {
 switch (Hierarchy) {
   case HIERARCHY_NONE: return   0;
   case HIERARCHY_1:    return   1;
   case HIERARCHY_2:    return   2;
   case HIERARCHY_4:    return   4;
   case HIERARCHY_AUTO: return 999;
   default:             dlog(0, "%s, unknown hierarchy %d",
                        __FUNCTION__, Hierarchy);
                        return 999;
   }
}

int GetVDRRollOffFromDVB(fe_rolloff_t RollOff) {
 #if (DVB_API_VERSION >= 5)
 switch (RollOff) {
   case ROLLOFF_AUTO:    return 35;
   case ROLLOFF_20:      return 20;
   case ROLLOFF_25:      return 25;
   case ROLLOFF_35:      return 35;
   default:              dlog(0, "%s, unknown rolloff %d",
                         __FUNCTION__, RollOff);
                         return 999;
   }
 #else
 return 35;
 #endif
}


cString PrintTransponder(const cChannel * Transponder) {
  cString buffer;
  int Frequency = Transponder->Frequency();
  if (Frequency < 1000)    Frequency *= 1000;
  if (Frequency > 999999)  Frequency /= 1000;
#if VDRVERSNUM < 10713      
    switch (Transponder->Source()) {
        #ifdef PLUGINPARAMPATCHVERSNUM
        case cSource::stPlug:
            buffer = cString::sprintf("PVRINPUT %.2fMHz %s",
                 Frequency/1000.0,
                 Transponder->PluginParam());
            break;
        #endif
        case cSource::stCable:
            buffer = cString::sprintf("DVB-C %.3fMHz M%d SR%d",
                 Frequency/1000.0,
                 GetVDRModulationtypeFromDVB((fe_modulation_t) Transponder->Modulation()),
                 Transponder->Srate());
            break;
        case cSource::stTerr:
            buffer = cString::sprintf("DVB-T %.3fMHz M%dI%dB%dC%dD%dT%dG%dY%d",
                 Frequency/1000.0, GetVDRModulationtypeFromDVB((fe_modulation_t) Transponder->Modulation()),
                 GetVDRInversionFromDVB((fe_spectral_inversion_t) Transponder->Inversion()),
                 Transponder->Bandwidth()<6000?
                       GetVDRBandwidthFromDVB((fe_bandwidth_t) Transponder->Bandwidth()):
                       GetVDRBandwidthFromInt(Transponder->Bandwidth()),
                 GetVDRCoderateFromDVB((fe_code_rate_t) Transponder->CoderateH()),
                 GetVDRCoderateFromDVB((fe_code_rate_t) Transponder->CoderateL()),
                 GetVDRTransmissionModeFromDVB((fe_transmit_mode_t) Transponder->Transmission()),
                 GetVDRGuardFromDVB((fe_guard_interval_t)Transponder->Guard()),
                 GetVDRHierarchyFromDVB((fe_hierarchy_t) Transponder->Hierarchy()));
            break;
        default:
          if (cSource::IsSat(Transponder->Source())) {
            buffer = cString::sprintf("DVB-S %d %cM%dC%dO%dS%d SR%d (%s)",
                 Transponder->Frequency(),
                 Transponder->Polarization(),
                 GetVDRModulationtypeFromDVB((fe_modulation_t) Transponder->Modulation()),
                 GetVDRCoderateFromDVB((fe_code_rate_t) Transponder->CoderateH()),
                 #if VDRVERSNUM < 10700
                 35,0,
                 Transponder->Srate(),
                 "DVB-S");
                 #else
                 GetVDRRollOffFromDVB((fe_rolloff_t) Transponder->RollOff()),
                 Transponder->System(),                    
                 Transponder->Srate(),
                 Transponder->System()==SYS_DVBS?"DVB-S":Transponder->System()==SYS_DVBS2?"DVB-S2":"UNKNOWN SYSTEM");
                 #endif 
            }
        }
#else
    switch (Transponder->Source()) {
        case cSource::stCable:
            {
            cDvbTransponderParameters * p = new cDvbTransponderParameters(Transponder->Parameters());
            buffer = cString::sprintf("DVB-C %.3fMHz M%d SR%d",
                 Frequency/1000.0,
                 GetVDRModulationtypeFromDVB((fe_modulation_t) p->Modulation()),
                 Transponder->Srate());
            }
            break;
        case cSource::stTerr:
            {
            cDvbTransponderParameters * p = new cDvbTransponderParameters(Transponder->Parameters());
            buffer = cString::sprintf("DVB-T %.3fMHz M%dI%dB%dC%dD%dT%dG%dY%d",
                 Frequency/1000.0, GetVDRModulationtypeFromDVB((fe_modulation_t) p->Modulation()),
                 GetVDRInversionFromDVB((fe_spectral_inversion_t) p->Inversion()),
                 GetVDRBandwidthFromInt(p->Bandwidth()),
                 GetVDRCoderateFromDVB((fe_code_rate_t) p->CoderateH()),
                 GetVDRCoderateFromDVB((fe_code_rate_t) p->CoderateL()),
                 GetVDRTransmissionModeFromDVB((fe_transmit_mode_t) p->Transmission()),
                 GetVDRGuardFromDVB((fe_guard_interval_t)p->Guard()),
                 GetVDRHierarchyFromDVB((fe_hierarchy_t) p->Hierarchy()));
            }
            break;
        #if VDRVERSNUM > 10713
        case cSource::stAtsc:
            {
            cDvbTransponderParameters * p = new cDvbTransponderParameters(Transponder->Parameters());
            buffer = cString::sprintf("ATSC %.3fMHz M%d",
                 Frequency/1000.0, GetVDRModulationtypeFromDVB((fe_modulation_t) p->Modulation()));
            }
            break;
        #endif
        default:
          if (cSource::IsSat(Transponder->Source())) {
            cDvbTransponderParameters * p = new cDvbTransponderParameters(Transponder->Parameters());
            buffer = cString::sprintf("DVB-S %d %cM%dC%dO%dS%d SR%d (%s)",
                 Transponder->Frequency(),
                 p->Polarization(),
                 GetVDRModulationtypeFromDVB((fe_modulation_t) p->Modulation()),
                 GetVDRCoderateFromDVB((fe_code_rate_t) p->CoderateH()),
                 GetVDRRollOffFromDVB((fe_rolloff_t) p->RollOff()),
                 p->System(),                    
                 Transponder->Srate(),
                 p->System()==SYS_DVBS?"DVB-S":p->System()==SYS_DVBS2?"DVB-S2":"UNKNOWN SYSTEM");
            }
          else {
            buffer = cString::sprintf("PVRINPUT %.2fMHz %s",
                 Frequency/1000.0,
                 Transponder->Parameters());
            }
        }
#endif
    return buffer;
}


cString PrintChannel(const cChannel * Channel) {
  cString buffer = cString::sprintf("%s", *Channel->ToText());
  #if VDRVERSNUM >= 10700 
  buffer.Truncate(-1); //remove "\n"
  #endif
  return buffer;
} 

cString PrintDvbApi(void) {
  cString buffer = cString::sprintf("compiled for DVB API %d.%d %s",
                DVB_API_VERSION,
                DVB_API_VERSION_MINOR,
                (DVB_API_VERSION < 5)?"*** warning: DVB-S2 support disabled ***":"");
  return buffer;
}

/* these structs should be used in PrintDvbApiUsed *only*
 */
typedef struct ___a_property {
    __u32 c;
    __u32 r0[3];
    union {__u32 d;struct {__u8 d[32];__u32 l;__u32 r1[3];void *r2;} b;} u;
    int result;
    } __attribute__ ((packed)) ___a_property_t;
typedef struct ___propertylist {__u32 count; ___a_property_t *properties; } ___propertylist_t;

cString PrintDvbApiUsed(int cardIndex) {
  cString buffer = cString::sprintf("using DVB API 3.2");
  cString dev = cString::sprintf("/dev/dvb/adapter%d/frontend%d", cardIndex, 0);
  int fe = open(*dev, O_RDONLY | O_NONBLOCK);

  ___a_property_t p[1];
  ___propertylist_t pl;
  p[0].c = 35;
  pl.count = 1;
  pl.properties = p;
  if (ioctl(fe, _IOR('o', 83, ___propertylist_t), &pl)) {
    if (fe > -1) close(fe);
    return buffer;
    }
  close(fe);
  buffer = cString::sprintf("using DVB API %d.%d", p[0].u.d >> 8, p[0].u.d & 0xff);
  return buffer;
}

bool SetSatTransponderDataFromVDR(cChannel * channel, int Source, int Frequency, eSatPolarizations Polarization, int Srate, eCableSatCodeRates CoderateH, eSatModulationTypes Modulation, eSatSystems System, eSatRollOffs RollOff) {
  return SetSatTransponderDataFromDVB(channel, Source, Frequency, GetVDRPolarizationFromDVB(SatPolarizations(Polarization)), 
                                      Srate, CableSatCodeRates(CoderateH), SatModulationTypes(Modulation), SatSystems(System), SatRollOffs(RollOff));
}

bool SetCableTransponderDataFromVDR(cChannel * channel, int Source, int Frequency, eCableModulations Modulation, int Srate, eCableSatCodeRates CoderateH, eCableTerrInversions Inversion) {
  return SetCableTransponderDataFromDVB(channel, Source, Frequency, CableModulations(Modulation), Srate, CableSatCodeRates(CoderateH), CableTerrInversions(Inversion));
}

bool SetTerrTransponderDataFromVDR(cChannel * channel, int Source, int Frequency, eTerrBandwidths Bandwidth, eTerrConstellations Modulation, eTerrHierarchies Hierarchy, eTerrCodeRates CodeRateH, eTerrCodeRates CodeRateL, eTerrGuardIntervals Guard, eTerrTransmissionModes Transmission, eCableTerrInversions Inversion) {
  return SetTerrTransponderDataFromDVB(channel, Source, Frequency, TerrBandwidths(Bandwidth), TerrConstellations(Modulation),
                                      TerrHierarchies(Hierarchy), TerrCodeRates(CodeRateH), TerrCodeRates(CodeRateL),
                                      TerrGuardIntervals(Guard), TerrTransmissionModes(Transmission), CableTerrInversions(Inversion));
}


unsigned int  GetFrontendStatus(int cardIndex) {
  fe_status_t value;
  cString dev = cString::sprintf("/dev/dvb/adapter%d/frontend%d", cardIndex, 0);

  int fe = open(*dev, O_RDONLY | O_NONBLOCK);
  if (fe < 0) {
     dlog(0, "GetFrontendStatus(): could not open %s", *dev);
     return 0;
     }
  if (IOCTL(fe, FE_READ_STATUS, &value) < 0) {
     close(fe);
     dlog(0, "GetFrontendStatus(): could not read %s", *dev);
     return 0;
     }
  close(fe);
  return value;
}

unsigned int  GetFrontendStrength(int cardIndex) {
  uint16_t value;
  cString dev = cString::sprintf("/dev/dvb/adapter%d/frontend%d", cardIndex, 0);

  int fe = open(*dev, O_RDONLY | O_NONBLOCK);
  if (fe < 0) {
     dlog(0, "GetFrontendStatus(): could not open %s", *dev);
     return 0;
     }
  if (IOCTL(fe, FE_READ_SIGNAL_STRENGTH, &value) < 0) {
     close(fe);
     dlog(0, "GetFrontendStrength(): could not read %s", *dev);
     return 0;
     }
  close(fe);
  return value;
}

cString GetFeName(int cardIndex) {
  struct dvb_frontend_info fe_info;
  cString dev = cString::sprintf("/dev/dvb/adapter%d/frontend%d", cardIndex, 0);
  cString fe_name;
  int fe = open(*dev, O_RDONLY | O_NONBLOCK);
  if (fe < 0) {
     dlog(0, "GetCapabilities(): could not open %s", *dev);
     return 0;
     }
  if (IOCTL(fe, FE_GET_INFO, &fe_info) < 0) {
     close(fe);
     dlog(0, "GetCapabilities(): could not read %s", *dev);
     return 0;
     }
  close(fe);
  fe_name = cString::sprintf("%s", (const char *) fe_info.name);
  return fe_name;
}

unsigned int GetFeType(int cardIndex) {
  struct dvb_frontend_info fe_info;
  cString dev = cString::sprintf("/dev/dvb/adapter%d/frontend%d", cardIndex, 0);

  int fe = open(*dev, O_RDONLY | O_NONBLOCK);
  if (fe < 0) {
     dlog(0, "GetCapabilities(): could not open %s", *dev);
     return 0;
     }
  if (IOCTL(fe, FE_GET_INFO, &fe_info) < 0) {
     close(fe);
     dlog(0, "GetCapabilities(): could not read %s", *dev);
     return 0;
     }
  close(fe);
  return fe_info.type;
}

unsigned int  GetCapabilities(int cardIndex) {
  struct dvb_frontend_info fe_info;
  cString dev = cString::sprintf("/dev/dvb/adapter%d/frontend%d", cardIndex, 0);

  int fe = open(*dev, O_RDONLY | O_NONBLOCK);
  if (fe < 0) {
     dlog(0, "GetCapabilities(): could not open %s", *dev);
     return 0;
     }
  if (IOCTL(fe, FE_GET_INFO, &fe_info) < 0) {
     close(fe);
     dlog(0, "GetCapabilities(): could not read %s", *dev);
     return 0;
     }
  close(fe);
  return fe_info.caps;
}

bool GetTerrCapabilities (int cardIndex, bool *CodeRate, bool *Modulation, bool *Inversion, bool *Bandwidth, bool *Hierarchy,
                              bool *TransmissionMode, bool *GuardInterval) {
  unsigned int cap = GetCapabilities(cardIndex);
  if (cap == 0)
     return false;
  *CodeRate         = cap & FE_CAN_FEC_AUTO;
  *Modulation       = cap & FE_CAN_QAM_AUTO;
  *Inversion        = cap & FE_CAN_INVERSION_AUTO; 
  *Bandwidth        = cap & FE_CAN_BANDWIDTH_AUTO;
  *Hierarchy        = cap & FE_CAN_HIERARCHY_AUTO;
  *TransmissionMode = cap & FE_CAN_GUARD_INTERVAL_AUTO;
  *GuardInterval    = cap & FE_CAN_TRANSMISSION_MODE_AUTO;
  return true; 
}

bool GetCableCapabilities(int cardIndex, bool *CodeRate, bool *Modulation, bool *Inversion) {
  int cap = GetCapabilities(cardIndex);
  if (cap < 0)
     return false;
  *CodeRate         = cap & FE_CAN_FEC_AUTO;
  *Modulation       = cap & FE_CAN_QAM_AUTO;
  *Inversion        = cap & FE_CAN_INVERSION_AUTO;
  return true; 
}

bool GetAtscCapabilities (int cardIndex, bool *CodeRate, bool *Modulation, bool *Inversion, bool *VSB, bool * QAM) {
  int cap = GetCapabilities(cardIndex);
  if (cap < 0)
     return false;
  *CodeRate         = cap & FE_CAN_FEC_AUTO;
  *Modulation       = cap & FE_CAN_QAM_AUTO;
  *Inversion        = cap & FE_CAN_INVERSION_AUTO;
  #if (DVB_API_VERSION >= 5)
  *VSB              = cap & FE_CAN_8VSB;
  #else
  *VSB              = false;
  #endif
  *QAM              = cap & FE_CAN_QAM_256;
  return true; 
}

bool GetSatCapabilities  (int cardIndex, bool *CodeRate, bool *Modulation, bool *RollOff, bool *DvbS2) {
  int cap = GetCapabilities(cardIndex);
  if (cap < 0)
     return false;
  *CodeRate         = cap & FE_CAN_FEC_AUTO;
  *Modulation       = cap & FE_CAN_QAM_AUTO;
  *RollOff          = cap & 0 /* there is no capability flag foreseen for rolloff auto? */ ;
  #if (DVB_API_VERSION >= 5)
  *DvbS2            = cap & FE_CAN_2G_MODULATION;
  #else
  *DvbS2            = false;
  #endif
  return true; 
}

bool IsPvrinput(const cChannel * Channel) {
#if VDRVERSNUM > 10713

   if (Channel->IsSourceType('V'))
      return true;

#elif defined (PLUGINPARAMPATCHVERSNUM)

   if (Channel->IsPlug()) {
      char Plugin[64];
      char optArg1[64];
      char optArg2[64];
      char optArg3[64];

      if ((sscanf(Channel->PluginParam(), "%a[^|]|%a[^|]|%a[^|]|%a[^:\n]", Plugin, optArg1, optArg2, optArg3) < 2))
         return false;
      if (! strcasecmp(Plugin, "PVRINPUT"))
         return true;
      }

#endif

   return false;
}

bool IsScantype(scantype_t type, const cChannel * c) {
  switch(type) {
        case DVB_TERR:   if (c->IsTerr())   return true; break;
        case DVB_CABLE:  if (c->IsCable())  return true; break;
        case DVB_SAT:    if (c->IsSat())    return true; break;
        case PVRINPUT_FM: 
        case PVRINPUT:   if (IsPvrinput(c)) return true; break;

        #if VDRVERSNUM > 10713
        case DVB_ATSC:   if (c->IsAtsc())   return true; break;
        #endif

        default:
            dlog(0, "%s(%d) UNKNOWN TYPE %d",
                __FUNCTION__, __LINE__, type);
        }
  return false;
}

bool ActiveTimers(scantype_t type) {
  for (cTimer * ti = Timers.First(); ti; ti = Timers.Next(ti))
      if (ti->Recording() && IsScantype(type, ti->Channel()))
         return true;

  return false;
}

bool PendingTimers(scantype_t type, int Margin) {
  time_t now = time(NULL);

  for (cTimer * ti = Timers.First(); ti; ti = Timers.Next(ti)) {
      ti->Matches();
      if (ti->HasFlags(tfActive) && (ti->StartTime() <= now + Margin) &&
           IsScantype(type, ti->Channel()))
         return true;
      }
  return false;
}
