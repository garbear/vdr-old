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

#include "../../../sourceparams.h"

#include <string>

#define DVB_SYSTEM_1 0 // see also nit.c
#define DVB_SYSTEM_2 1

struct tDvbParameterMap
{
  int         userValue;
  int         driverValue;
  std::string userString;
};

extern const tDvbParameterMap InversionValues[];
extern const tDvbParameterMap BandwidthValues[];
extern const tDvbParameterMap CoderateValues[];
extern const tDvbParameterMap ModulationValues[];
extern const tDvbParameterMap SystemValuesSat[];
extern const tDvbParameterMap SystemValuesTerr[];
extern const tDvbParameterMap TransmissionValues[];
extern const tDvbParameterMap GuardValues[];
extern const tDvbParameterMap HierarchyValues[];
extern const tDvbParameterMap RollOffValues[];

class DvbParameters
{
  static std::string MapToUserString(int value, const tDvbParameterMap *map);
  static int         MapToUser(int value, const tDvbParameterMap *map, std::string &strString);
  static int         MapToDriver(int value, const tDvbParameterMap *map);
  static int         UserIndex(int value, const tDvbParameterMap *map);
  static int         DriverIndex(int value, const tDvbParameterMap *map);
};

class cDvbTransponderParameters
{
  friend class cDvbSourceParam;

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

  cString ToString(char type) const;
  bool Parse(const char *s);

private:
  int PrintParameter(char *p, char name, int value) const;
  const char *ParseParameter(const char *s, int &value, const tDvbParameterMap *map = NULL);

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

class cOsdItem;

class cDvbSourceParam : public cSourceParam
{
public:
  cDvbSourceParam(char source, const std::string &description);
  virtual void SetData(const cChannel &channel);
  virtual void GetData(cChannel &channel) const;
  virtual cOsdItem *GetOsdItem();

  // TODO: Change in base class
  virtual void SetData(cChannel *channel) { return SetData(*channel); }
  virtual void GetData(cChannel *channel) { return GetData(*channel); }

private:
  cDvbTransponderParameters m_dtp;
  int m_param;
  int m_srate;
};
