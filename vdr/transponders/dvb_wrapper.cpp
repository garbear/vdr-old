/*
 * dvb_wrapper.c: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * wirbelscan-0.0.5
 *
 * $Id$ v20101210
 */

#include "dvb_wrapper.h"

namespace VDR
{

fe_code_rate_t CableSatCodeRates(eCableSatCodeRates cr)
{
  switch(cr)
  {
    case eCoderateNone: return FEC_NONE;
    case eCoderate12  : return FEC_1_2;
    case eCoderate23  : return FEC_2_3;
    case eCoderate34  : return FEC_3_4;
    case eCoderate56  : return FEC_5_6;
    case eCoderate78  : return FEC_7_8;
    case eCoderate89  : return FEC_8_9;
    case eCoderateAuto: return FEC_AUTO;
    case eCoderate35  : return FEC_3_5;
    case eCoderate45  : return FEC_4_5;
    case eCoderate910 : return FEC_9_10;
    default           : return FEC_AUTO;
  }
}

fe_modulation_t CableModulations(eCableModulations cm)
{
  switch(cm)
  {
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
    default                 : return QAM_AUTO;
  }
}

fe_modulation_t AtscModulations(eAtscModulations am)
{
  switch(am)
  {
    case eVsb8     : return VSB_8;
    case eVsb16    : return VSB_16;
    case eQam64    : return QAM_64;
    case eQam256   : return QAM_256;
    case eQamAuto  : return QAM_AUTO;
    default        : return QAM_AUTO;
  }
}

fe_polarization_t SatPolarizations(eSatPolarizations sp)
{
  switch(sp)
  {
    case eHorizontal  : return POLARIZATION_HORIZONTAL;
    case eVertical    : return POLARIZATION_VERTICAL;
    case eLeft        : return POLARIZATION_CIRCULAR_LEFT;
    case eRight       : return POLARIZATION_CIRCULAR_RIGHT;
    default           : return POLARIZATION_HORIZONTAL;
  }
}

fe_rolloff_t SatRollOffs(eSatRollOffs ro)
{
  switch(ro)
{
    case eRolloff35   : return ROLLOFF_35;
    case eRolloff25   : return ROLLOFF_25;
    case eRolloff20   : return ROLLOFF_20;
    case eRolloffAuto : return ROLLOFF_35;
    default           : return ROLLOFF_35;
  }
}

fe_delivery_system_t SatSystems(eSatSystems ss)
{
  switch(ss)
  {
    case eDvbs   : return SYS_DVBS;
    case eDvbs2  : return SYS_DVBS2;
    default      : return SYS_DVBS;
  }
}

fe_modulation_t SatModulationTypes(eSatModulationTypes mt)
{
  switch(mt)
  {
    case eSatModulationQpsk : return QPSK;
    case eSatModulationQam16: return QAM_16;
    case eSatModulation8psk : return PSK_8;
    default                 : return QPSK;
  }
}

int TerrBandwidths(eTerrBandwidths tb)
{
  switch(tb)
  {
    case eBandwidth8Mhz  : return 8000000;
    case eBandwidth7Mhz  : return 7000000;
    case eBandwidth6Mhz  : return 6000000;
    case eBandwidth5Mhz  : return 5000000;
    default              : return 8000000;
  }
}

fe_modulation_t TerrConstellations(eTerrConstellations tc)
{
  switch(tc)
  {
    case eModulationQpsk : return QPSK;
    case eModulation16   : return QAM_16;
    case eModulation64   : return QAM_64;
    case eModulationAuto : return QAM_AUTO;
    default              : return QAM_AUTO;
  }
}

fe_hierarchy_t TerrHierarchies(eTerrHierarchies th)
{
  switch(th)
  {
    case eHierarchyNone : return HIERARCHY_NONE;
    case eHierarchy1    : return HIERARCHY_1;
    case eHierarchy2    : return HIERARCHY_2;
    case eHierarchy4    : return HIERARCHY_4;
    case eHierarchyAuto : return HIERARCHY_AUTO;
    default             : return HIERARCHY_AUTO;
  }
}

fe_code_rate_t TerrCodeRates(eTerrCodeRates cr)
{
  switch(cr)
  {
    case eTerrCoderateNone: return FEC_NONE;
    case eTerrCoderate12  : return FEC_1_2;
    case eTerrCoderate23  : return FEC_2_3;
    case eTerrCoderate34  : return FEC_3_4;
    case eTerrCoderate56  : return FEC_5_6;
    case eTerrCoderate78  : return FEC_7_8;
    case eTerrCoderateAuto: return FEC_AUTO;
    default               : return FEC_AUTO;
  }
}

fe_guard_interval_t TerrGuardIntervals(eTerrGuardIntervals gi)
{
  switch(gi)
  {
    case eGuardinterval32  : return GUARD_INTERVAL_1_32;
    case eGuardinterval16  : return GUARD_INTERVAL_1_16;
    case eGuardinterval8   : return GUARD_INTERVAL_1_8;
    case eGuardinterval4   : return GUARD_INTERVAL_1_4;
    case eGuardintervalAuto: return GUARD_INTERVAL_AUTO;
    default                : return GUARD_INTERVAL_AUTO;
  }
}

fe_transmit_mode_t TerrTransmissionModes (eTerrTransmissionModes tm)
{
  switch(tm)
  {
    case eTransmissionmode2K  : return TRANSMISSION_MODE_2K;
    case eTransmissionmode8K  : return TRANSMISSION_MODE_8K;
    case eTransmissionmode4K  : return TRANSMISSION_MODE_4K;
    case eTransmissionmodeAuto: return TRANSMISSION_MODE_AUTO;
    default                   : return TRANSMISSION_MODE_AUTO;
  }
}

fe_spectral_inversion_t CableTerrInversions(eCableTerrInversions in)
{
  switch(in)
  {
    case eInversionOff : return INVERSION_OFF;
    case eInversionOn  : return INVERSION_ON;
    case eInversionAuto: return INVERSION_AUTO;
    default            : return INVERSION_AUTO;
  }
}

}
