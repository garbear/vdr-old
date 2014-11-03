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
   * Construct a timer with default/specified params. Timer index is set to
   * TIMER_INVALID_INDEX.
   *
   * A timer is considered unchanged (Observable::Changed() returns false) for
   * newly constructed/deserialised timers. Assigning an index does not mark
   * time timer as changed. Once assigned, an index cannot be changed.
   */
  cTimer(void);
  cTimer(const CDateTime&     startTime,
         const CDateTime&     endTime,
         const CDateTimeSpan& lifetime,
         uint8_t              weekdayMask,
         const ChannelPtr&    channel,
         bool                 bActive,
         std::string          strRecordingFilename);

  virtual ~cTimer(void) { }

  static TimerPtr EmptyTimer;

  /*!
   * Set timer params (excluding index) to match the specified timer. Timer is
   * marked as changed if any params are modified.
   */
  cTimer& operator=(const cTimer& rhs);

  /*!
   * Timer params
   *   - Index:      Unique timer ID, monotonically increases as new timers are
   *                 added. Initialised as TIMER_INVALID_INDEX upon construction.
   *   - Start time: Date and time of the timer's first occurrence.
   *   - End time:   Start time plus duration; NOT the last occurrence of a
   *                 repeating timer.
   *   - Lifetime:   Timer encompasses all occurrences that begin <= (start time + lifetime).
   *   - Weekday Mask: SMTWRFS, Sunday is LSB; set to 0 for non-repeating event.
   *   - Channel:    Channel that this timer is assigned to.
   *   - Active:     Timer may be disabled to allow the addition of conflicting
   *                 timers.
   *   - Filename:   The filename of the recording is derived from this.
   */
  unsigned int     Index(void) const             { return m_index; }
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
   * A timer is valid if it has a channel, a valid start time, and the end time
   * occurs after the start time.
   */
  bool IsValid(void) const;

  /*!
   * Assign timer a unique index. Only succeeds if index was previously invalid;
   * changing a valid index has no effect and SetIndex() will return false.
   *
   * SetIndex() does not mark timer as changed.
   */
  bool SetIndex(unsigned int index);

  /*!
   * Set the timer params. Timer is marked as changed only if params actually
   * change.
   */
  void SetTime(const CDateTime& startTime,
               const CDateTime& endTime,
               const CDateTimeSpan& lifetime,
               uint8_t weekdayMask);
  void SetChannel(const ChannelPtr& channel);
  void SetActive(bool bActive);
  void SetRecordingFilename(const std::string& strFile);

  // TODO
  bool IsPending(void) const   { return CDateTime::GetUTCDateTime() <= (m_startTime + m_lifetime); }
  bool IsExpired(void) const   { return m_endTime + m_lifetime < CDateTime::GetUTCDateTime(); }
  bool IsRecording(void) const { return false; }

  /*!
   * Serialise timer. Fails if timer is invalid.
   */
  bool SerialiseTimer(TiXmlNode *node) const;

  /*!
   * Deserialise timer. Fails if timer has already been assigned an index or if
   * the deserialised timer is invalid. Timer is marked as unchanged after
   * deserialisation.
   */
  bool DeserialiseTimer(const TiXmlNode *node);

private:
  void Reset(void);

  static std::string WeekdayMaskToString(uint8_t weekdayMask);
  static uint8_t StringToWeekdayMask(const std::string& strWeekdayMask);

  unsigned int  m_index;
  CDateTime     m_startTime;
  CDateTime     m_endTime;
  CDateTimeSpan m_lifetime;
  uint8_t       m_weekdayMask;
  ChannelPtr    m_channel;
  bool          m_bActive;
  std::string   m_strRecordingFilename;
};

}
