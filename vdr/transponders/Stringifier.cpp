/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "Stringifier.h"
#include "TransponderDefinitions.h"

#include <string.h>

namespace VDR
{

const char* Stringifier::TypeToString(TRANSPONDER_TYPE type)
{
  switch (type)
  {
  case TRANSPONDER_ATSC:        return TRANSPONDER_STRING_TYPE_ATSC;
  case TRANSPONDER_CABLE:       return TRANSPONDER_STRING_TYPE_CABLE;
  case TRANSPONDER_SATELLITE:   return TRANSPONDER_STRING_TYPE_SATELLITE;
  case TRANSPONDER_TERRESTRIAL: return TRANSPONDER_STRING_TYPE_TERRESTRIAL;
  default:                      return "";
  }
}

TRANSPONDER_TYPE Stringifier::StringToType(const char* strType)
{
  if (strType)
  {
    if (strcmp(strType, TRANSPONDER_STRING_TYPE_ATSC)        == 0) return TRANSPONDER_ATSC;
    if (strcmp(strType, TRANSPONDER_STRING_TYPE_CABLE)       == 0) return TRANSPONDER_CABLE;
    if (strcmp(strType, TRANSPONDER_STRING_TYPE_SATELLITE)   == 0) return TRANSPONDER_SATELLITE;
    if (strcmp(strType, TRANSPONDER_STRING_TYPE_TERRESTRIAL) == 0) return TRANSPONDER_TERRESTRIAL;
  }
  return TRANSPONDER_INVALID;
}

const char* Stringifier::BandwidthToString(fe_bandwidth_t bandwidth)
{
  switch (bandwidth)
  {
  case BANDWIDTH_8_MHZ:     return TRANSPONDER_STRING_BANDWIDTH_8_MHZ;
  case BANDWIDTH_7_MHZ:     return TRANSPONDER_STRING_BANDWIDTH_7_MHZ;
  case BANDWIDTH_6_MHZ:     return TRANSPONDER_STRING_BANDWIDTH_6_MHZ;
  case BANDWIDTH_AUTO:      return TRANSPONDER_STRING_BANDWIDTH_AUTO;
  case BANDWIDTH_5_MHZ:     return TRANSPONDER_STRING_BANDWIDTH_5_MHZ;
  case BANDWIDTH_10_MHZ:    return TRANSPONDER_STRING_BANDWIDTH_10_MHZ;
  case BANDWIDTH_1_712_MHZ: return TRANSPONDER_STRING_BANDWIDTH_1_712_MHZ;
  default:                  return "";
  }
}

fe_bandwidth_t Stringifier::StringToBandwidth(const char* strBandwidth)
{
  if (strBandwidth)
  {
    if (strcmp(strBandwidth, TRANSPONDER_STRING_BANDWIDTH_8_MHZ)     == 0) return BANDWIDTH_8_MHZ;
    if (strcmp(strBandwidth, TRANSPONDER_STRING_BANDWIDTH_7_MHZ)     == 0) return BANDWIDTH_7_MHZ;
    if (strcmp(strBandwidth, TRANSPONDER_STRING_BANDWIDTH_6_MHZ)     == 0) return BANDWIDTH_6_MHZ;
    if (strcmp(strBandwidth, TRANSPONDER_STRING_BANDWIDTH_AUTO)      == 0) return BANDWIDTH_AUTO;
    if (strcmp(strBandwidth, TRANSPONDER_STRING_BANDWIDTH_5_MHZ)     == 0) return BANDWIDTH_5_MHZ;
    if (strcmp(strBandwidth, TRANSPONDER_STRING_BANDWIDTH_10_MHZ)    == 0) return BANDWIDTH_10_MHZ;
    if (strcmp(strBandwidth, TRANSPONDER_STRING_BANDWIDTH_1_712_MHZ) == 0) return BANDWIDTH_1_712_MHZ;
  }
  return BANDWIDTH_8_MHZ;
}

const char* Stringifier::CoderateToString(fe_code_rate_t coderate)
{
  switch (coderate)
  {
  case FEC_NONE: return TRANSPONDER_STRING_FEC_NONE;
  case FEC_1_2:  return TRANSPONDER_STRING_FEC_1_2;
  case FEC_2_3:  return TRANSPONDER_STRING_FEC_2_3;
  case FEC_3_4:  return TRANSPONDER_STRING_FEC_3_4;
  case FEC_4_5:  return TRANSPONDER_STRING_FEC_4_5;
  case FEC_5_6:  return TRANSPONDER_STRING_FEC_5_6;
  case FEC_6_7:  return TRANSPONDER_STRING_FEC_6_7;
  case FEC_7_8:  return TRANSPONDER_STRING_FEC_7_8;
  case FEC_8_9:  return TRANSPONDER_STRING_FEC_8_9;
  case FEC_AUTO: return TRANSPONDER_STRING_FEC_AUTO;
  case FEC_3_5:  return TRANSPONDER_STRING_FEC_3_5;
  case FEC_9_10: return TRANSPONDER_STRING_FEC_9_10;
  case FEC_2_5:  return TRANSPONDER_STRING_FEC_2_5;
  default:       return "";
  }
}

fe_code_rate_t Stringifier::StringToCoderate(const char* strCoderate)
{
  if (strCoderate)
  {
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_NONE) == 0) return FEC_NONE;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_1_2)  == 0) return FEC_1_2;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_2_3)  == 0) return FEC_2_3;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_3_4)  == 0) return FEC_3_4;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_4_5)  == 0) return FEC_4_5;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_5_6)  == 0) return FEC_5_6;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_6_7)  == 0) return FEC_6_7;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_7_8)  == 0) return FEC_7_8;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_8_9)  == 0) return FEC_8_9;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_AUTO) == 0) return FEC_AUTO;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_3_5)  == 0) return FEC_3_5;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_9_10) == 0) return FEC_9_10;
    if (strcmp(strCoderate, TRANSPONDER_STRING_FEC_2_5)  == 0) return FEC_2_5;
  }
  return FEC_AUTO;
}

