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

#include <string>

namespace VDR
{

class cSource
{
public:
  enum eSourceType
  {
    stNone  = 0x00000000,
    stAtsc  = ('A' << 24),
    stCable = ('C' << 24),
    stSat   = ('S' << 24),
    stTerr  = ('T' << 24),
    st_Mask = 0xFF000000,
    st_Pos  = 0x0000FFFF,
  };

  enum eSatelliteDirection
  {
    sdEast,
    sdWest,
  };

  cSource();
  cSource(char source, const std::string &strDescription);
  ~cSource() { }

  int Code() const { return m_code; }
  const std::string &Description() const { return m_strDescription; }

  bool Parse(const std::string &str);

  static char ToChar(int code) { return (code & st_Mask) >> 24; }
  static std::string ToString(int code);

  static int FromString(const std::string &str);
  static int FromData(eSourceType sourceType, int position = 0, eSatelliteDirection dir = sdWest);

  static bool IsAtsc(int code)              { return (code & st_Mask) == stAtsc; }
  static bool IsCable(int code)             { return (code & st_Mask) == stCable; }
  static bool IsSat(int code)               { return (code & st_Mask) == stSat; }
  static bool IsTerr(int code)              { return (code & st_Mask) == stTerr; }
  static bool IsType(int code, char source) { return (int)(code & st_Mask) == (((int)source) << 24); }

private:
  int         m_code;
  std::string m_strDescription;
};

// TODO: I need a home!
//#include <map>
//std::map<int, cSource> gSources;

}
