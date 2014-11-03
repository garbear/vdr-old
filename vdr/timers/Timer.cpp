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
{
  Reset();
}

cTimer::cTimer(const CDateTime& startTime,
               const CDateTime& endTime,
               const CDateTimeSpan& lifetime,
               uint8_t weekdayMask,
               const ChannelPtr& channel,
               bool bActive,
               std::string strRecordingFilename)
 : m_index(TIMER_INVALID_INDEX),
   m_startTime(startTime),
   m_endTime(endTime),
   m_lifetime(lifetime),
   m_weekdayMask(weekdayMask),
   m_channel(channel),
   m_bActive(bActive),
   m_strRecordingFilename(strRecordingFilename)
{
}

void cTimer::Reset(void)
{
  m_index = TIMER_INVALID_INDEX;
  m_startTime.Reset();
  m_endTime.Reset();
  m_lifetime = CDateTimeSpan();
  m_weekdayMask = 0;
  m_channel.reset();
  m_bActive = true;
  m_strRecordingFilename.clear();
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

bool cTimer::IsValid(void) const
{
  return m_channel && m_channel->ID().IsValid() && // Channel must be valid
         m_startTime.IsValid()                  && // Start time must be valid
         m_startTime < m_endTime;                  // Start time must be < end time (implies end time is valid)
}

bool cTimer::SetIndex(unsigned int index)
{
  if (m_index != TIMER_INVALID_INDEX)
    return false;

  m_index = index;
  return true;
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

bool cTimer::SerialiseTimer(TiXmlNode *node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *timerElement = node->ToElement();
  if (timerElement == NULL)
    return false;

  if (m_index == TIMER_INVALID_INDEX)
  {
    esyslog("Error saving timer: invalid index");
    return false;
  }

  if (!m_channel || !m_channel->ID().IsValid())
  {
    esyslog("Error saving timer %u: invalid channel", m_index);
    return false;
  }

  if (!m_startTime.IsValid())
  {
    esyslog("Error saving timer %u: invalid start time", m_index);
    return false;
  }

  if (m_startTime >= m_endTime)
  {
    esyslog("Error saving timer %u: start time is >= end time", m_index);
    return false;
  }

  assert(IsValid());

  timerElement->SetAttribute(TIMER_XML_ATTR_INDEX, m_index);

  time_t startTime;
  m_startTime.GetAsTime(startTime);
  timerElement->SetAttribute(TIMER_XML_ATTR_START, startTime);

  time_t endTime;
  m_endTime.GetAsTime(endTime);
  timerElement->SetAttribute(TIMER_XML_ATTR_END, endTime);

  timerElement->SetAttribute(TIMER_XML_ATTR_LIFETIME, m_lifetime.GetDays());

  if (m_weekdayMask)
    timerElement->SetAttribute(TIMER_XML_ATTR_DAY_MASK, WeekdayMaskToString(m_weekdayMask));

  if (!m_channel->ID().Serialise(node))
    return false;

  if (!m_bActive)
    timerElement->SetAttribute(TIMER_XML_ATTR_DISABLED, "true");

  timerElement->SetAttribute(TIMER_XML_ATTR_FILENAME, m_strRecordingFilename);

  return true;
}

bool cTimer::DeserialiseTimer(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (m_index != TIMER_INVALID_INDEX)
  {
    esyslog("Error loading timer: timer already has a valid index (%u)", m_index);
    return false;
  }

  Reset();

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_INDEX))
    m_index = StringUtils::IntVal(attr, TIMER_INVALID_INDEX);

  if (m_index == TIMER_INVALID_INDEX)
  {
    esyslog("Error loading timer: invalid index");
    return false;
  }

  cChannelID chanId;
  chanId.Deserialise(node);
  if (chanId.IsValid())
    m_channel = cChannelManager::Get().GetByChannelID(chanId);

  if (!m_channel)
  {
    esyslog("Error loading timer %u: no channel for ID [%s]", m_index, chanId.ToString().c_str());
    return false;
  }

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_START))
    m_startTime = (time_t)StringUtils::IntVal(attr);

  if (!m_startTime.IsValid())
  {
    esyslog("Error loading timer %u: invalid start time", m_index);
    return false;
  }

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_END))
    m_endTime = (time_t)StringUtils::IntVal(attr);

  if (m_startTime >= m_endTime)
  {
    esyslog("Error loading timer %u: start time is >= end time", m_index);
    return false;
  }

  assert(IsValid());

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_LIFETIME))
    m_lifetime = CDateTimeSpan(StringUtils::IntVal(attr), 0, 0, 0);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_DAY_MASK))
    m_weekdayMask = StringToWeekdayMask(attr);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_DISABLED))
    m_bActive = !StringUtils::EqualsNoCase(attr, "true");

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_FILENAME))
    m_strRecordingFilename = attr;

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
