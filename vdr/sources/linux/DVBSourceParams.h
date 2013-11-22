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

#include <string>
#include <vector>

#define DVB_SYSTEM_1 0 // see also nit.c
#define DVB_SYSTEM_2 1

struct tDvbParameter
{
  int         userValue;
  int         driverValue;
  std::string userString;
};

typedef std::vector<const tDvbParameter*> DvbParameterVector;

class cDvbParameters
{
  static std::string MapToUserString(int value, const DvbParameterVector &vecParams);
  static int         MapToUser(int value, const DvbParameterVector &vecParams, std::string &strString); // TODO: strString?
  static int         MapToDriver(int value, const DvbParameterVector &vecParams);

  static const DvbParameterVector &InversionValues();
  static const DvbParameterVector &BandwidthValues();
  static const DvbParameterVector &CoderateValues();
  static const DvbParameterVector &ModulationValues();
  static const DvbParameterVector &SystemValuesSat();
  static const DvbParameterVector &SystemValuesTerr();
  static const DvbParameterVector &TransmissionValues();
  static const DvbParameterVector &GuardValues();
  static const DvbParameterVector &HierarchyValues();
  static const DvbParameterVector &RollOffValues();

private:
  /*!
   * \brief Get the tDvbParameterMap from the vector with a matching value
   * \param value The value to match
   * \param vecMap A vector of tDvbParameterMap pointers (returned from the *Values() functions below)
   * \param map The map, if discovered
   * \return True if the map was discovered, false if value didn't match any map values
   */
  //static bool        DriverIndex(int value, const DvbParameterVector &vecMap, tDvbParameter &map);

  //static int         UserIndex(int value, const DvbParameterVector &vecParams);
};

class cDvbTransponderParameters
{
  friend class cDvbSourceParams;

public:
  cDvbTransponderParameters(const std::string &strParameters = "");

  char Polarization(void) const { return m_polarization; }
  void SetPolarization(char polarization) { m_polarization = polarization; }

  int Inversion(void) const { return m_inversion; }
  void SetInversion(int inversion) { m_inversion = inversion; }

  int Bandwidth(void) const { return m_coderateH; }
  void SetBandwidth(int bandwidth) { m_bandwidth = bandwidth; }

  int CoderateH(void) const { return m_coderateH; }
  void SetCoderateH(int coderateH) { m_coderateH = coderateH; }

  int CoderateL(void) const { return m_coderateL; }
  void SetCoderateL(int coderateL) { m_coderateL = coderateL; }

  int Modulation(void) const { return m_modulation; }
  void SetModulation(int modulation) { m_modulation = modulation; }

  int System(void) const { return m_system; }
  void SetSystem(int system) { m_system = system; }

  int Transmission(void) const { return m_transmission; }
  void SetTransmission(int transmission) { m_transmission = transmission; }

  int Guard(void) const { return m_guard; }
  void SetGuard(int guard) { m_guard = guard; }

  int Hierarchy(void) const { return m_hierarchy; }
  void SetHierarchy(int hierarchy) { m_hierarchy = hierarchy; }

  int RollOff(void) const { return m_rollOff; }
  void SetRollOff(int rollOff) { m_rollOff = rollOff; }

  int StreamId(void) const { return m_streamId; }
  void SetStreamId(int streamId) { m_streamId = streamId; }

  /*!
   * \brief Serialize the object to a string using a confusing sequence of
   *        capital letters and variable-length numbers
   */
  std::string Serialize(char type) const;
  bool Deserialize(const std::string &str);

private:
  /*!
   * \brief Return a string of name + value
   * \param name Parameter name (capital letter)
   * \param value Parameter value (in the range 0..999)
   * \return The concatenated string, or empty if value lays outside the range
   */
  static std::string PrintParameter(char name, int value);

  /*!
   * \brief Return the value of the first serialized parameter pair in a string
   * \param paramString The serialized parameter pairs
   * \param value The discovered parameter value
   * \param map If map is provided, ParseFirstParameter will interpret the value as the index of a map value
   * \return The remainder of the string after the parameter pair, or empty on failure
   */
  static std::string ParseFirstParameter(const std::string &paramString, int &value);
  static std::string ParseFirstParameter(const std::string &paramString, int &value, const DvbParameterVector &vecParams);

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
class cOsdItem;

class cDvbSourceParams : public iSourceParams
{
public:
  cDvbSourceParams(char source, const std::string &description);

  /*!
   * \brief Sets all source specific parameters to those of the given Channel
   *
   * Must also reset a counter to use with later calls to GetOsdItem().
   * */
  virtual void SetData(const cChannel &channel);

  /*!
   * \brief Copies all source specific parameters to the given Channel
   */
  virtual void GetData(cChannel &channel) const;

  //virtual cOsdItem *GetOsdItem();

private:
  cDvbTransponderParameters m_dtp;
  int m_param;
  int m_srate;
};