const char* Stringifier::DeliverySystemToString(fe_delivery_system_t deliverySystem)
{
  switch (deliverySystem)
  {
  case SYS_UNDEFINED:    return TRANSPONDER_STRING_SYSTEM_UNDEFINED;
  case SYS_DVBC_ANNEX_AC:return TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_A;
  case SYS_DVBC_ANNEX_B: return TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_B;
  case SYS_DVBT:         return TRANSPONDER_STRING_SYSTEM_DVBT;
  case SYS_DSS:          return TRANSPONDER_STRING_SYSTEM_DSS;
  case SYS_DVBS:         return TRANSPONDER_STRING_SYSTEM_DVBS;
  case SYS_DVBS2:        return TRANSPONDER_STRING_SYSTEM_DVBS2;
  case SYS_DVBH:         return TRANSPONDER_STRING_SYSTEM_DVBH;
  case SYS_ISDBT:        return TRANSPONDER_STRING_SYSTEM_ISDBT;
  case SYS_ISDBS:        return TRANSPONDER_STRING_SYSTEM_ISDBS;
  case SYS_ISDBC:        return TRANSPONDER_STRING_SYSTEM_ISDBC;
  case SYS_ATSC:         return TRANSPONDER_STRING_SYSTEM_ATSC;
  case SYS_ATSCMH:       return TRANSPONDER_STRING_SYSTEM_ATSCMH;
  case SYS_DTMB:         return TRANSPONDER_STRING_SYSTEM_DTMB;
  case SYS_CMMB:         return TRANSPONDER_STRING_SYSTEM_CMMB;
  case SYS_DAB:          return TRANSPONDER_STRING_SYSTEM_DAB;
  case SYS_DVBT2:        return TRANSPONDER_STRING_SYSTEM_DVBT2;
  case SYS_TURBO:        return TRANSPONDER_STRING_SYSTEM_TURBO;
  case SYS_DVBC_ANNEX_C: return TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_C;
  default:               return "";
  }
}

