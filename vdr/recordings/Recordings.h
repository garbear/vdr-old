#pragma once

#include "Recording.h"

class cRecordings : public cList<cRecording>, public PLATFORM::CThread {
private:
  static char *updateFileName;
  bool deleted;
  time_t lastUpdate;
  int state;
  const char *UpdateFileName(void);
  void Refresh(bool Foreground = false);
  void ScanVideoDir(const std::string& strDirName, bool Foreground = false, int LinkLevel = 0);
protected:
  void* Process(void);
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
  void ResetResume(const char *ResumeFileName = NULL);
  void ClearSortNames(void);
  cRecording *GetByName(const char *FileName);
  void AddByName(const char *FileName, bool TriggerUpdate = true);
  cRecording* FindByUID(uint32_t uid);
  void DelByName(const char *FileName);
  void UpdateByName(const char *FileName);
  int TotalFileSizeMB(void);
  double MBperMinute(void);
       ///< Returns the average data rate (in MB/min) of all recordings, or -1 if
       ///< this value is unknown.
  };

extern cRecordings Recordings;
extern cRecordings DeletedRecordings;
