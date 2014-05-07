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

class TiXmlNode;

namespace VDR
{

enum SOURCE_TYPE
{
  SOURCE_TYPE_NONE = 0,
  SOURCE_TYPE_ATSC,
  SOURCE_TYPE_CABLE,
  SOURCE_TYPE_SATELLITE,
  SOURCE_TYPE_TERRESTRIAL,
  SOURCE_TYPE_IPTV,
  SOURCE_TYPE_ANALOG_VIDEO
};

enum SATELLITE_DIRECTION
{
  DIRECTION_EAST,
  DIRECTION_WEST
};

/*!
 * Channel sources in VDR were single letters (A, C, T, I, V) or S followed by
 * the satellite's position and direction - see sources.conf:
 *
 * http://projects.vdr-developer.org/git/vdr.git/tree/sources.conf?h=stable/2.0
 *
 * Now they are represented by a class wrapping the SOURCE_TYPE enum. A
 * cChannelSource is constructed from the enum value, optionally allowing
 * satellite position and direction for SOURCE_TYPE_SATELLITE.
 */
class cChannelSource
{
public:
  /*!
   * Create a new source object. If satellite, the position parameter is used
   * to identify the satellite source. positive position specifies degrees west,
   * unless DIRECTION_EAST is given.
   */
  cChannelSource(SOURCE_TYPE         source   = SOURCE_TYPE_NONE,
                 float               position = 0.0f,
                 SATELLITE_DIRECTION dir      = DIRECTION_WEST);

  /*!
   * operator==() is overloaded, so sources can be directly compared to enum
   * values (however, satellite position and direction can't be compared in
   * this way):
   *
   * cChannelSource source(SOURCE_TYPE_SATELLITE, 45, DIRECTION_WEST);
   * if (source == SOURCE_TYPE_SATELLITE)
   *   ...
   *
   * To compare a satellite's position and direction as well, use:
   *
   * if (source == cChannelSource(SOURCE_TYPE_SATELLITE, 100, DIRECTION_EAST)
   *   ...
   */
  bool operator==(SOURCE_TYPE rhs)           const { return m_sourceType == rhs; }
  bool operator!=(SOURCE_TYPE rhs)           const { return !(*this == rhs); }
  bool operator==(const cChannelSource& rhs) const;
  bool operator!=(const cChannelSource& rhs) const { return !(*this == rhs); }

  // Allow channel sources to be used as keys
  bool operator<(const cChannelSource& rhs) const { return m_sourceType      < rhs.m_sourceType  &&
                                                           m_positionDegrees < rhs.m_positionDegrees; }
  bool operator>(const cChannelSource& rhs) const { return !(*this < rhs); }

  bool Serialise(TiXmlNode* node) const;
  bool Deserialise(const TiXmlNode* node);

  /*!
   * Convert this channel source to a string. Guaranteed to be a valid filename
   * (no spaces or slashes), or empty if source == SOURCE_TYPE_NONE.
   */
  std::string ToString(void) const;
  void Deserialise(const std::string& str); // TODO: Not implemented, remove me

private:
  SOURCE_TYPE         m_sourceType;
  float               m_positionDegrees; // Positive in west direction
                                         // Only used if source == SOURCE_TYPE_SATELLITE
};

}
