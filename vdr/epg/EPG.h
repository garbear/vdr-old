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

#include "Schedule.h"

class cSchedulesLock
{
public:
  cSchedulesLock(bool WriteLock = false, int TimeoutMs = 0);
  virtual ~cSchedulesLock();

  cSchedules* Get(void) const;

private:
  bool m_bLocked;
};

class cSchedules : public cList<cSchedule> {
  friend class cSchedule;
  friend class cSchedulesLock;

public:
  static void SetEpgDataFileName(const char *FileName);
  static time_t Modified(void);
  void SetModified(cSchedule *Schedule);
  void Cleanup(bool Force = false);
  void ResetVersions(void);
  bool ClearAll(void);
  bool Dump(FILE *f = NULL, const char *Prefix = "", eDumpMode DumpMode = dmAll, time_t AtTime = 0);
  bool Read(FILE *f = NULL);
  cSchedule *AddSchedule(tChannelID ChannelID);
  const cSchedule *GetSchedule(tChannelID ChannelID) const;
  const cSchedule *GetSchedule(const cChannel *Channel, bool AddIfMissing = false) const;

protected:
  static cSchedules& Get(void);

private:
  cSchedules(void);
  PLATFORM::CReadWriteLock rwlock;
  std::string              epgDataFileName;
  time_t                   lastDump;
  time_t                   modified;
};

void ReportEpgBugFixStats(bool Force = false);

#endif //__EPG_H
