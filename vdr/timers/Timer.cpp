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

#include "Timer.h"
#include "TimerDefinitions.h"
#include "channels/Channel.h"
#include "channels/ChannelManager.h"
#include "epg/Event.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

namespace VDR
{

TimerPtr cTimer::EmptyTimer;

cTimer::cTimer(void)
: m_id(TIMER_INVALID_ID),
  m_weekdayMask(0),
  m_bActive(true)
{
}

cTimer::cTimer(const CDateTime& startTime,
               const CDateTime& endTime,
               const CDateTimeSpan& lifetime,
               uint8_t weekdayMask,
               const ChannelPtr& channel,
               bool bActive,
               std::string strRecordingFilename)
 : m_id(TIMER_INVALID_ID),
   m_startTime(startTime),
   m_endTime(endTime),
   m_lifetime(lifetime),
   m_weekdayMask(weekdayMask),
   m_channel(channel),
   m_bActive(bActive),
   m_strRecordingFilename(strRecordingFilename)
{
}

bool cTimer::SetID(unsigned int id)
{
  if (m_id == TIMER_INVALID_ID)
  {
    m_id = id;
    return true;
  }
  return false;
}

cTimer& cTimer::operator=(const cTimer& rhs)
{
  if (this != &rhs)
  {
    SetTime(rhs.m_startTime, rhs.m_endTime, rhs.m_lifetime, rhs.m_weekdayMask);
    SetChannel(rhs.m_channel);
    SetActive(rhs.m_bActive);
    SetRecordingFilename(rhs.m_strRecordingFilename);
  }
  return *this;
}

void cTimer::SetTime(const CDateTime& startTime, const CDateTime& endTime, const CDateTimeSpan& lifetime, uint8_t weekdayMask)
{
  if (m_startTime   != startTime ||
      m_endTime     != endTime   ||
      m_lifetime    != lifetime  ||
      m_weekdayMask != weekdayMask)
  {
    m_startTime   = startTime;
    m_endTime     = endTime;
    m_lifetime    = lifetime;
    m_weekdayMask = weekdayMask;
    SetChanged();
  }
}

void cTimer::SetChannel(const ChannelPtr& channel)
{
  if (m_channel != channel)
  {
    m_channel = channel;
    SetChanged();
  }
}

void cTimer::SetActive(bool bActive)
{
  if (m_bActive != bActive)
  {
    m_bActive = bActive;
    SetChanged();
  }
}

void cTimer::SetRecordingFilename(const std::string& strFile)
{
  if (m_strRecordingFilename != strFile)
  {
    m_strRecordingFilename = strFile;
    SetChanged();
  }
}

bool cTimer::IsValid(bool bCheckID /* = true */) const
{
  return (bCheckID ? m_id != TIMER_INVALID_ID : true) && // Condition (1)
         m_startTime.IsValid()                        && // Condition (2)
         m_endTime.IsValid()                          && // Condition (3)
         m_startTime < m_endTime                      && // Condition (4)
         m_channel && m_channel->IsValid();              // Condition (5)
}

void cTimer::LogInvalidProperties(bool bCheckID /* = true */) const
{
  // Check condition (1)
  if (bCheckID && m_id == TIMER_INVALID_ID)
    esyslog("Invalid timer: ID has not been set");

  // Check conditions (2) and (3)
  if (!m_startTime.IsValid() || !m_endTime.IsValid())
  {
    esyslog("Invalid timer %u: invalid %s", m_id,
        m_endTime.IsValid() ? "start time" :
            m_startTime.IsValid() ? "end time" :
                "start and end time");
  }

  time_t startTime;
  time_t endTime;

  m_startTime.GetAsTime(startTime);
  m_endTime.GetAsTime(endTime);

  // Check condition (4)
  if (m_startTime >= m_endTime)
    esyslog("Invalid timer %u: start time (%d) is >= end time (%d)", m_id, startTime, endTime);

  // Check condition (5)
  if (!m_channel || !m_channel->IsValid())
    esyslog("Invalid timer %u: invalid channel", m_id);
}

bool cTimer::Serialise(TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement* elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (!IsValid())
  {
    LogInvalidProperties();
    return false;
  }

  elem->SetAttribute(TIMER_XML_ATTR_INDEX, m_id);

  time_t startTime;
  m_startTime.GetAsTime(startTime);
  elem->SetAttribute(TIMER_XML_ATTR_START, startTime);

  time_t endTime;
  m_endTime.GetAsTime(endTime);
  elem->SetAttribute(TIMER_XML_ATTR_END, endTime);

  elem->SetAttribute(TIMER_XML_ATTR_LIFETIME, m_lifetime.GetDays());

  if (m_weekdayMask)
    elem->SetAttribute(TIMER_XML_ATTR_DAY_MASK, WeekdayMaskToString(m_weekdayMask));

  if (!m_channel->ID().Serialise(node))
    return false;

  if (!m_bActive)
    elem->SetAttribute(TIMER_XML_ATTR_DISABLED, "true");

  elem->SetAttribute(TIMER_XML_ATTR_FILENAME, m_strRecordingFilename);

  return true;
}

bool cTimer::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  const TiXmlElement* elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (m_id != TIMER_INVALID_ID)
  {
    esyslog("Error loading timer: object already has a valid index (%u)", m_id);
    return false;
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_INDEX);
    m_id = attr ? StringUtils::IntVal(attr, TIMER_INVALID_ID) : 0;
  }

  cChannelID chanId;
  chanId.Deserialise(node);
  m_channel = cChannelManager::Get().GetByChannelID(chanId);

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_START);
    m_startTime = (time_t)(attr ? StringUtils::IntVal(attr) : 0);
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_END);
    m_endTime = (time_t)(attr ? StringUtils::IntVal(attr) : 0);
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_LIFETIME);
    m_lifetime.SetDateTimeSpan(0, 0, 0, attr ? StringUtils::IntVal(attr) : 0);
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_DAY_MASK);
    m_weekdayMask = attr ? StringToWeekdayMask(attr) : 0;
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_DISABLED);
    const bool bDisabled = (attr ? StringUtils::EqualsNoCase(attr, "true") : false);
    m_bActive = !bDisabled;
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_FILENAME);
    m_strRecordingFilename = attr ? attr : "";
  }

  if (!IsValid())
  {
    LogInvalidProperties();
    return false;
  }

  SetChanged(false);

  return true;
}

