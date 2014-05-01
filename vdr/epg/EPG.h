/*
 * epg.h: Electronic Program Guide
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version (as used in VDR before 1.3.0) written by
 * Robert Schneider <Robert.Schneider@web.de> and Rolf Hakenes <hakenes@hippomi.de>.
 *
 * $Id: epg.h 2.15 2012/09/24 12:53:53 kls Exp $
 */

#ifndef __EPG_H
#define __EPG_H

#include "EPGTypes.h"
#include "Schedule.h"
#include "channels/ChannelID.h"
#include "channels/ChannelTypes.h"
#include "platform/threads/mutex.h"
#include "utils/DateTime.h"

#include <map>
#include <vector>

namespace VDR
{
class CChannelFilter;
class cSchedules;

class cSchedulesLock
{
public:
  cSchedulesLock(bool WriteLock = false, int TimeoutMs = 0);
  virtual ~cSchedulesLock();

  cSchedules* Get(void) const;

  // This function has been butchered out of VDR's old EIT filter
  // I take shame in not fixing all the horrid things it does
  static EventPtr GetEvent(const tChannelID& channelId, tEventID eventId);

  static bool AddEvent(const tChannelID& channelId, const EventPtr& event);

private:
  bool m_bLocked;
};

class cSchedules
{
  friend class cSchedule;
  friend class cSchedulesLock;

public:
  virtual ~cSchedules(void);

  static CDateTime Modified(void);
  void SetModified(SchedulePtr Schedule);
  void ResetVersions(void);
  bool ClearAll(void);
  void CleanTables(void);

  bool Save(void);
  bool Read(void);
  SchedulePtr AddSchedule(const tChannelID& ChannelID);
  void DelSchedule(SchedulePtr schedule);
  SchedulePtr GetSchedule(const tChannelID& ChannelID);
  SchedulePtr GetSchedule(ChannelPtr Channel, bool AddIfMissing = false);
  ScheduleVector GetUpdatedSchedules(const std::map<int, CDateTime>& lastUpdated, CChannelFilter& filter);

  static SchedulePtr EmptySchedule;
protected:
  static cSchedules& Get(void);

private:
  cSchedules(void);

  PLATFORM::CReadWriteLock  m_rwlock;
  CDateTime                 m_modified;
  ScheduleVector  m_schedules;
  bool                      m_bHasUnsavedData;
};

void ReportEpgBugFixStats(bool Force = false);
}
#endif //__EPG_H
