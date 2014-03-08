#pragma once

#include <stddef.h>
#include <limits.h>
#include "channels/Channel.h"
#include "TimerTime.h"

class cRecording;

enum eTimerFlags
{
  tfNone      = 0,
  tfActive    = 1 << 0,
  tfInstant   = 1 << 1,
  tfVps       = 1 << 2,
  tfRecording = 1 << 3,
  tfAll       = 0xFFFF,
};

enum eTimerMatch
{
  tmNone,
  tmPartial,
  tmFull
};

class cEvent;
class cSchedules;
class TiXmlNode;
class cRecorder;

class cTimer
{
public:
  cTimer(void);
  cTimer(ChannelPtr channel, const CTimerTime& time, uint32_t iTimerFlags, uint32_t iPriority, uint32_t iLifetimeDays, const char *strRecordingFilename);
  cTimer(const cEvent *Event);
  cTimer(const cTimer &Timer);
  virtual ~cTimer();

  static TimerPtr EmptyTimer;

  cTimer& operator= (const cTimer &Timer);
  bool operator==(const cTimer &Timer);
  virtual int Compare(const cTimer &Timer) const;

  bool Recording(void) const                { return m_recorder != NULL; }
  bool Pending(void) const                  { return m_bPending; }
  CDateTime LastRecordingAttempt(void) const{ return m_lastRecordingAttempt; }
  bool RecordingAttemptAllowed(void) const;
  bool InVpsMargin(void) const              { return m_bInVpsMargin; }
  uint Flags(void) const                    { return m_iTimerFlags; }
  ChannelPtr Channel(void) const            { return m_channel; }
  time_t Day(void) const                    { return m_time.DayAsTime(); }
  int WeekDays(void) const                  { return m_time.WeekDayMask(); }
  CDateTime Start(void) const               { return m_time.FirstStart(); }
  int DurationSecs(void) const              { return m_time.DurationSecs(); }
  int Priority(void) const                  { return m_iPriority; }
  int LifetimeDays(void) const              { return m_iLifetimeDays; }
  std::string RecordingFilename(void) const { return m_strRecordingFilename; }
  const cEvent *Event(void) const           { return m_time.EPGEvent(); }

  std::string ToDescr(void) const;

  bool SerialiseTimer(TiXmlNode *node) const;
  bool DeserialiseTimer(const TiXmlNode *node);

  bool IsRepeatingEvent(void) const;
  bool Matches(CDateTime checkTime = CDateTime::GetCurrentDateTime(), bool bDirectly = false, int iMarginSeconds = 0);
  eTimerMatch MatchesEvent(const cEvent *Event, int *Overlap = NULL);
  bool Expired(void) const;
  time_t StartTimeAsTime(void) const;
  CDateTime StartTime(void) const { return m_time.Start(); }
  time_t EndTimeAsTime(void) const;
  CDateTime EndTime(void) const { return m_time.End(); }
  bool HasFlags(uint Flags) const;
  size_t Index(void) const { return m_index; }

  void SetRecordingFilename(const std::string& strFile);
  void SetEventFromSchedule(cSchedules *Schedules = NULL);
  void ClearEvent(void);
  void SetEvent(const cEvent *Event);
  void SetRecording(cRecorder* recorder, cRecording* recording);
  cRecording* GetRecording(void) const { return m_recording; }
  void SetPending(bool Pending);
  void SetInVpsMargin(bool InVpsMargin);
  void SetDuration(int iDurationSecs);
  void SetPriority(int Priority);
  void SetLifetimeDays(int iLifetimeDays);
  void SetFlags(uint Flags);
  void ClrFlags(uint Flags);
  void InvFlags(uint Flags);
  void SetIndex(size_t index) { m_index = index; }

  void Skip(void);

  bool StartRecording(void);
  bool CheckRecordingStatus(const CDateTime& Now);
  void SwitchTransponder(const CDateTime& Now);

  static int CompareTimers(const cTimer *a, const cTimer *b);

private:
  CTimerTime    m_time;
  CDateTime     m_lastEPGEventCheck;       ///< last time we searched for a matching event
  CDateTime     m_lastRecordingAttempt;    ///< last time we started a recording (so we don't spam when it fails)
  bool          m_bPending;
  bool          m_bInVpsMargin;
  uint          m_iTimerFlags;             ///< flags for this timer. see eTimerFlags
  uint32_t      m_iPriority;               ///< lower priority is deleted first when we run out of disk space
  uint32_t      m_iLifetimeDays;           ///< recording is deleted after this many days if lower than MAXLIFETIME
  std::string   m_strRecordingFilename;    ///< filename of the recording or empty if not recording or recorded
  ChannelPtr    m_channel;                 ///< the channel to record
  cRecorder*    m_recorder;                ///< the recorder that's being used while this timer is being recorded
  cRecording*   m_recording;               ///< the recording that is currently running
  size_t        m_index; // XXX (re)move me
};
