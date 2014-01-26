#include "Schedule.h"
#include "EPG.h"
#include "Config.h"
#include "channels/Channel.h"
#include "channels/ChannelManager.h"

#define RUNNINGSTATUSTIMEOUT 30 // seconds before the running status is considered unknown

cSchedule::cSchedule(tChannelID ChannelID)
{
  channelID   = ChannelID;
  hasRunning  = false;
  modified    = 0;
  presentSeen = 0;
}

cEvent *cSchedule::AddEvent(cEvent *Event)
{
  events.Add(Event);
  Event->schedule = this;
  HashEvent(Event);
  return Event;
}

void cSchedule::DelEvent(cEvent *Event)
{
  if (Event->schedule == this)
  {
    if (hasRunning && Event->IsRunning())
      ClrRunningStatus();
    UnhashEvent(Event);
    events.Del(Event);
  }
}

void cSchedule::HashEvent(cEvent *Event)
{
  eventsHashID.Add(Event, Event->EventID());
  if (Event->StartTime() > 0) // 'StartTime < 0' is apparently used with NVOD channels
    eventsHashStartTime.Add(Event, Event->StartTime());
}

void cSchedule::UnhashEvent(cEvent *Event)
{
  eventsHashID.Del(Event, Event->EventID());
  if (Event->StartTime() > 0) // 'StartTime < 0' is apparently used with NVOD channels
    eventsHashStartTime.Del(Event, Event->StartTime());
}

const cEvent *cSchedule::GetPresentEvent(void) const
{
  const cEvent *pe = NULL;
  time_t now = time(NULL);
  for (cEvent *p = events.First(); p; p = events.Next(p))
  {
    if (p->StartTime() <= now)
      pe = p;
    else if (p->StartTime() > now + 3600)
      break;
    if (p->SeenWithin(RUNNINGSTATUSTIMEOUT)
        && p->RunningStatus() >= SI::RunningStatusPausing)
      return p;
  }
  return pe;
}

const cEvent *cSchedule::GetFollowingEvent(void) const
{
  const cEvent *p = GetPresentEvent();
  if (p)
    p = events.Next(p);
  else
  {
    time_t now = time(NULL);
    for (p = events.First(); p; p = events.Next(p))
    {
      if (p->StartTime() >= now)
        break;
    }
  }
  return p;
}

const cEvent *cSchedule::GetEvent(tEventID EventID, time_t StartTime) const
{
  // Returns the event info with the given StartTime or, if no actual StartTime
  // is given, the one with the given EventID.
  if (StartTime > 0) // 'StartTime < 0' is apparently used with NVOD channels
    return eventsHashStartTime.Get(StartTime);
  else
    return eventsHashID.Get(EventID);
}

const cEvent *cSchedule::GetEventAround(time_t Time) const
{
  const cEvent *pe = NULL;
  time_t delta = ~0;
  for (cEvent *p = events.First(); p; p = events.Next(p))
  {
    time_t dt = Time - p->StartTime();
    if (dt >= 0 && dt < delta && p->EndTime() >= Time)
    {
      delta = dt;
      pe = p;
    }
  }
  return pe;
}

void cSchedule::SetRunningStatus(cEvent *Event, int RunningStatus, cChannel *Channel)
{
  hasRunning = false;
  for (cEvent *p = events.First(); p; p = events.Next(p))
  {
    if (p == Event)
    {
      if (p->RunningStatus() > SI::RunningStatusNotRunning
          || RunningStatus > SI::RunningStatusNotRunning)
      {
        p->SetRunningStatus(RunningStatus, Channel);
        break;
      }
    }
    else if (RunningStatus >= SI::RunningStatusPausing
        && p->StartTime() < Event->StartTime())
      p->SetRunningStatus(SI::RunningStatusNotRunning);
    if (p->RunningStatus() >= SI::RunningStatusPausing)
      hasRunning = true;
  }
}

void cSchedule::ClrRunningStatus(cChannel *Channel)
{
  if (hasRunning)
  {
    for (cEvent *p = events.First(); p; p = events.Next(p))
    {
      if (p->RunningStatus() >= SI::RunningStatusPausing)
      {
        p->SetRunningStatus(SI::RunningStatusNotRunning, Channel);
        hasRunning = false;
        break;
      }
    }
  }
}

