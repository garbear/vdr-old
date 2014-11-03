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
#include "lib/platform/threads/mutex.h"
#include "utils/DateTime.h"
#include "utils/Observer.h"

namespace VDR
{

class cTimerManager : public Observer, public Observable
{
public:
  static cTimerManager& Get(void);
  ~cTimerManager(void);

  TimerPtr    GetByIndex(unsigned int index) const;
  TimerVector GetTimers(void) const;
  size_t      TimerCount(void) const;

  /*!
   * Add a timer and assign it an index. Fails if newTimer already has a valid
   * index, if newTimer's params are invalid, or if newTimer has a time conflict
   * with an active timer.
   *
   * Returns true if newTimer is added. As a side effect, newTimer will be
   * assigned a valid index.
   */
  bool AddTimer(const TimerPtr& newTimer);

  /*!
   * Update the timer with the given index. The index of updatedTimer is
   * ignored.
   *
   * Returns false if there is no timer with the given index, or an attempt to
   * activate a disabled timer causes a time conflict with another active timer.
   * Returns true otherwise (even if no params are modified).
   */
  bool UpdateTimer(unsigned int index, const cTimer& updatedTimer);

  /*!
   * Remove the timer with the given index. If timer is recording,
   * bInterruptRecording must be set to true or this will fail. Returns true if
   * the timer was removed.
   */
  bool RemoveTimer(unsigned int index, bool bInterruptRecording);

  virtual void Notify(const Observable &obs, const ObservableMessage msg);
  void NotifyObservers(void);

  bool LoadTimers(void);
  bool LoadTimers(const std::string &file);
  bool SaveTimers(const std::string &file = "");

private:
  bool TimerConflicts(const cTimer& timer) const;

  cTimerManager(void);

  TimerMap         m_timers;      // Index -> timer
  unsigned int     m_maxIndex;    // Monotonically increasing timer index
  std::string      m_strFilename; // timers.xml filename

  PLATFORM::CMutex m_mutex;
};

}
