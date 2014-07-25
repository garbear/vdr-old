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

class TiXmlElement;

namespace VDR
{

/*!
 * cAtscParams : Parameters required to tune ATSC channels
 *
 *   - None (params are common to all other types)
 */
class cAtscParams
{
public:
  cAtscParams(void) { }
  bool operator==(const cAtscParams& rhs) const { return true; }

  void Serialise(TiXmlElement* elem) const   { }
  void Deserialise(const TiXmlElement* elem) { }
};

 /*!
  * cCableParams : Parameters required to tune cable channels
  *
  *   - CoderateH
  *   - SymbolRate
  */
class cCableParams
{
public:
  cCableParams(void);
  bool operator==(const cCableParams& rhs) const;

  fe_code_rate_t CoderateH(void) const        { return m_coderateH; }
  void SetCoderateH(fe_code_rate_t coderateH) { m_coderateH = coderateH; }

  unsigned int SymbolRateHz(void) const           { return m_symbolRateHz; }
  void SetSymbolRateHz(unsigned int symbolRateHz) { m_symbolRateHz = symbolRateHz; }

  void Serialise(TiXmlElement* elem) const;
  void Deserialise(const TiXmlElement* elem);

private:
  fe_code_rate_t m_coderateH;
  unsigned int   m_symbolRateHz;
};

 /*!
  * cSatelliteParams : Parameters required to tune satellite channels
  *
  *   - CoderateH
  *   - Polarization
  *   - RollOff
  *   - StreamId
  *   - SymbolRate
  */
class cSatelliteParams
{
public:
  cSatelliteParams(void);
  bool operator==(const cSatelliteParams& rhs) const;

  fe_code_rate_t CoderateH(void) const        { return m_coderateH; }
  void SetCoderateH(fe_code_rate_t coderateH) { m_coderateH = coderateH; }

  fe_polarization_t Polarization(void) const           { return m_polarization; }
  void SetPolarization(fe_polarization_t polarization) { m_polarization = polarization; }

  fe_rolloff_t RollOff(void) const      { return m_rollOff; }
  void SetRollOff(fe_rolloff_t rollOff) { m_rollOff = rollOff; }

  unsigned int StreamId(void) const       { return m_streamId; }
  void SetStreamId(unsigned int streamId) { m_streamId = streamId; }

  unsigned int SymbolRateHz(void) const           { return m_symbolRateHz; }
  void SetSymbolRateHz(unsigned int symbolRateHz) { m_symbolRateHz = symbolRateHz; }

  void Serialise(TiXmlElement* elem) const;
  void Deserialise(const TiXmlElement* elem);

private:
  fe_code_rate_t    m_coderateH;
  fe_polarization_t m_polarization;
  fe_rolloff_t      m_rollOff;
  unsigned int      m_streamId;
  unsigned int      m_symbolRateHz;
};

 /*!
  * cTerrestrialParams : Parameters required for over-the-air channels
  *
  *  - Bandwidth
  *  - CoderateH
  *  - CoderateL
  *  - Guard
  *  - Hierarchy
  *  - StreamId
  *  - Transmission
  */
class cTerrestrialParams
{
public:
  cTerrestrialParams(void);
  bool operator==(const cTerrestrialParams& rhs) const;

  fe_bandwidth_t Bandwidth(void) const        { return m_bandwidth; }
  unsigned int BandwidthHz(void) const;
  void SetBandwidth(fe_bandwidth_t bandwidth) { m_bandwidth = bandwidth; }

  fe_code_rate_t CoderateH(void) const        { return m_coderateH; }
  void SetCoderateH(fe_code_rate_t coderateH) { m_coderateH = coderateH; }

  fe_code_rate_t CoderateL(void) const        { return m_coderateL; }
  void SetCoderateL(fe_code_rate_t coderateL) { m_coderateL = coderateL; }

  fe_guard_interval_t Guard(void) const    { return m_guard; }
  void SetGuard(fe_guard_interval_t guard) { m_guard = guard; }

  fe_hierarchy_t Hierarchy(void) const        { return m_hierarchy; }
  void SetHierarchy(fe_hierarchy_t hierarchy) { m_hierarchy = hierarchy; }

  unsigned int StreamId(void) const       { return m_streamId; }
  void SetStreamId(unsigned int streamId) { m_streamId = streamId; }

  fe_transmit_mode_t Transmission(void) const           { return m_transmission; }
  void SetTransmission(fe_transmit_mode_t transmission) { m_transmission = transmission; }

  void Serialise(TiXmlElement* elem) const;
  void Deserialise(const TiXmlElement* elem);

private:
  fe_bandwidth_t      m_bandwidth;
  fe_code_rate_t      m_coderateH;
  fe_code_rate_t      m_coderateL;
  fe_guard_interval_t m_guard;
  fe_hierarchy_t      m_hierarchy;
  unsigned int        m_streamId;
  fe_transmit_mode_t  m_transmission;
};

}
