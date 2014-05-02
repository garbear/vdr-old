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

#include "Recording.h"

namespace VDR
{
class cRecordings : public PLATFORM::CThread
{
public:
  cRecordings(bool Deleted = false);
  virtual ~cRecordings();
  bool Load(void) { return Update(true); }
       ///< Loads the current list of recordings and returns true if there
       ///< is anything in it (for compatibility with older plugins - use
       ///< Update(true) instead).
  bool Update(bool Wait = false);
       ///< Triggers an update of the list of recordings, which will run
       ///< as a separate thread if Wait is false. If Wait is true, the
       ///< function returns only after the update has completed.
       ///< Returns true if Wait is true and there is anything in the list
       ///< of recordings, false otherwise.
  void TouchUpdate(void);
       ///< Touches the '.update' file in the video directory, so that other
       ///< instances of VDR that access the same video directory can be triggered
       ///< to update their recordings list.
  bool NeedsUpdate(void);
  void ChangeState(void) { state++; }
  bool StateChanged(int &State);
  void ResetResume(const std::string& ResumeFileName = "");
  cRecording* GetByName(const std::string& strFileName);
  cRecording* AddByName(const std::string& strFileName, bool TriggerUpdate = true);
  cRecording* FindByUID(uint32_t uid);
  void DelByName(const std::string& strFileName);
  void UpdateByName(const std::string& strFileName);
  size_t TotalFileSizeMB(bool bDeletedRecordings = false);
  double MBperMinute(void);
       ///< Returns the average data rate (in MB/min) of all recordings, or -1 if
       ///< this value is unknown.

  void Add(cRecording* recording);
  void Clear(void);
  bool RemoveDeletedRecordings(void);
  bool RemoveDeletedRecording(time_t& LastFreeDiskCheck, int Factor);
  bool RemoveOldestRecording(int Priority);
  void AssertFreeDiskSpace(int Priority = 0, bool Force = false);
       ///< The special Priority value -1 means that we shall get rid of any
       ///< deleted recordings faster than normal (because we're cutting).
       ///< If Force is true, the check will be done even if the timeout
       ///< hasn't expired yet.

  size_t Size(void) const;

  /// XXX use shared_ptr for these
  std::vector<cRecording*> Recordings(void) const;

protected:
  void* Process(void);

private:
  std::string UpdateFileName(void);
  void Refresh(bool Foreground = false);
  void ScanVideoDir(const std::string& strDirName, bool Foreground = false, int LinkLevel = 0);

  static std::string m_strUpdateFileName;
  bool deleted;
  time_t lastUpdate;
  int state;
  std::vector<cRecording*> m_recordings;
  std::vector<cRecording*> m_deletedRecordings;
  time_t                   m_LastFreeDiskCheck;
};

extern cRecordings Recordings;
}
