/*
 * timers.h: Timer handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: timers.h 2.7 2013/03/11 10:35:53 kls Exp $
 */

#ifndef __TIMERS_H
#define __TIMERS_H

#include "channels/ChannelManager.h"
#include "Config.h"
#include "utils/Tools.h"
#include "Timer.h"

#include <vector>

class cEvent;
class cRecording;

class cTimers
{
public:
  static cTimers& Get(void);
  TimerPtr GetTimer(cTimer *Timer);
  TimerPtr GetMatch(time_t t);
  TimerPtr GetMatch(const cEvent *Event, eTimerMatch *Match = NULL);
  TimerPtr GetNextActiveTimer(void);
  TimerPtr GetByIndex(size_t index);
  cRecording* GetActiveRecording(const ChannelPtr channel);
  void SetModified(void);
  bool Modified(int &State);
      ///< Returns true if any of the timers have been modified, which
      ///< is detected by State being different than the internal state.
      ///< Upon return the internal state will be stored in State.
  void SetEvents(void);
  void DeleteExpired(void);
  void Add(TimerPtr Timer, TimerPtr After = cTimer::EmptyTimer);
  void Ins(TimerPtr Timer, TimerPtr Before = cTimer::EmptyTimer);
  void Del(TimerPtr Timer, bool DeleteObject = true);
  void ClearEvents(void);
  size_t Size(void) { return m_timers.size(); }

  bool HasTimer(const cEvent* event) const;

  bool Load(void);
  bool Load(const std::string &file);
  bool Save(const std::string &file = "");

private:
  cTimers(void);

  int                   m_iState;
  time_t                m_lastSetEvents;
  time_t                m_lastDeleteExpired;
  std::vector<TimerPtr> m_timers;
  size_t                m_maxIndex;
  PLATFORM::CMutex      m_mutex;
  std::string           m_strFilename;
};

#endif //__TIMERS_H
