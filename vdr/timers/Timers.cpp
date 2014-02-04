/*
 * timers.c: Timer handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: timers.c 2.18 2013/03/29 15:37:16 kls Exp $
 */

#include "Timers.h"
#include "Timer.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "utils/Status.h"
#include "utils/UTF8Utils.h"

using namespace std;


// --- cTimers ---------------------------------------------------------------

cTimers Timers;

cTimers::cTimers(void)
{
  state = 0;
  beingEdited = 0;;
  lastSetEvents = 0;
  lastDeleteExpired = 0;
}

cTimer *cTimers::GetTimer(cTimer *Timer)
{
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      if (ti->Channel() == Timer->Channel() &&
          (ti->WeekDays() && ti->WeekDays() == Timer->WeekDays() || !ti->WeekDays() && ti->Day() == Timer->Day()) &&
          ti->Start() == Timer->Start() &&
          ti->Stop() == Timer->Stop())
         return ti;
      }
  return NULL;
}

cTimer *cTimers::GetMatch(time_t t)
{
  static int LastPending = -1;
  cTimer *t0 = NULL;
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      if (!ti->Recording() && ti->Matches(t)) {
         if (ti->Pending()) {
            if (ti->Index() > LastPending) {
               LastPending = ti->Index();
               return ti;
               }
            else
               continue;
            }
         if (!t0 || ti->Priority() > t0->Priority())
            t0 = ti;
         }
      }
  if (!t0)
     LastPending = -1;
  return t0;
}

cTimer *cTimers::GetMatch(const cEvent *Event, eTimerMatch *Match)
{
  cTimer *t = NULL;
  eTimerMatch m = tmNone;
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      eTimerMatch tm = ti->Matches(Event);
      if (tm > m) {
         t = ti;
         m = tm;
         if (m == tmFull)
            break;
         }
      }
  if (Match)
     *Match = m;
  return t;
}

cTimer *cTimers::GetNextActiveTimer(void)
{
  cTimer *t0 = NULL;
  for (cTimer *ti = First(); ti; ti = Next(ti)) {
      ti->Matches();
      if ((ti->HasFlags(tfActive)) && (!t0 || ti->StopTime() > time(NULL) && ti->Compare(*t0) < 0))
         t0 = ti;
      }
  return t0;
}

void cTimers::SetModified(void)
{
  cStatus::MsgTimerChange(NULL, tcMod);
  state++;
}

void cTimers::Add(cTimer *Timer, cTimer *After)
{
  cConfig<cTimer>::Add(Timer, After);
  cStatus::MsgTimerChange(Timer, tcAdd);
}

void cTimers::Ins(cTimer *Timer, cTimer *Before)
{
  cConfig<cTimer>::Ins(Timer, Before);
  cStatus::MsgTimerChange(Timer, tcAdd);
}

void cTimers::Del(cTimer *Timer, bool DeleteObject)
{
  cStatus::MsgTimerChange(Timer, tcDel);
  cConfig<cTimer>::Del(Timer, DeleteObject);
}

bool cTimers::Modified(int &State)
{
  bool Result = state != State;
  State = state;
  return Result;
}

void cTimers::SetEvents(void)
{
  if (time(NULL) - lastSetEvents < 5)
     return;
  cSchedulesLock SchedulesLock(false, 100);
  cSchedules *Schedules = SchedulesLock.Get();
  if (Schedules)
  {
    if (!lastSetEvents || Schedules->Modified() >= lastSetEvents)
    {
      for (cTimer *ti = First(); ti; ti = Next(ti))
      {
        ti->SetEventFromSchedule(Schedules);
      }
    }
  }
  lastSetEvents = time(NULL);
}

void cTimers::DeleteExpired(void)
{
  if (time(NULL) - lastDeleteExpired < 30)
     return;
  cTimer *ti = First();
  while (ti) {
        cTimer *next = Next(ti);
        if (ti->Expired()) {
           isyslog("deleting timer %s", *ti->ToDescr());
           Del(ti);
           SetModified();
           }
        ti = next;
        }
  lastDeleteExpired = time(NULL);
}

int cTimer::CompareTimers(const cTimer *a, const cTimer *b)
{
  time_t t1 = a->StartTime();
  time_t t2 = b->StartTime();
  int r = t1 - t2;
  if (r == 0)
    r = b->priority - a->priority;
  return r;
}
