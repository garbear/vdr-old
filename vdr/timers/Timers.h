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
#include "utils/Observer.h"
#include "Timer.h"

#include <vector>

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
  TimerPtr GetNextPendingTimer(time_t Now);
  TimerPtr GetMatch(const cEvent *Event, eTimerMatch *Match = NULL);
  TimerPtr GetNextActiveTimer(void);
  TimerPtr GetByIndex(size_t index);
  std::vector<TimerPtr> GetTimers(void) const;
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

  void Process(void);

private:
  cTimers(void);

  void StartNewRecordings(time_t Now);

  int                        m_iState;
  time_t                     m_lastSetEvents;
  time_t                     m_lastDeleteExpired;
  std::map<size_t, TimerPtr> m_timers;
  size_t                     m_maxIndex;
  PLATFORM::CMutex           m_mutex;
  std::string                m_strFilename;

  int LastTimerChannel;//XXX
};

#endif //__TIMERS_H
