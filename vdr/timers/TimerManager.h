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
#include "utils/Observer.h"

namespace VDR
{

class cTimerManager : public Observer, public Observable
{
public:
  static cTimerManager& Get(void);
  ~cTimerManager(void);

  TimerPtr    GetByID(unsigned int id) const;
  TimerVector GetTimers(void) const;
  size_t      TimerCount(void) const;

  /*!
   * Add a timer and assign it an ID. Fails if newTimer already has a valid
   * ID, if newTimer's properties are invalid, or if newTimer has a time
   * conflict with an active timer. (TODO: Allow conflicting timers to support
   * multiple devices).
   *
   * Returns true if newTimer is added. As a side effect, newTimer will be
   * assigned a valid ID.
   */
  bool AddTimer(const TimerPtr& newTimer);

  /*!
   * Update the timer with the given ID. The ID of updatedTimer is ignored.
   *
   * Returns false if there is no timer with the given ID, or an attempt to
   * activate a disabled timer causes a time conflict with another active timer.
   * Returns true otherwise (even if no properties are modified).
   */
  bool UpdateTimer(unsigned int id, const cTimer& updatedTimer);

  /*!
   * Remove the timer with the given ID. If timer is recording,
   * bInterruptRecording must be set to true or this will fail. Returns true if
   * the timer was removed.
   */
  bool RemoveTimer(unsigned int id, bool bInterruptRecording);

  virtual void Notify(const Observable& obs, const ObservableMessage msg);
  void NotifyObservers(void);

  bool LoadTimers(void);
  bool LoadTimers(const std::string& file);
  bool SaveTimers(const std::string& file = "");

private:
  bool TimerConflicts(const cTimer& timer) const;

  cTimerManager(void);

  TimerMap         m_timers;      // ID -> timer
  unsigned int     m_maxID;       // Monotonically increasing timer ID
  std::string      m_strFilename; // timers.xml filename

  PLATFORM::CMutex m_mutex;
};

}
