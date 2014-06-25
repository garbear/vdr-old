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

#include "Timer.h"
#include "TimerTypes.h"
#include "Config.h"
#include "utils/Tools.h"
#include "utils/Observer.h"
#include "utils/DateTime.h"

#include <map>
#include <vector>

namespace VDR
{
class cEvent;
class cRecording;

class cTimers : public Observable
{
public:
  static cTimers& Get(void);
  TimerPtr GetTimer(cTimer *Timer);
  /*!
   * @return the next timer that is scheduled to record now with the highest priority
   */
  TimerPtr GetNextPendingTimer(const CDateTime& Now);
  TimerPtr GetMatch(const cEvent *Event, eTimerMatch *Match = NULL);
  TimerPtr GetNextActiveTimer(void);
  TimerPtr GetByIndex(size_t index);
  TimerVector GetTimers(void) const;
  cRecording* GetActiveRecording(const ChannelPtr channel);
  void SetModified(void);
  bool Modified(int &State);
      ///< Returns true if any of the timers have been modified, which
      ///< is detected by State being different than the internal state.
      ///< Upon return the internal state will be stored in State.
  void SetEvents(void);
  void DeleteExpired(void);
  void Add(TimerPtr Timer);
  void Del(TimerPtr Timer);
  void ClearEvents(void);
  size_t Size(void);

  bool HasTimer(const cEvent* event) const;

  bool Load(void);
  bool Load(const std::string &file);
  bool Save(const std::string &file = "");

  void ProcessOnce(void);

  TimerPtr GetTimerForRecording(const cRecording* recording) const;

private:
  cTimers(void);

  void StartNewRecordings(const CDateTime& Now);

  int                        m_iState;
  CDateTime                  m_lastSetEvents;
  time_t                     m_lastDeleteExpired;
  std::map<size_t, TimerPtr> m_timers;
  size_t                     m_maxIndex;
  PLATFORM::CMutex           m_mutex;
  std::string                m_strFilename;
};
}