std::string cTimer::WeekdayMaskToString(uint8_t weekdayMask)
{
  std::string strWeekdayMask = "_______";
  if (weekdayMask & TIMER_MASK_SUNDAY)    strWeekdayMask[0] = 'S';
  if (weekdayMask & TIMER_MASK_MONDAY)    strWeekdayMask[1] = 'M';
  if (weekdayMask & TIMER_MASK_TUESDAY)   strWeekdayMask[2] = 'T';
  if (weekdayMask & TIMER_MASK_WEDNESDAY) strWeekdayMask[3] = 'W';
  if (weekdayMask & TIMER_MASK_THURSDAY)  strWeekdayMask[4] = 'R';
  if (weekdayMask & TIMER_MASK_FRIDAY)    strWeekdayMask[5] = 'F';
  if (weekdayMask & TIMER_MASK_SATURDAY)  strWeekdayMask[6] = 'S';
  return strWeekdayMask;
}

uint8_t cTimer::StringToWeekdayMask(const std::string& strWeekdayMask)
{
  if (strWeekdayMask.length() != 7)
    return 0;

  return (strWeekdayMask[0] == 'S' ? TIMER_MASK_SUNDAY    : 0) |
         (strWeekdayMask[1] == 'M' ? TIMER_MASK_MONDAY    : 0) |
         (strWeekdayMask[2] == 'T' ? TIMER_MASK_TUESDAY   : 0) |
         (strWeekdayMask[3] == 'W' ? TIMER_MASK_WEDNESDAY : 0) |
         (strWeekdayMask[4] == 'R' ? TIMER_MASK_THURSDAY  : 0) |
         (strWeekdayMask[5] == 'F' ? TIMER_MASK_FRIDAY    : 0) |
         (strWeekdayMask[6] == 'S' ? TIMER_MASK_SATURDAY  : 0);
}

}
