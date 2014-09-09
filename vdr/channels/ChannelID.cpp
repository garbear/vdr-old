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

#include "ChannelID.h"
#include "ChannelDefinitions.h"
#include "utils/CRC32.h"
#include "utils/StringUtils.h"

#include <tinyxml.h>

namespace VDR
{

const cChannelID cChannelID::InvalidID;

cChannelID::cChannelID(uint16_t nid  /* = 0 */,
                       uint16_t tsid /* = 0 */,
                       uint16_t sid  /* = 0 */)
 : m_nid(nid),
   m_tsid(tsid),
   m_sid(sid),
   m_atscSourceId(ATSC_SOURCE_ID_NONE)
{
}

bool cChannelID::operator==(const cChannelID &rhs) const
{
  return m_nid          == rhs.m_nid  &&
         m_tsid         == rhs.m_tsid &&
         m_sid          == rhs.m_sid  &&
         m_atscSourceId == rhs.m_atscSourceId;
}

bool cChannelID::IsValid(void) const
{
  return (m_nid != 0 || m_tsid != 0) && m_sid != 0;
}

void cChannelID::SetID(uint16_t nid, uint16_t tsid, uint16_t sid)
{
  m_nid  = nid;
  m_tsid = tsid;
  m_sid  = sid;
}

void cChannelID::SetATSCSourceID(uint16_t sourceId)
{
  m_atscSourceId = sourceId;
}

bool cChannelID::Serialise(TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (m_nid != 0)
    elem->SetAttribute(CHANNEL_ID_XML_ATTR_NID, m_nid);

  if (m_tsid != 0)
    elem->SetAttribute(CHANNEL_ID_XML_ATTR_TSID, m_tsid);

  if (m_sid != 0)
    elem->SetAttribute(CHANNEL_ID_XML_ATTR_SID, m_sid);

  if (m_atscSourceId >= 0)
    elem->SetAttribute(CHANNEL_ID_XML_ATTR_ATSC_SID, m_atscSourceId);

  return true;
}

bool cChannelID::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  const char *nid = elem->Attribute(CHANNEL_ID_XML_ATTR_NID);
  m_nid = nid ? StringUtils::IntVal(nid) : 0;

  const char *tsid = elem->Attribute(CHANNEL_ID_XML_ATTR_TSID);
  m_tsid = tsid ? StringUtils::IntVal(tsid) : 0;

  const char *sid = elem->Attribute(CHANNEL_ID_XML_ATTR_SID);
  m_sid = sid ? StringUtils::IntVal(sid) : 0;

  const char *source = elem->Attribute(CHANNEL_ID_XML_ATTR_ATSC_SID);
  m_atscSourceId = source ? StringUtils::IntVal(source) : ATSC_SOURCE_ID_NONE;

  return true;
}

uint32_t cChannelID::Hash(void) const
{
  return CCRC32::CRC32(ToString());
}

std::string cChannelID::ToString(void) const
{
  return StringUtils::Format("%u-%u-%u", m_nid, m_tsid, m_sid);
}

}
