/*
 * recording.h: Recording file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: recording.h 2.46 2013/03/04 14:01:23 kls Exp $
 */

#ifndef __RECORDING_H
#define __RECORDING_H

#include <time.h>
#include "RecordingConfig.h"
#include "channels/ChannelManager.h"
#include "Config.h"
#include "Timers.h"
#include "epg/EPG.h"
#include "utils/Tools.h"
#include "platform/threads/threads.h"
#include "filesystem/File.h"
#include "ResumeFile.h"
#include "recordings/Mark.h"

class cRecordingInfo;

extern int DirectoryPathMax;
extern int DirectoryNameMax;
extern bool DirectoryEncoding;
extern int InstanceId;

void RemoveDeletedRecordings(void);
void AssertFreeDiskSpace(int Priority = 0, bool Force = false);
     ///< The special Priority value -1 means that we shall get rid of any
     ///< deleted recordings faster than normal (because we're cutting).
     ///< If Force is true, the check will be done even if the timeout
     ///< hasn't expired yet.

class cRecording : public cListObject {
  friend class cRecordings;
private:
  mutable int resume;
  mutable char *titleBuffer;
  mutable char *sortBufferName;
  mutable char *sortBufferTime;
  mutable char *fileName;
  mutable char *name;
  mutable int fileSizeMB;
  mutable int numFrames;
  int channel;
  int instanceId;
  bool isPesRecording;
  mutable int isOnVideoDirectoryFileSystem; // -1 = unknown, 0 = no, 1 = yes
  double framesPerSecond;
  cRecordingInfo *info;
  cRecording(const cRecording&); // can't copy cRecording
  cRecording &operator=(const cRecording &); // can't assign cRecording
  static char *StripEpisodeName(char *s, bool Strip);
  char *SortName(void) const;
  void ClearSortName(void);
  int GetResume(void) const;
  time_t start;
  int priority;
  int lifetime;
  time_t deleted;
  int64_t m_hash;
public:
  cRecording(cTimer *Timer, const cEvent *Event);
  cRecording(const char *FileName);
  virtual ~cRecording();
  time_t Start(void) const { return start; }
  int Priority(void) const { return priority; }
  int Lifetime(void) const { return lifetime; }
  time_t Deleted(void) const { return deleted; }
  virtual int Compare(const cListObject &ListObject) const;
  const char *Name(void) const { return name; }
  const char *FileName(void) const;
  uint32_t UID(void);
  const char *Title(char Delimiter = ' ', bool NewIndicator = false, int Level = -1) const;
  const cRecordingInfo *Info(void) const { return info; }
  const char *PrefixFileName(char Prefix);
  int HierarchyLevels(void) const;
  void ResetResume(void) const;
  double FramesPerSecond(void) const { return framesPerSecond; }
  int NumFrames(void) const;
       ///< Returns the number of frames in this recording.
       ///< If the number of frames is unknown, -1 will be returned.
  int LengthInSeconds(void) const;
       ///< Returns the length (in seconds) of this recording, or -1 in case of error.
  int FileSizeMB(void) const;
       ///< Returns the total file size of this recording (in MB), or -1 if the file
       ///< size is unknown.
  bool IsNew(void) const { return GetResume() <= 0; }
  bool IsEdited(void) const;
  bool IsPesRecording(void) const { return isPesRecording; }
  bool IsOnVideoDirectoryFileSystem(void) const;
  void ReadInfo(void);
  bool WriteInfo(void);
  void SetStartTime(time_t Start);
       ///< Sets the start time of this recording to the given value.
       ///< If a filename has already been set for this recording, it will be
       ///< deleted and a new one will be generated (using the new start time)
       ///< at the next call to FileName().
       ///< Use this function with care - it does not check whether a recording with
       ///< this new name already exists, and if there is one, results may be
       ///< unexpected!
  bool Delete(void);
       ///< Changes the file name so that it will no longer be visible in the "Recordings" menu
       ///< Returns false in case of error
  bool Remove(void);
       ///< Actually removes the file from the disk
       ///< Returns false in case of error
  bool Undelete(void);
       ///< Changes the file name so that it will be visible in the "Recordings" menu again and
       ///< not processed by cRemoveDeletedRecordingsThread.
       ///< Returns false in case of error
  };

int SecondsToFrames(int Seconds, double FramesPerSecond = DEFAULTFRAMESPERSECOND);
      // Returns the number of frames corresponding to the given number of seconds.

int ReadFrame(cUnbufferedFile *f, uchar *b, int Length, int Max);

char *ExchangeChars(char *s, bool ToFileSystem);
      // Exchanges the characters in the given string to or from a file system
      // specific representation (depending on ToFileSystem). The given string will
      // be modified and may be reallocated if more space is needed. The return
      // value points to the resulting string, which may be different from s.

enum eRecordingsSortMode { rsmName, rsmTime };
extern eRecordingsSortMode RecordingsSortMode;
bool HasRecordingsSortMode(const char *Directory);
void GetRecordingsSortMode(const char *Directory);
void SetRecordingsSortMode(const char *Directory, eRecordingsSortMode SortMode);
void IncRecordingsSortMode(const char *Directory);

#endif //__RECORDING_H
