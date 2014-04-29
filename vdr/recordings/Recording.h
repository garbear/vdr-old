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

#include "Types.h"
#include <time.h>
#include "RecordingConfig.h"
#include "Config.h"
#include "timers/Timers.h"
#include "epg/EPG.h"
#include "utils/Tools.h"
#include "platform/threads/threads.h"
#include "filesystem/File.h"
#include "filesystem/ResumeFile.h"
#include "marks/Mark.h"

namespace VDR
{
class cRecordingInfo;
class CVideoFile;

extern int InstanceId;

typedef enum
{
  RECORDING_FILE_LIMITS_UNIX,
  RECORDING_FILE_LIMITS_MSDOS,
} recording_file_limits_t;

class cRecording
{
  friend class cRecordings;
public:
  cRecording(TimerPtr Timer, const cEvent *Event);
  cRecording(const std::string& strFileName);
  virtual ~cRecording();
  bool operator==(const cRecording& other) const;
  time_t Start(void) const { return m_start; }
  CDateTime Start2(void) const { return CDateTime(m_start).GetAsUTCDateTime(); } //XXX
  int Priority(void) const { return m_iPriority; }
  int LifetimeDays(void) const { return m_iLifetimeDays; }
  time_t Deleted(void) const { return m_deleted; }
  std::string Name(void) const { return m_strName; }
  std::string FileName(void);
  uint32_t UID(void);
  const cRecordingInfo *Info(void) const { return m_recordingInfo; }
  std::string PrefixFileName(char Prefix);
  int HierarchyLevels(void) const;
  void ResetResume(void);
  double FramesPerSecond(void) const { return m_dFramesPerSecond; }
  int NumFrames(void);
       ///< Returns the number of frames in this recording.
       ///< If the number of frames is unknown, -1 will be returned.
  int LengthInSeconds(void);
       ///< Returns the length (in seconds) of this recording, or -1 in case of error.
  size_t FileSizeMB(void);
       ///< Returns the total file size of this recording (in MB), or -1 if the file
       ///< size is unknown.
  bool IsNew(void) { return GetResume() <= 0; }
  bool IsEdited(void) const;
  bool IsPesRecording(void) const { return m_bIsPesRecording; }
  bool IsOnVideoDirectoryFileSystem(void);
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

  static int SecondsToFrames(int Seconds, double FramesPerSecond = DEFAULTFRAMESPERSECOND);
        // Returns the number of frames corresponding to the given number of seconds.

  static int ReadFrame(CVideoFile *f, uchar *b, int Length, int Max);

  static std::string ExchangeChars(const std::string& strSubject, bool ToFileSystem);
        // Exchanges the characters in the given string to or from a file system
        // specific representation (depending on ToFileSystem). The given string will
        // be modified and may be reallocated if more space is needed. The return
        // value points to the resulting string, which may be different from s.

  static void SetFileLimits(const recording_file_limits_t limits);

  static int      DirectoryPathMax;
  static int      DirectoryNameMax;
  static bool     DirectoryEncoding;

private:
  cRecording(const cRecording&); // can't copy cRecording
  cRecording &operator=(const cRecording &); // can't assign cRecording
  static char *StripEpisodeName(char *s, bool Strip);
  int GetResume(void);

  static char *ExchangeCharsXXX(char *s, bool ToFileSystem);

  int             m_iResume;
  std::string     m_strFileName;
  std::string     m_strName;
  ssize_t         m_iFileSizeMB;
  int             m_iNumFrames;
  int             m_iChannel;
  int             m_iInstanceId;
  bool            m_bIsPesRecording;
  int             m_iIsOnVideoDirectoryFileSystem; // -1 = unknown, 0 = no, 1 = yes
  double          m_dFramesPerSecond;
  cRecordingInfo* m_recordingInfo;
  time_t          m_start;
  int             m_iPriority;
  int             m_iLifetimeDays;
  time_t          m_deleted;
  int64_t         m_hash;
  PLATFORM::CMutex m_mutex;
};
}

#endif //__RECORDING_H
