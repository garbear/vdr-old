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

#include "TransponderParams.h"
#include "TransponderTypes.h"

class TiXmlNode;

namespace VDR
{

class cTransponder
{
public:
  cTransponder(TRANSPONDER_TYPE type = TRANSPONDER_INVALID);

  void Reset(void);

  bool operator==(const cTransponder& rhs) const;
  bool operator!=(const cTransponder& rhs) const { return !(*this == rhs); }

  TRANSPONDER_TYPE Type(void) const { return m_type; }
  void SetType(TRANSPONDER_TYPE type);

  // Parameters common to all transponders
  fe_delivery_system_t DeliverySystem(void) const;
  void SetDeliverySystem(fe_delivery_system_t ds) { m_deliverySystem = ds; }

  unsigned int FrequencyHz(void) const            { return m_frequencyHz; }
  unsigned int FrequencyKHz(void) const           { return m_frequencyHz / 1000; }
  unsigned int FrequencyMHz(void) const           { return m_frequencyHz / (1000 * 1000); }
  void SetFrequencyHz(unsigned int frequencyHz)   { m_frequencyHz = frequencyHz; }
  void SetFrequencyKHz(unsigned int frequencyKHz) { m_frequencyHz = frequencyKHz / 1000; }
  void SetFrequencyMHz(unsigned int frequencyMHz) { m_frequencyHz = frequencyMHz / (1000 * 1000); }

  fe_spectral_inversion_t Inversion(void) const        { return m_inversion; }
  void SetInversion(fe_spectral_inversion_t inversion) { m_inversion = inversion; }

  fe_modulation_t Modulation(void) const         { return m_modulation; }
  void SetModulation(fe_modulation_t modulation) { m_modulation = modulation; }

  // Parameters specific to transponder type
  const cAtscParams& AtscParams(void) const               { return m_atscParams; }
        cAtscParams& AtscParams(void)                     { return m_atscParams; }
  const cCableParams&       CableParams(void) const       { return m_cableParams; }
        cCableParams&       CableParams(void)             { return m_cableParams; }
  const cSatelliteParams&   SatelliteParams(void) const   { return m_satelliteParams; }
        cSatelliteParams&   SatelliteParams(void)         { return m_satelliteParams; }
  const cTerrestrialParams& TerrestrialParams(void) const { return m_terrestrialParams; }
        cTerrestrialParams& TerrestrialParams(void)       { return m_terrestrialParams; }

  bool IsValid(void) const       { return m_type != TRANSPONDER_INVALID; }
  bool IsAtsc(void) const        { return m_type == TRANSPONDER_ATSC; }
  bool IsCable(void) const       { return m_type == TRANSPONDER_CABLE; }
  bool IsSatellite(void) const   { return m_type == TRANSPONDER_SATELLITE; }
  bool IsTerrestrial(void) const { return m_type == TRANSPONDER_TERRESTRIAL; }

  bool Serialise(TiXmlNode* node) const;
  bool Deserialise(const TiXmlNode* node);

private:
  TRANSPONDER_TYPE        m_type;

  // Parameters common to all transponders
  fe_delivery_system_t    m_deliverySystem;
  unsigned int            m_frequencyHz;
  fe_spectral_inversion_t m_inversion;
  fe_modulation_t         m_modulation;

  // Parameters specific to transponder type
  cAtscParams             m_atscParams;
  cCableParams            m_cableParams;
  cSatelliteParams        m_satelliteParams;
  cTerrestrialParams      m_terrestrialParams;

};

}
