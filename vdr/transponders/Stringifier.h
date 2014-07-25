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
#pragma once

#include "TransponderTypes.h"

namespace VDR
{

class Stringifier
{
public:
  static const char* TypeToString(TRANSPONDER_TYPE type);
  static TRANSPONDER_TYPE StringToType(const char* strType);

  static const char* BandwidthToString(fe_bandwidth_t bandwidth);
  static fe_bandwidth_t StringToBandwidth(const char* strBandwidth);

  static const char* CoderateToString(fe_code_rate_t coderate);
  static fe_code_rate_t StringToCoderate(const char* strCoderate);

  static const char* DeliverySystemToString(fe_delivery_system_t deliverySystem);
  static fe_delivery_system_t StringToDeliverySystem(const char* strDeliverySystem);

  static const char* GuardToString(fe_guard_interval_t guard);
  static fe_guard_interval_t StringToGuard(const char* strGuard);

  static const char* HierarchyToString(fe_hierarchy_t hierarchy);
  static fe_hierarchy_t StringToHierarchy(const char* strHierarchy);

  static const char* InversionToString(fe_spectral_inversion_t inversion);
  static fe_spectral_inversion_t StringToInversion(const char* strInversion);

  static const char* ModulationToString(fe_modulation_t modulation);
  static fe_modulation_t StringToModulation(const char* strModulation);

  static const char* PolarizationToString(fe_polarization_t polarization);
  static fe_polarization_t StringToPolarization(const char* strPolarization);

  static const char* RollOffToString(fe_rolloff_t rollOff);
  static fe_rolloff_t StringToRollOff(const char* strRollOff);

  static const char* TransmissionToString(fe_transmit_mode_t transmission);
  static fe_transmit_mode_t StringToTransmission(const char* strTransmission);
};

}
