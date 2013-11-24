/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "sources/ISourceParams.h"

#include <linux/dvb/frontend.h>
#include <string>

#define DVB_SYSTEM_1  0 // see also nit.c
#define DVB_SYSTEM_2  1

class cDvbTransponderParams
{
public:
  /*!
   * \brief Create a new cDvbTransponderParams object with default values
   * \param strParameters If given, the string will be deserialized, overriding
   *        the members' default values
   */
  cDvbTransponderParams(const std::string &strParameters = "");

  char Polarization() const { return m_polarization; }
  void SetPolarization(char polarization) { m_polarization = polarization; }

  int Inversion() const { return m_inversion; }
  void SetInversion(int inversion) { m_inversion = inversion; }

  int Bandwidth() const { return m_bandwidth; }
  void SetBandwidth(int bandwidth) { m_bandwidth = bandwidth; }

  int CoderateH() const { return m_coderateH; }
  void SetCoderateH(int coderateH) { m_coderateH = coderateH; }

  int CoderateL() const { return m_coderateL; }
  void SetCoderateL(int coderateL) { m_coderateL = coderateL; }

  int Modulation() const { return m_modulation; }
  void SetModulation(int modulation) { m_modulation = modulation; }

  int System() const { return m_system; }
  void SetSystem(int system) { m_system = system; }

  int Transmission() const { return m_transmission; }
  void SetTransmission(int transmission) { m_transmission = transmission; }

  int Guard() const { return m_guard; }
  void SetGuard(int guard) { m_guard = guard; }

  int Hierarchy() const { return m_hierarchy; }
  void SetHierarchy(int hierarchy) { m_hierarchy = hierarchy; }

  int RollOff() const { return m_rollOff; }
  void SetRollOff(int rollOff) { m_rollOff = rollOff; }

  int StreamId() const { return m_streamId; }
  void SetStreamId(int streamId) { m_streamId = streamId; }

  /*!
   * \brief Serialize the object to a string using a confusing sequence of
   *        capital letters and variable-length numbers
   * \param type 'A', 'C', 'S' or 'T', presumably for the DVB type?
   * \return Serialized string, or empty on invalid type
   */
  std::string Serialize(char type) const;

  /*!
   * \brief Deserialize a serialized string
   * \param str The serialized representation of a cDvbTransponderParams object
   * \return True on success, or false if deserialization fails (the object may
   *         still be modified in this case)
   */
  bool Deserialize(const std::string &str);

  /*!
   * \brief Translate a modulation enum value into its string representation
   * \param modulation The modulation value, e.g. QAM_256
   * \return The string representation, e.g. "QAM256"
   */
  static const char *TranslateModulation(fe_modulation modulation);

private:
  char m_polarization;
  int  m_inversion;
  int  m_bandwidth;
  int  m_coderateH;
  int  m_coderateL;
  int  m_modulation;
  int  m_system;
  int  m_transmission;
  int  m_guard;
  int  m_hierarchy;
  int  m_rollOff;
  int  m_streamId;
};

class cChannel;

class cDvbSourceParams : public iSourceParams
{
public:
  cDvbSourceParams(char source, const std::string &description);

  /*!
   * \brief Sets all source specific parameters to those of the given Channel
   * */
  virtual void SetData(const cChannel &channel);

  /*!
   * \brief Copies all source specific parameters to the given Channel
   */
  virtual void GetData(cChannel &channel) const;

  //virtual cOsdItem *GetOsdItem();

private:
  cDvbTransponderParams m_dtp;
  int                   m_srate;
};
