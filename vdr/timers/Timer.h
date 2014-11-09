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
#include "devices/TunerHandle.h"
#include "lib/platform/threads/mutex.h"
#include "recordings/RecordingTypes.h"
#include "utils/DateTime.h"
#include "utils/Observer.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <time.h>

class TiXmlNode;

namespace VDR
{

/*!
 * Timer sorting will race if comparisons are not atomic in time. A functor
 * allows us to capture state and use the reference time passed to the functor's
 * constructor for all comparisons in the sorting instance.
 *
 * cTimerSorter produces a weak ordering of timers sorted by the output of
 * cTimer::GetOccurrence(). This allows for efficient greedy processing by
 * sorting all pending timers after (possibly intermingled) expired and
 * occurring timers:
 *
 *    Expired/Occurring <= now < Pending
 */
struct cTimerSorter
{
public:
  cTimerSorter(const CDateTime& now) : m_now(now) { }

  bool operator()(const TimerPtr& lhs, const TimerPtr& rhs) const;

private:
  const CDateTime m_now; // Reference time
};

class cTimer : public Observable
{
public:
  /*!
   * Construct a timer with default or specified properties. Timer ID is
   * initialised to TIMER_INVALID_INDEX and must be set via SetID(). Once set,
   * timer ID is immutable.
   *
   * Parameters:
   *   - startTime:   The first occurrence of the timer
   *   - endTime:     Start time plus duration; NOT the last occurrence of a
   *                  repeating timer
   *   - deadline:    Specifies the time after which no timer occurrences may occur.
   *                  The timer's expiration date is then the last occurrence (subject
   *                  to the weekday mask) that begins on or before the deadline.
   *   - weekdayMask: SMTWRFS, Sunday is LSB; set to 0 for non-repeating event
   *   - channel:     Channel that this timer is assigned to
   *   - strFilename: The filename that recording filenames are derived from
   *   - bActive:     Timer may be disabled to allow the addition of conflicting
   *                  timers (defaults to TRUE)
   */
  cTimer(void);
  cTimer(const CDateTime&   startTime,   /* required */
         const CDateTime&   endTime,     /* required */
         uint8_t            weekdayMask, /* required */
         const CDateTime&   deadline,    /* required */
         const ChannelPtr&  channel,     /* required */
         const std::string& strFilename  = "",
         bool               bActive      = true);

  virtual ~cTimer(void) { }

  static TimerPtr EmptyTimer;

  /*!
   * Time-independent properties.
   */
  unsigned int       ID(void) const               { return m_id; }
  const std::string& Filename(void) const         { return m_strFilename; }
  const CDateTime&   StartTime(void) const        { return m_startTime; }
  const CDateTime&   EndTime(void) const          { return m_endTime; }
  CDateTimeSpan      Duration(void) const         { return m_endTime - m_startTime; }
  uint8_t            WeekdayMask(void) const      { return m_weekdayMask; }
  const CDateTime&   Expires(void) const          { return m_expires; }
  CDateTimeSpan      Lifetime(void) const         { return m_expires - m_startTime; }
  bool               IsRepeatingEvent(void) const { return (m_weekdayMask != 0) ; }
  bool               OccursOnWeekday(unsigned int weekday) const { return m_weekdayMask & (1 << weekday); }
  ChannelPtr         Channel(void) const          { return m_channel; }
  bool               IsActive(void) const         { return m_bActive; }

  /*!
   * Time-dependent states. These depend on the current time of day, which is
   * provided as a reference to allow comparisons that are atomic across time.
   *
   *   - Expired:   timer is not occurring and has no more occurrences
   *   - Occurring: timer has an occurrence that coincides with now
   *   - Pending:   timer is not occurring but has a future occurrence
   *
   * These states are mutually exclusive for a given time.
   */
  bool IsExpired(const CDateTime& now) const   { return m_expires + Duration() <= now; }
  bool IsOccurring(const CDateTime& now) const;
  bool IsPending(const CDateTime& now) const   { return !IsExpired(now) && !IsOccurring(now); }

  /*!
   * GetOccurrence() returns the start time of a timer's occurrence depending on
   * the timer's state:
   *   (1) Expired:   Returns the start time of the last occurrence (value of Expires())
   *   (2) Occurring: Returns the start time of the occurrence
   *   (3) Pending:   Returns the start time of the next occurrence
   */
  CDateTime GetOccurrence(const CDateTime& now) const;

  /*!
   * Check for time conflicts with another timer.
   */
  bool Conflicts(const cTimer& timer) const;

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

  void SetFilename(const std::string& strFilename);
  void SetTime(const CDateTime& startTime,
               const CDateTime& endTime,
               uint8_t          weekdayMask,
               const CDateTime& deadline);
  void SetChannel(const ChannelPtr& channel);
  void SetActive(bool bActive);

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

  /*!
   * Start/stop timer's recording.
   */
  void StartRecording(void);
  void StopRecording(void);

  /*!
   * Returns true if the timer is currently recording.
   */
  bool IsRecording(const CDateTime& now) const;

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
  /*!
   * The timer's expiration date is the last occurrence (subject to the start
   * time and weekday mask) that begins on or before the deadline.
   */
  static CDateTime GetExpiration(const CDateTime& startTime,
                                 uint8_t          weekdayMask,
                                 const CDateTime& deadline);

  // Stringify methods
  static std::string WeekdayMaskToString(uint8_t weekdayMask);
  static uint8_t StringToWeekdayMask(const std::string& strWeekdayMask);

  unsigned int     m_id;
  std::string      m_strFilename;
  CDateTime        m_startTime;
  CDateTime        m_endTime;
  uint8_t          m_weekdayMask;
  CDateTime        m_expires;
  ChannelPtr       m_channel;
  bool             m_bActive;
  RecordingPtr     m_recording;
  PLATFORM::CMutex m_mutex;
};

}