fe_delivery_system_t Stringifier::StringToDeliverySystem(const char* strDeliverySystem)
{
  if (strDeliverySystem)
  {
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_UNDEFINED)    == 0) return SYS_UNDEFINED;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_A) == 0) return SYS_DVBC_ANNEX_AC;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_B) == 0) return SYS_DVBC_ANNEX_B;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DVBT)         == 0) return SYS_DVBT;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DSS)          == 0) return SYS_DSS;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DVBS)         == 0) return SYS_DVBS;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DVBS2)        == 0) return SYS_DVBS2;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DVBH)         == 0) return SYS_DVBH;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_ISDBT)        == 0) return SYS_ISDBT;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_ISDBS)        == 0) return SYS_ISDBS;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_ISDBC)        == 0) return SYS_ISDBC;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_ATSC)         == 0) return SYS_ATSC;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_ATSCMH)       == 0) return SYS_ATSCMH;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DTMB)         == 0) return SYS_DTMB;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_CMMB)         == 0) return SYS_CMMB;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DAB)          == 0) return SYS_DAB;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DVBT2)        == 0) return SYS_DVBT2;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_TURBO)        == 0) return SYS_TURBO;
    if (strcmp(strDeliverySystem, TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_C) == 0) return SYS_DVBC_ANNEX_C;
  }
  return SYS_UNDEFINED;
}

const char* Stringifier::GuardToString(fe_guard_interval_t guard)
{
  switch (guard)
  {
  case GUARD_INTERVAL_1_32:   return TRANSPONDER_STRING_GUARD_1_32;
  case GUARD_INTERVAL_1_16:   return TRANSPONDER_STRING_GUARD_1_16;
  case GUARD_INTERVAL_1_8:    return TRANSPONDER_STRING_GUARD_1_8;
  case GUARD_INTERVAL_1_4:    return TRANSPONDER_STRING_GUARD_1_4;
  case GUARD_INTERVAL_AUTO:   return TRANSPONDER_STRING_GUARD_AUTO;
  case GUARD_INTERVAL_1_128:  return TRANSPONDER_STRING_GUARD_1_128;
  case GUARD_INTERVAL_19_128: return TRANSPONDER_STRING_GUARD_19_128;
  case GUARD_INTERVAL_19_256: return TRANSPONDER_STRING_GUARD_19_256;
  case GUARD_INTERVAL_PN420:  return TRANSPONDER_STRING_GUARD_PN420;
  case GUARD_INTERVAL_PN595:  return TRANSPONDER_STRING_GUARD_PN595;
  case GUARD_INTERVAL_PN945:  return TRANSPONDER_STRING_GUARD_PN945;
  default:                    return "";
  }
}

fe_guard_interval_t Stringifier::StringToGuard(const char* strGuard)
{
  if (strGuard)
  {
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_1_32)   == 0) return GUARD_INTERVAL_1_32;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_1_16)   == 0) return GUARD_INTERVAL_1_16;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_1_8)    == 0) return GUARD_INTERVAL_1_8;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_1_4)    == 0) return GUARD_INTERVAL_1_4;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_AUTO)   == 0) return GUARD_INTERVAL_AUTO;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_1_128)  == 0) return GUARD_INTERVAL_1_128;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_19_128) == 0) return GUARD_INTERVAL_19_128;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_19_256) == 0) return GUARD_INTERVAL_19_256;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_PN420)  == 0) return GUARD_INTERVAL_PN420;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_PN595)  == 0) return GUARD_INTERVAL_PN595;
    if (strcmp(strGuard, TRANSPONDER_STRING_GUARD_PN945)  == 0) return GUARD_INTERVAL_PN945;
  }
  return GUARD_INTERVAL_AUTO;
}

const char* Stringifier::HierarchyToString(fe_hierarchy_t hierarchy)
{
  switch (hierarchy)
  {
  case HIERARCHY_NONE: return TRANSPONDER_STRING_HIERARCHY_NONE;
  case HIERARCHY_1:    return TRANSPONDER_STRING_HIERARCHY_1;
  case HIERARCHY_2:    return TRANSPONDER_STRING_HIERARCHY_2;
  case HIERARCHY_4:    return TRANSPONDER_STRING_HIERARCHY_4;
  case HIERARCHY_AUTO: return TRANSPONDER_STRING_HIERARCHY_AUTO;
  default:             return "";
  }
}

fe_hierarchy_t Stringifier::StringToHierarchy(const char* strHierarchy)
{
  if (strHierarchy)
  {
    if (strcmp(strHierarchy, TRANSPONDER_STRING_HIERARCHY_NONE) == 0) return HIERARCHY_NONE;
    if (strcmp(strHierarchy, TRANSPONDER_STRING_HIERARCHY_1)    == 0) return HIERARCHY_1;
    if (strcmp(strHierarchy, TRANSPONDER_STRING_HIERARCHY_2)    == 0) return HIERARCHY_2;
    if (strcmp(strHierarchy, TRANSPONDER_STRING_HIERARCHY_4)    == 0) return HIERARCHY_4;
    if (strcmp(strHierarchy, TRANSPONDER_STRING_HIERARCHY_AUTO) == 0) return HIERARCHY_AUTO;
  }
  return HIERARCHY_AUTO;
}

