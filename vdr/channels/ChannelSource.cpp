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

#include "ChannelSource.h"
#include "ChannelDefinitions.h"
#include "utils/StringUtils.h"
#include "utils/log/Log.h"

#include <math.h>
#include <stdlib.h>
#include <tinyxml.h>

// Default?
#define DIRECTION_UNKNOWN  DIRECTION_EAST

namespace VDR
{

/*
 * Round the position to the nearest tenth of a degree. Return value units are
 * 1/10 degree.
 */
int RoundToTenths(float positionDegrees)
{
  return (int)floor(positionDegrees * 10 + 0.5);
}

cChannelSource::cChannelSource(SOURCE_TYPE         source   /* = SOURCE_TYPE_NONE */,
                               float               position /* = 0.0f             */,
                               SATELLITE_DIRECTION dir      /* = DIRECTION_WEST   */)
 : m_sourceType(source),
   m_positionDegrees(dir == DIRECTION_WEST ? position : -position)
{
}

bool cChannelSource::operator==(const cChannelSource& rhs) const
{
  // Only compare position if source is SOURCE_TYPE_SATELLITE, and only compare
  // to 1 decimal point.
  if (m_sourceType != SOURCE_TYPE_SATELLITE)
    return m_sourceType == rhs.m_sourceType;
  else
    return RoundToTenths(m_positionDegrees) == RoundToTenths(rhs.m_positionDegrees);
}

bool cChannelSource::Serialise(TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *sourceElement = node->ToElement();
  if (sourceElement == NULL)
    return false;

  if (m_sourceType != SOURCE_TYPE_NONE)
    sourceElement->SetAttribute(CHANNEL_SOURCE_XML_ATTR_SOURCE, (int)m_sourceType);

  if (m_sourceType == SOURCE_TYPE_SATELLITE)
    sourceElement->SetAttribute(CHANNEL_SOURCE_XML_ATTR_POSITION, m_positionDegrees);

  return true;
}

bool cChannelSource::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  const char *source = elem->Attribute(CHANNEL_SOURCE_XML_ATTR_SOURCE);
  m_sourceType = SOURCE_TYPE_NONE;
  if (source != NULL)
  {
    const SOURCE_TYPE sourceEnum = (SOURCE_TYPE)StringUtils::IntVal(source);

    switch (sourceEnum)
    {
    case SOURCE_TYPE_ATSC:
    case SOURCE_TYPE_CABLE:
    case SOURCE_TYPE_SATELLITE:
    case SOURCE_TYPE_TERRESTRIAL:
    case SOURCE_TYPE_IPTV:
    case SOURCE_TYPE_ANALOG_VIDEO:
      m_sourceType = sourceEnum;
      break;
    }
  }

  if (m_sourceType == SOURCE_TYPE_SATELLITE)
  {
    const char *position = elem->Attribute(CHANNEL_SOURCE_XML_ATTR_POSITION);
    m_positionDegrees = position ? atof(position) : 0.0f;
  }

  return true;
}

std::string cChannelSource::ToString(void) const
{
  switch (m_sourceType)
  {
  case SOURCE_TYPE_SATELLITE:
  {
    const int tenths = RoundToTenths(::fabs(m_positionDegrees));
    return StringUtils::Format("S%d.%d%c", tenths / 10, tenths % 10,
        m_positionDegrees >= 0.0f ? 'W' : 'E');
  }
  case SOURCE_TYPE_ATSC:         return "A";
  case SOURCE_TYPE_CABLE:        return "C";
  case SOURCE_TYPE_TERRESTRIAL:  return "T";
  case SOURCE_TYPE_IPTV:         return "I";
  case SOURCE_TYPE_ANALOG_VIDEO: return "V";
  case SOURCE_TYPE_NONE:
  default:
    return "";
  }
}

void cChannelSource::Deserialise(const std::string& str)
{
  // TODO
}

}