void cSchedule::ResetVersions(void)
{
  for (cEvent *p = events.First(); p; p = events.Next(p))
    p->SetVersion(0xFF);
}

void cSchedule::Sort(void)
{
  events.Sort();
  // Make sure there are no RunningStatusUndefined before the currently running event:
  if (hasRunning)
  {
    for (cEvent *p = events.First(); p; p = events.Next(p))
    {
      if (p->RunningStatus() >= SI::RunningStatusPausing)
        break;
      p->SetRunningStatus(SI::RunningStatusNotRunning);
    }
  }
}

void cSchedule::DropOutdated(time_t SegmentStart, time_t SegmentEnd, uchar TableID, uchar Version)
{
  if (SegmentStart > 0 && SegmentEnd > 0)
  {
    for (cEvent *p = events.First(); p; p = events.Next(p))
    {
      if (p->EndTime() > SegmentStart)
      {
        if (p->StartTime() < SegmentEnd)
        {
          // The event overlaps with the given time segment.
          if (p->TableID() > TableID
              || (p->TableID() == TableID && p->Version() != Version))
          {
            // The segment overwrites all events from tables with higher ids, and
            // within the same table id all events must have the same version.
            // We can't delete the event right here because a timer might have
            // a pointer to it, so let's set its id and start time to 0 to have it
            // "phased out":
            if (hasRunning && p->IsRunning())
              ClrRunningStatus();
            UnhashEvent(p);
            p->eventID = 0;
            p->startTime = 0;
          }
        }
        else
          break;
      }
    }
  }
}

void cSchedule::Cleanup(void)
{
  Cleanup(time(NULL));
}

void cSchedule::Cleanup(time_t Time)
{
  cEvent *Event;
  while ((Event = events.First()) != NULL)
  {
    if (!Event->HasTimer() && Event->EndTime() + Setup.EPGLinger * 60 + 3600 < Time) // adding one hour for safety
      DelEvent(Event);
    else
      break;
  }
}

void cSchedule::Dump(FILE *f, const char *Prefix, eDumpMode DumpMode, time_t AtTime) const
{
  ChannelPtr channel = cChannelManager::Get().GetByChannelID(channelID, true);
  if (channel)
  {
    fprintf(f, "%sC %s %s\n", Prefix,
        channel->GetChannelID().Serialize().c_str(), channel->Name().c_str());
    const cEvent *p;
    switch (DumpMode)
      {
    case dmAll:
      {
        for (p = events.First(); p; p = events.Next(p))
          p->Dump(f, Prefix);
      }
      break;
    case dmPresent:
      {
        if ((p = GetPresentEvent()) != NULL)
          p->Dump(f, Prefix);
      }
      break;
    case dmFollowing:
      {
        if ((p = GetFollowingEvent()) != NULL)
          p->Dump(f, Prefix);
      }
      break;
    case dmAtTime:
      {
        if ((p = GetEventAround(AtTime)) != NULL)
          p->Dump(f, Prefix);
      }
      break;
    default:
      esyslog("ERROR: unknown DumpMode %d (%s %d)", DumpMode, __FUNCTION__,
          __LINE__);
      }
    fprintf(f, "%sc\n", Prefix);
  }
}

bool cSchedule::Read(FILE *f, cSchedules *Schedules)
{
  if (Schedules)
  {
    cReadLine ReadLine;
    char *s;
    while ((s = ReadLine.Read(f)) != NULL)
    {
      if (*s == 'C')
      {
        s = skipspace(s + 1);
        char *p = strchr(s, ' ');
        if (p)
          *p = 0; // strips optional channel name
        if (*s)
        {
          tChannelID channelID = tChannelID::Deserialize(s);
          if (channelID.Valid())
          {
            cSchedule *p = Schedules->AddSchedule(channelID);
            if (p)
            {
              if (!cEvent::Read(f, p))
                return false;
              p->Sort();
              Schedules->SetModified(p);
            }
          }
          else
          {
            esyslog("ERROR: invalid channel ID: %s", s);
            return false;
          }
        }
      }
      else
      {
        esyslog("ERROR: unexpected tag while reading EPG data: %s", s);
        return false;
      }
    }
    return true;
  }
  return false;
}