const char* Stringifier::InversionToString(fe_spectral_inversion_t inversion)
{
  switch (inversion)
  {
  case INVERSION_OFF:  return TRANSPONDER_STRING_INVERSION_OFF;
  case INVERSION_ON:   return TRANSPONDER_STRING_INVERSION_ON;
  case INVERSION_AUTO: return TRANSPONDER_STRING_INVERSION_AUTO;
  default:             return "";
  }
}

fe_spectral_inversion_t Stringifier::StringToInversion(const char* strInversion)
{
  if (strInversion)
  {
    if (strcmp(strInversion, TRANSPONDER_STRING_INVERSION_OFF)  == 0) return INVERSION_OFF;
    if (strcmp(strInversion, TRANSPONDER_STRING_INVERSION_ON)   == 0) return INVERSION_ON;
    if (strcmp(strInversion, TRANSPONDER_STRING_INVERSION_AUTO) == 0) return INVERSION_AUTO;
  }
  return INVERSION_AUTO;
}

const char* Stringifier::ModulationToString(fe_modulation_t modulation)
{
  switch (modulation)
  {
  case QPSK:     return TRANSPONDER_STRING_MODULATION_QPSK;
  case QAM_16:   return TRANSPONDER_STRING_MODULATION_QAM_16;
  case QAM_32:   return TRANSPONDER_STRING_MODULATION_QAM_32;
  case QAM_64:   return TRANSPONDER_STRING_MODULATION_QAM_64;
  case QAM_128:  return TRANSPONDER_STRING_MODULATION_QAM_128;
  case QAM_256:  return TRANSPONDER_STRING_MODULATION_QAM_256;
  case QAM_AUTO: return TRANSPONDER_STRING_MODULATION_QAM_AUTO;
  case VSB_8:    return TRANSPONDER_STRING_MODULATION_VSB_8;
  case VSB_16:   return TRANSPONDER_STRING_MODULATION_VSB_16;
  case PSK_8:    return TRANSPONDER_STRING_MODULATION_PSK_8;
  case APSK_16:  return TRANSPONDER_STRING_MODULATION_APSK_16;
  case APSK_32:  return TRANSPONDER_STRING_MODULATION_APSK_32;
  case DQPSK:    return TRANSPONDER_STRING_MODULATION_DQPSK;
  case QAM_4_NR: return TRANSPONDER_STRING_MODULATION_QAM_4_NR;
  default:       return "";
  }
}

fe_modulation_t Stringifier::StringToModulation(const char* strModulation)
{
  if (strModulation)
  {
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_QPSK)     == 0) return QPSK;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_QAM_16)   == 0) return QAM_16;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_QAM_32)   == 0) return QAM_32;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_QAM_64)   == 0) return QAM_64;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_QAM_128)  == 0) return QAM_128;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_QAM_256)  == 0) return QAM_256;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_QAM_AUTO) == 0) return QAM_AUTO;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_VSB_8)    == 0) return VSB_8;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_VSB_16)   == 0) return VSB_16;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_PSK_8)    == 0) return PSK_8;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_APSK_16)  == 0) return APSK_16;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_APSK_32)  == 0) return APSK_32;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_DQPSK)    == 0) return DQPSK;
    if (strcmp(strModulation, TRANSPONDER_STRING_MODULATION_QAM_4_NR) == 0) return QAM_4_NR;
  }
  return QPSK;
}

const char* Stringifier::PolarizationToString(fe_polarization_t polarization)
{
  switch (polarization)
  {
  case POLARIZATION_HORIZONTAL:     return TRANSPONDER_STRING_POLARIZATION_HORIZONTAL;
  case POLARIZATION_VERTICAL:       return TRANSPONDER_STRING_POLARIZATION_VERTICAL;
  case POLARIZATION_CIRCULAR_LEFT:  return TRANSPONDER_STRING_POLARIZATION_LEFT;
  case POLARIZATION_CIRCULAR_RIGHT: return TRANSPONDER_STRING_POLARIZATION_RIGHT;
  default:                          return "";
  }
}

