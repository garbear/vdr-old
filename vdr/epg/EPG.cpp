/*
 * epg.c: Electronic Program Guide
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version (as used in VDR before 1.3.0) written by
 * Robert Schneider <Robert.Schneider@web.de> and Rolf Hakenes <hakenes@hippomi.de>.
 *
 * $Id: epg.c 2.23.1.1 2013/09/01 09:16:53 kls Exp $
 */

#include "EPG.h"
#include "EPGDataWriter.h"
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include "libsi/si.h"
#include "recordings/Timers.h"
#include "platform/threads/threads.h"

#include "vdr/utils/CalendarUtils.h"
#include "vdr/utils/UTF8Utils.h"

#define EPGDATAWRITEDELTA   600 // seconds between writing the epg.data file

// --- cSchedulesLock --------------------------------------------------------

cSchedulesLock::cSchedulesLock(bool WriteLock, int TimeoutMs)
{
  m_bLocked = cSchedules::Get().rwlock.Lock(WriteLock, TimeoutMs);
}

cSchedulesLock::~cSchedulesLock()
{
  if (m_bLocked)
    cSchedules::Get().rwlock.Unlock();
}

cSchedules* cSchedulesLock::Get(void) const
{
  return m_bLocked ? &cSchedules::Get() : NULL;
}

// --- cSchedules ------------------------------------------------------------

cSchedules::cSchedules(void)
{
  lastDump = time(NULL);
  modified = 0;
}

cSchedules& cSchedules::Get(void)
{
  static cSchedules _instance;
  return _instance;
}

void cSchedules::SetEpgDataFileName(const char *FileName)
{
  cSchedulesLock lock(true);
  cSchedules* schedules = lock.Get();
  if (schedules)
  {
    schedules->epgDataFileName = FileName ? FileName : "";
    cEpgDataWriter::Get().SetDump(!schedules->epgDataFileName.empty());
  }
}

time_t cSchedules::Modified(void)
{
  cSchedulesLock lock(true);
  cSchedules* schedules = lock.Get();
  return schedules ? schedules->modified : 0;
}

void cSchedules::SetModified(cSchedule *Schedule)
{
  Schedule->SetModified();
  modified = time(NULL);
}

void cSchedules::Cleanup(bool Force)
{
  if (Force)
    lastDump = 0;
  time_t now = time(NULL);
  if (now - lastDump > EPGDATAWRITEDELTA)
  {
    if (Force)
      cEpgDataWriter::Get().Perform();
    else if (!cEpgDataWriter::Get().IsRunning())
      cEpgDataWriter::Get().CreateThread();
    lastDump = now;
  }
}

void cSchedules::ResetVersions(void)
{
  for (cSchedule *Schedule = First(); Schedule; Schedule = Next(Schedule))
    Schedule->ResetVersions();
}

bool cSchedules::ClearAll(void)
{
  for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer))
    Timer->SetEvent(NULL);
  for (cSchedule *Schedule = First(); Schedule; Schedule = Next(Schedule))
    Schedule->Cleanup(INT_MAX);
  return true;
}

bool cSchedules::Dump(FILE *f, const char *Prefix, eDumpMode DumpMode, time_t AtTime)
{
  cSafeFile *sf = NULL;
  if (!f)
  {
    sf = new cSafeFile(epgDataFileName.c_str());
    if (sf->Open())
      f = *sf;
    else
    {
      LOG_ERROR;
      delete sf;
      return false;
    }
  }
  for (cSchedule *p = First(); p; p = Next(p))
    p->Dump(f, Prefix, DumpMode, AtTime);
  if (sf)
  {
    sf->Close();
    delete sf;
  }
  return true;
}

bool
cSchedules::Read(FILE *f)
{
  bool OwnFile = f == NULL;
  if (OwnFile)
  {
    if (!epgDataFileName.empty() && access(epgDataFileName.c_str(), R_OK) == 0)
    {
      dsyslog("reading EPG data from %s", epgDataFileName.c_str());
      if ((f = fopen(epgDataFileName.c_str(), "r")) == NULL)
      {
        LOG_ERROR;
        return false;
      }
    }
    else
      return false;
  }
  bool result = cSchedule::Read(f, this);
  if (OwnFile)
    fclose(f);
  if (result)
  {
    // Initialize the channels' schedule pointers, so that the first WhatsOn menu will come up faster:
    std::vector<ChannelPtr> channels = cChannelManager::Get().GetCurrent();
    for (std::vector<ChannelPtr>::const_iterator it = channels.begin();
        it != channels.end(); ++it)
    {
      ChannelPtr Channel = (*it);
      GetSchedule(Channel.get());
    }
  }
  return result;
}

cSchedule *cSchedules::AddSchedule(tChannelID ChannelID)
{
  //ChannelID.ClrRid();
  cSchedule *p = (cSchedule *) GetSchedule(ChannelID);
  if (!p)
  {
    p = new cSchedule(ChannelID);
    Add(p);
    ChannelPtr channel = cChannelManager::Get().GetByChannelID(ChannelID);
    if (channel)
      channel->SetSchedule(p);
  }
  return p;
}

const cSchedule *cSchedules::GetSchedule(tChannelID ChannelID) const
{
  //ChannelID.ClrRid();
  for (cSchedule *p = First(); p; p = Next(p))
  {
    if (p->ChannelID() == ChannelID)
      return p;
  }
  return NULL;
}

const cSchedule *cSchedules::GetSchedule(const cChannel *channel, bool bAddIfMissing) const
{
  if (!channel->HasSchedule() && bAddIfMissing)
  {
    cSchedule *Schedule = new cSchedule(channel->GetChannelID());
    ((cSchedules *) this)->Add(Schedule);
    channel->SetSchedule(Schedule);
  }

  return channel->HasSchedule() ? channel->Schedule() : NULL;
}
