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
#include "devices/DeviceManager.h"
#include "epg/Event.h"
#include "recordings/Recording.h"
#include "recordings/RecordingManager.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <assert.h>

namespace VDR
{

TimerPtr cTimer::EmptyTimer;

static const CDateTimeSpan ONE_DAY = CDateTimeSpan(1, 0, 0, 0);

// --- cTimerSorter ------------------------------------------------------------

bool cTimerSorter::operator()(const TimerPtr& lhs, const TimerPtr& rhs) const
{
  // Guarantee that expired timers come before occurring timers
  if (lhs->IsExpired(m_now) && rhs->IsOccurring(m_now)) return true;
  if (lhs->IsOccurring(m_now) && rhs->IsExpired(m_now)) return false;

  return lhs->GetSortOccurrence(m_now) < rhs->GetSortOccurrence(m_now);
}

// --- cTimer::const_iterator --------------------------------------------------

cTimer::const_iterator::const_iterator(void)
 : m_weekdayMask(0)
{
}

cTimer::const_iterator::const_iterator(const CDateTime& startTime,
                                       const CDateTime& endTime,
                                       const CDateTime& expires,
                                       uint8_t          weekdayMask)
 : m_initialStartTime(startTime),
   m_startTime(startTime),
   m_endTime(endTime),
   m_expires(expires),
   m_weekdayMask(weekdayMask)
{
}

bool cTimer::const_iterator::operator==(const const_iterator& rhs) const
{
  return (m_startTime == rhs.m_startTime) || (IsExpired() && rhs.IsExpired());
}

cTimer::const_iterator& cTimer::const_iterator::operator++(void)
{
  while (!IsExpired())
  {
    m_startTime += ONE_DAY;
    m_endTime   += ONE_DAY;

    if (!IsRepeatingEvent() || OccursOnWeekday(m_startTime.GetDayOfWeek()))
      break;
  }

  return *this;
}

cTimer::const_iterator& cTimer::const_iterator::operator--(void)
{
  while (m_startTime > m_initialStartTime)
  {
    m_startTime -= ONE_DAY;
    m_endTime   -= ONE_DAY;

    if (!IsRepeatingEvent() || OccursOnWeekday(m_startTime.GetDayOfWeek()))
      break;
  }

  return *this;
}

// --- cTimer ------------------------------------------------------------------

cTimer::cTimer(void)
 : m_id(TIMER_INVALID_ID),
   m_weekdayMask(0),
   m_bActive(true)
{
}

cTimer::cTimer(const CDateTime&   startTime,
               const CDateTime&   endTime,
               uint8_t            weekdayMask,
               const CDateTime&   deadline,
               const ChannelPtr&  channel,
               const std::string& strFilename,
               bool               bActive)
 : m_id(TIMER_INVALID_ID),
   m_strFilename(strFilename),
   m_startTime(startTime),
   m_endTime(endTime),
   m_weekdayMask(weekdayMask & 0x7f),
   m_expires(GetExpiration(startTime, m_weekdayMask, deadline)),
   m_channel(channel),
   m_bActive(bActive),
   m_begin(m_startTime, m_endTime, m_expires, m_weekdayMask),
   m_end(m_expires + ONE_DAY, m_endTime, m_expires, m_weekdayMask)
{
}

bool cTimer::SetID(unsigned int id)
{
  PLATFORM::CLockObject lock(m_mutex);
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
    PLATFORM::CLockObject lock(m_mutex);

    SetFilename(rhs.m_strFilename);
    SetTime(rhs.m_startTime, rhs.m_endTime, rhs.m_weekdayMask, rhs.m_expires);
    SetChannel(rhs.m_channel);
    SetActive(rhs.m_bActive);
  }
  return *this;
}

CDateTime cTimer::GetExpiration(const CDateTime& startTime, uint8_t weekdayMask, const CDateTime& deadline)
{
  CDateTime last = startTime;

  const bool bIsRepeating = (weekdayMask != 0);
  if (bIsRepeating)
  {
    // Fast forward to the deadline
    while (last + ONE_DAY <= deadline)
      last += ONE_DAY;

    // Rewind until we satisfy the weekday mask, but don't rewind past start time
    while ((weekdayMask & (1 << last.GetDayOfWeek()) == 0) && (last - ONE_DAY) > startTime)
      last -= ONE_DAY;
  }

  return last;
}

void cTimer::SetFilename(const std::string& strFilename)
{
  PLATFORM::CLockObject lock(m_mutex);

  if (m_strFilename != strFilename)
  {
    m_strFilename = strFilename;
    SetChanged();
  }
}

void cTimer::SetTime(const CDateTime& startTime, const CDateTime& endTime, uint8_t weekdayMask, const CDateTime& deadline)
{
  weekdayMask &= 0x7f;
  CDateTime expires = GetExpiration(startTime, weekdayMask, deadline);

  PLATFORM::CLockObject lock(m_mutex);

  if (m_startTime   != startTime   ||
      m_endTime     != endTime     ||
      m_weekdayMask != weekdayMask ||
      m_expires     != expires)
  {
    m_startTime   = startTime;
    m_endTime     = endTime;
    m_weekdayMask = weekdayMask;
    m_expires     = expires;
    m_begin       = const_iterator(m_startTime, m_endTime, m_expires, m_weekdayMask);
    m_end         = const_iterator(m_expires + ONE_DAY, m_endTime, m_expires, m_weekdayMask);
    SetChanged();
  }
}

