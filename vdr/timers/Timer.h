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

#include "TimerTypes.h"
#include "channels/ChannelTypes.h"
#include "utils/DateTime.h"
#include "utils/Observer.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <time.h>

class TiXmlNode;

namespace VDR
{

class cTimer : public Observable
{
public:
  /*!
   * Construct a timer with default or specified properties. Timer ID is
   * initialised to TIMER_INVALID_INDEX and must be set via SetID().
   */
  cTimer(void);
  cTimer(const CDateTime&     startTime,
         const CDateTime&     endTime,
         const CDateTimeSpan& lifetime,
         uint8_t              weekdayMask,
         const ChannelPtr&    channel,
         bool                 bActive,
         std::string          strTimerFilename);

  virtual ~cTimer(void) { }

  static TimerPtr EmptyTimer;

  /*!
   * Timer properties
   *   - ID:           Unique identifier for the timer; immutable once set
   *   - Start time:   Date and time of the timer's first occurrence
   *   - End time:     Start time plus duration; NOT the last occurrence of a
   *                   repeating timer
   *   - Lifetime:     Timer encompasses all occurrences that begin before or on
   *                   (start time + lifetime)
   *   - Weekday mask: SMTWRFS, Sunday is LSB; set to 0 for non-repeating event
   *   - Channel:      Channel that this timer is assigned to
   *   - Active:       Timer may be disabled to allow the addition of conflicting
   *                   timers (default value is TRUE)
   *   - Filename:     The filename of the timer is derived from this
   */
  unsigned int     ID(void) const                { return m_id; }
  const CDateTime& StartTime(void) const         { return m_startTime; }
  const CDateTime& EndTime(void) const           { return m_endTime; }
  CDateTimeSpan    Duration(void) const          { return m_endTime - m_startTime; }
  const CDateTimeSpan& Lifetime(void) const      { return m_lifetime; }
  uint8_t          WeekdayMask(void) const       { return m_weekdayMask; }
  bool             IsRepeatingEvent(void) const  { return (m_weekdayMask != 0) ; }
  ChannelPtr       Channel(void) const           { return m_channel; }
  bool             IsActive(void) const          { return m_bActive; }
  std::string      RecordingFilename(void) const { return m_strRecordingFilename; }

  /*!
   * Set the timer ID. This ID should be unique among timers.
   *
   * The ID may only be set once. If the timer already has a valid ID then
   * SetID() has no effect and returns false.
   *
   * Setting the ID does not mark the timer as changed.
   */
  bool SetID(unsigned int id);

  /*!
   * Set timer properties. Properties can be set all at once via operator=(),
   * or sequentially via the Set*() methods. operator=() does not set the ID.
   *
   * Timer will be marked as changed if any properties (excluding ID) are
   * modified.
   */
  cTimer& operator=(const cTimer& rhs);

  void SetTime(const CDateTime& startTime,
               const CDateTime& endTime,
               const CDateTimeSpan& lifetime,
               uint8_t weekdayMask);
  void SetChannel(const ChannelPtr& channel);
  void SetActive(bool bActive);
  void SetRecordingFilename(const std::string& strFile);

  /*!
   * A timer is valid if:
   *   (1) m_id           is a valid ID (i.e. is not TIMER_INVALID_ID)
   *   (2) m_startTime    is a valid CDateTime object
   *   (3) m_endTime      is a valid CDateTime object
   *   (4) m_startTime < m_endTime
   *   (5) m_channel      is a valid channel
   *
   * HOWEVER, the ID check (1) may be skipped by setting bCheckID to false.
   * This allows the timer manager to check validity before assigning an ID.
   */
  bool IsValid(bool bCheckID = true) const;
  void LogInvalidProperties(bool bCheckID = true) const;

  // TODO
  bool IsPending(const CDateTime& now) const   { return now <= m_startTime + m_lifetime; }
  bool IsExpired(const CDateTime& now) const   { return m_startTime + m_lifetime < now; }
  bool IsRecording(const CDateTime& now) const { return false; }

  /*!
   * Serialise the timer. If serialisation succeeds, the serialised timer is
   * guaranteed to be valid (IsValid() returns true); otherwise, false is
   * returned.
   */
  bool Serialise(TiXmlNode* node) const;

  /*!
   * Deserialise the timer. If deserialisation succeeds, the deserialised timer
   * is guaranteed to be valid (IsValid() returns true); otherwise, false is
   * returned.
   *
   * Deserialisation will fail if the timer already has a valid ID.
   *
   * Upon deserialisation, the timer is marked as unchanged. (This mirrors
   * the state of a newly-constructed timer.)
   */
  bool Deserialise(const TiXmlNode* node);

private:
  // Stringify methods
  static std::string WeekdayMaskToString(uint8_t weekdayMask);
  static uint8_t StringToWeekdayMask(const std::string& strWeekdayMask);

  unsigned int  m_id;
  CDateTime     m_startTime;
  CDateTime     m_endTime;
  CDateTimeSpan m_lifetime;
  uint8_t       m_weekdayMask;
  ChannelPtr    m_channel;
  bool          m_bActive;
  std::string   m_strRecordingFilename;
};

}
