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

#include "dvb/extended_frontend.h"

#include <string>
#include <vector>

class TiXmlNode;

namespace VDR
{

// TODO: Remove this enum and store system directly using fe_delivery_system
enum eSystemType
{
  DVB_SYSTEM_1 = 0, // SYS_DVBS or SYS_DVBT
  DVB_SYSTEM_2 = 1  // SYS_DVBS2 or SYS_DVBT2
};

// TODO: Copied from ScanConfig.h
enum eDvbType
{
  DVB_TERR,
  DVB_CABLE,
  DVB_SAT,
  DVB_ATSC,
};

class cDvbTransponder
{
public:
  cDvbTransponder(fe_polarization       polarization = POLARIZATION_HORIZONTAL,
                  fe_spectral_inversion inversion    = INVERSION_AUTO,
                  fe_bandwidth          bandwidth    = BANDWIDTH_8_MHZ,
                  fe_code_rate          coderateH    = FEC_AUTO,
                  fe_code_rate          coderateL    = FEC_AUTO,
                  fe_modulation         modulation   = QPSK,
                  eSystemType           system       = DVB_SYSTEM_1,
                  fe_transmit_mode      transmission = TRANSMISSION_MODE_AUTO,
                  fe_guard_interval     guard        = GUARD_INTERVAL_AUTO,
                  fe_hierarchy          hierarchy    = HIERARCHY_AUTO,
                  fe_rolloff            rollOff      = ROLLOFF_AUTO);

  bool operator==(const cDvbTransponder& rhs) const;
  bool operator!=(const cDvbTransponder& rhs) const { return !(*this == rhs); }

  void Reset();

  fe_polarization Polarization() const { return m_polarization; }
  void SetPolarization(fe_polarization polarization) { m_polarization = polarization; }

  fe_spectral_inversion Inversion() const { return m_inversion; }
  void SetInversion(fe_spectral_inversion inversion) { m_inversion = inversion; }

  fe_bandwidth Bandwidth() const { return m_bandwidth; }
  unsigned int BandwidthHz() const;
  void SetBandwidth(fe_bandwidth bandwidth) { m_bandwidth = bandwidth; }

  fe_code_rate CoderateH() const { return m_coderateH; }
  void SetCoderateH(fe_code_rate coderateH) { m_coderateH = coderateH; }

  fe_code_rate CoderateL() const { return m_coderateL; }
  void SetCoderateL(fe_code_rate coderateL) { m_coderateL = coderateL; }

  fe_modulation Modulation() const { return m_modulation; }
  void SetModulation(fe_modulation modulation) { m_modulation = modulation; }

  eSystemType System() const { return m_system; }
  void SetSystem(eSystemType system) { m_system = system; }

  fe_transmit_mode Transmission() const { return m_transmission; }
  void SetTransmission(fe_transmit_mode transmission) { m_transmission = transmission; }

  fe_guard_interval Guard() const { return m_guard; }
  void SetGuard(fe_guard_interval guard) { m_guard = guard; }

  fe_hierarchy Hierarchy() const { return m_hierarchy; }
  void SetHierarchy(fe_hierarchy hierarchy) { m_hierarchy = hierarchy; }

  fe_rolloff RollOff() const { return m_rollOff; }
  void SetRollOff(fe_rolloff rollOff) { m_rollOff = rollOff; }

  unsigned int StreamId() const { return m_streamId; }
  void SetStreamId(unsigned int streamId) { m_streamId = streamId; }

  bool Serialise(eDvbType type, TiXmlNode* node) const;
  bool Deserialise(const TiXmlNode* node);
  bool Deserialise(const std::string& strParameters);

  /*!
   * \brief Translate a modulation enum value into its string representation
   * \param modulation The modulation value, e.g. QAM_256
   * \return The string representation, e.g. "QAM256"
   */
  static const char *TranslateModulation(fe_modulation modulation);

  static std::vector<std::string> GetModulationsFromCaps(fe_caps_t caps);

private:
  fe_polarization       m_polarization;
  fe_spectral_inversion m_inversion;
  fe_bandwidth          m_bandwidth;
  fe_code_rate          m_coderateH;
  fe_code_rate          m_coderateL;
  fe_modulation         m_modulation;
  eSystemType           m_system; // TODO: Change this to fe_delivery_system
  fe_transmit_mode      m_transmission;
  fe_guard_interval     m_guard;
  fe_hierarchy          m_hierarchy;
  fe_rolloff            m_rollOff;
  unsigned int          m_streamId;
};

}