fe_polarization_t Stringifier::StringToPolarization(const char* strPolarization)
{
  if (strPolarization)
  {
    if (strcmp(strPolarization, TRANSPONDER_STRING_POLARIZATION_HORIZONTAL) == 0) return POLARIZATION_HORIZONTAL;
    if (strcmp(strPolarization, TRANSPONDER_STRING_POLARIZATION_VERTICAL)   == 0) return POLARIZATION_VERTICAL;
    if (strcmp(strPolarization, TRANSPONDER_STRING_POLARIZATION_LEFT)       == 0) return POLARIZATION_CIRCULAR_LEFT;
    if (strcmp(strPolarization, TRANSPONDER_STRING_POLARIZATION_RIGHT)      == 0) return POLARIZATION_CIRCULAR_RIGHT;
  }
  return POLARIZATION_HORIZONTAL;
}

const char* Stringifier::RollOffToString(fe_rolloff_t rollOff)
{
  switch (rollOff)
  {
  case ROLLOFF_35:   return TRANSPONDER_STRING_ROLLOFF_35;
  case ROLLOFF_20:   return TRANSPONDER_STRING_ROLLOFF_20;
  case ROLLOFF_25:   return TRANSPONDER_STRING_ROLLOFF_25;
  case ROLLOFF_AUTO: return TRANSPONDER_STRING_ROLLOFF_AUTO;
  default:           return "";
  }
}

fe_rolloff_t Stringifier::StringToRollOff(const char* strRollOff)
{
  if (strRollOff)
  {
    if (strcmp(strRollOff, TRANSPONDER_STRING_ROLLOFF_35)   == 0) return ROLLOFF_35;
    if (strcmp(strRollOff, TRANSPONDER_STRING_ROLLOFF_20)   == 0) return ROLLOFF_20;
    if (strcmp(strRollOff, TRANSPONDER_STRING_ROLLOFF_25)   == 0) return ROLLOFF_25;
    if (strcmp(strRollOff, TRANSPONDER_STRING_ROLLOFF_AUTO) == 0) return ROLLOFF_AUTO;
  }
  return ROLLOFF_AUTO;
}

const char* Stringifier::TransmissionToString(fe_transmit_mode_t transmission)
{
  switch (transmission)
  {
  case TRANSMISSION_MODE_2K:    return TRANSPONDER_STRING_TRANSMISSION_2K;
  case TRANSMISSION_MODE_8K:    return TRANSPONDER_STRING_TRANSMISSION_8K;
  case TRANSMISSION_MODE_AUTO:  return TRANSPONDER_STRING_TRANSMISSION_AUTO;
  case TRANSMISSION_MODE_4K:    return TRANSPONDER_STRING_TRANSMISSION_4K;
  case TRANSMISSION_MODE_1K:    return TRANSPONDER_STRING_TRANSMISSION_1K;
  case TRANSMISSION_MODE_16K:   return TRANSPONDER_STRING_TRANSMISSION_16K;
  case TRANSMISSION_MODE_32K:   return TRANSPONDER_STRING_TRANSMISSION_32K;
  case TRANSMISSION_MODE_C1:    return TRANSPONDER_STRING_TRANSMISSION_C1;
  case TRANSMISSION_MODE_C3780: return TRANSPONDER_STRING_TRANSMISSION_C3780;
  default:                      return "";
  }
}

fe_transmit_mode_t Stringifier::StringToTransmission(const char* strTransmission)
{
  if (strTransmission)
  {
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_2K)    == 0) return TRANSMISSION_MODE_2K;
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_8K)    == 0) return TRANSMISSION_MODE_8K;
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_AUTO)  == 0) return TRANSMISSION_MODE_AUTO;
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_4K)    == 0) return TRANSMISSION_MODE_4K;
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_1K)    == 0) return TRANSMISSION_MODE_1K;
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_16K)   == 0) return TRANSMISSION_MODE_16K;
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_32K)   == 0) return TRANSMISSION_MODE_32K;
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_C1)    == 0) return TRANSMISSION_MODE_C1;
    if (strcmp(strTransmission, TRANSPONDER_STRING_TRANSMISSION_C3780) == 0) return TRANSMISSION_MODE_C3780;
  }
  return TRANSMISSION_MODE_AUTO;
}

}