void cTimer::SetChannel(const ChannelPtr& channel)
{
  PLATFORM::CLockObject lock(m_mutex);

  if (m_channel != channel)
  {
    m_channel = channel;
    SetChanged();
  }
}

void cTimer::SetActive(bool bActive)
{
  PLATFORM::CLockObject lock(m_mutex);

  if (m_bActive != bActive)
  {
    m_bActive = bActive;
    SetChanged();
  }
}

bool cTimer::IsValid(bool bCheckID /* = true */) const
{
  PLATFORM::CLockObject lock(m_mutex);

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

bool cTimer::IsOccurring(const CDateTime& now) const
{
  for (const_iterator it = begin(); it != end(); ++it)
  {
    if (it.StartTime() <= now && now < it.EndTime())
      return true;
  }
  return false;
}

CDateTime cTimer::GetSortOccurrence(const CDateTime& now) const
{
  PLATFORM::CLockObject lock(m_mutex);

  if (IsExpired(now))
    return m_expires;

  // Timer is occurring or pending
  const_iterator it = begin();
  while (it.EndTime() <= now)
    ++it;
  return it.StartTime();
}

bool cTimer::Conflicts(const cTimer& timer) const
{
  for (const_iterator it = begin(); it != end(); ++it)
  {
    if (timer.IsExpired(it.StartTime()))
      return false;

    if (timer.IsOccurring(it.StartTime()))
      return true;

    CDateTime pendingTime = timer.GetSortOccurrence(it.StartTime());
    if (pendingTime < it.EndTime())
      return  true;
  }

  return false;
}

void cTimer::StartRecording(void)
{
  PLATFORM::CLockObject lock(m_mutex);

  const CDateTime now = CDateTime::GetUTCDateTime();

  if (!IsActive() || !IsOccurring(now))
    return;

  // If a recording has already been started, retrieve it from cRecordingManager
  if (!IsRecording(now))
    m_recording = cRecordingManager::Get().GetByChannel(m_channel, now);

  if (IsRecording(now))
  {
    m_recording->Resume();
  }
  else
  {
    const CDateTime startTime = GetSortOccurrence(now);
    m_recording = RecordingPtr(new cRecording(m_strFilename,
                                              "TEST", // TODO
                                              m_channel,
                                              startTime,
                                              startTime + Duration()));
    cRecordingManager::Get().AddRecording(m_recording);
  }
}

void cTimer::StopRecording(void)
{
  PLATFORM::CLockObject lock(m_mutex);

  if (IsRecording(CDateTime::GetUTCDateTime()))
    m_recording->Interrupt();
}

bool cTimer::IsRecording(const CDateTime& now) const
{
  PLATFORM::CLockObject lock(m_mutex);

  return m_recording && m_recording->IsValid() &&
         m_recording->StartTime() <= now && now < m_recording->EndTime();
}

bool cTimer::Serialise(TiXmlNode* node) const
{
  PLATFORM::CLockObject lock(m_mutex);

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

  elem->SetAttribute(TIMER_XML_ATTR_FILENAME, m_strFilename);

  time_t startTime;
  m_startTime.GetAsTime(startTime);
  elem->SetAttribute(TIMER_XML_ATTR_START, startTime);

  time_t endTime;
  m_endTime.GetAsTime(endTime);
  elem->SetAttribute(TIMER_XML_ATTR_END, endTime);

  if (m_weekdayMask)
    elem->SetAttribute(TIMER_XML_ATTR_DAY_MASK, WeekdayMaskToString(m_weekdayMask));

  time_t tmExpires;
  m_expires.GetAsTime(tmExpires);
  if (tmExpires > 0)
    elem->SetAttribute(TIMER_XML_ATTR_EXPIRES, tmExpires);

  if (!m_channel->ID().Serialise(node))
    return false;

  if (!m_bActive)
    elem->SetAttribute(TIMER_XML_ATTR_DISABLED, "true");

  return true;
}

bool cTimer::Deserialise(const TiXmlNode* node)
{
  PLATFORM::CLockObject lock(m_mutex);

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
    m_id = attr ? StringUtils::IntVal(attr, TIMER_INVALID_ID) : TIMER_INVALID_ID;
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_FILENAME);
    m_strFilename = attr ? attr : "";
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_START);
    m_startTime = (time_t)(attr ? StringUtils::IntVal(attr) : 0);
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_END);
    m_endTime = (time_t)(attr ? StringUtils::IntVal(attr) : 0);
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_DAY_MASK);
    m_weekdayMask = attr ? StringToWeekdayMask(attr) : 0;
  }

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_EXPIRES);
    m_expires = (time_t)(attr ? StringUtils::IntVal(attr) : 0);
  }

  cChannelID chanId;
  chanId.Deserialise(node);
  m_channel = cChannelManager::Get().GetByChannelID(chanId);

  {
    const char* attr = elem->Attribute(TIMER_XML_ATTR_DISABLED);
    const bool bDisabled = (attr ? StringUtils::EqualsNoCase(attr, "true") : false);
    m_bActive = !bDisabled;
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
