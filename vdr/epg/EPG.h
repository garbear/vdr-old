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

#include "Types.h"
#include "Schedule.h"
#include "utils/DateTime.h"
#include <vector>
#include <map>

class CChannelFilter;

class cSchedulesLock
{
public:
  cSchedulesLock(bool WriteLock = false, int TimeoutMs = 0);
  virtual ~cSchedulesLock();

  cSchedules* Get(void) const;

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
  std::vector<SchedulePtr> GetUpdatedSchedules(const std::map<int, CDateTime>& lastUpdated, CChannelFilter& filter);

  static SchedulePtr EmptySchedule;
protected:
  static cSchedules& Get(void);

private:
  cSchedules(void);

  PLATFORM::CReadWriteLock  m_rwlock;
  CDateTime                 m_modified;
  std::vector<SchedulePtr>  m_schedules;
  bool                      m_bHasUnsavedData;
};

void ReportEpgBugFixStats(bool Force = false);

#endif //__EPG_H
