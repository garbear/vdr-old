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

#include "EPGTypes.h"
#include "Schedule.h"
#include "channels/ChannelID.h"
#include "channels/ChannelTypes.h"
#include "lib/platform/threads/mutex.h"
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
  static EventPtr GetEvent(const cChannelID& channelId, tEventID eventId);

  static bool AddEvent(const cChannelID& channelId, const EventPtr& event);

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
  SchedulePtr AddSchedule(const cChannelID& ChannelID);
  void DelSchedule(SchedulePtr schedule);
  SchedulePtr GetSchedule(const cChannelID& ChannelID);
  SchedulePtr GetSchedule(ChannelPtr Channel, bool AddIfMissing = false);
  ScheduleVector GetUpdatedSchedules(const std::map<int, CDateTime>& lastUpdated, CChannelFilter& filter);

  static const SchedulePtr EmptySchedule;
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
