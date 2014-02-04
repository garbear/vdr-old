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

class cTimers : public cConfig<cTimer> {
private:
  int state;
  int beingEdited;
  time_t lastSetEvents;
  time_t lastDeleteExpired;
public:
  cTimers(void);
  cTimer *GetTimer(cTimer *Timer);
  cTimer *GetMatch(time_t t);
  cTimer *GetMatch(const cEvent *Event, eTimerMatch *Match = NULL);
  cTimer *GetNextActiveTimer(void);
  int BeingEdited(void) { return beingEdited; }
  void IncBeingEdited(void) { beingEdited++; }
  void DecBeingEdited(void) { if (!--beingEdited) lastSetEvents = 0; }
  void SetModified(void);
  bool Modified(int &State);
      ///< Returns true if any of the timers have been modified, which
      ///< is detected by State being different than the internal state.
      ///< Upon return the internal state will be stored in State.
  void SetEvents(void);
  void DeleteExpired(void);
  void Add(cTimer *Timer, cTimer *After = NULL);
  void Ins(cTimer *Timer, cTimer *Before = NULL);
  void Del(cTimer *Timer, bool DeleteObject = true);
  };

extern cTimers Timers;

#endif //__TIMERS_H
