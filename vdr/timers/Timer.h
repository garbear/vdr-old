#pragma once

#include <stddef.h>
#include <limits.h>
#include "channels/Channel.h"

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

enum eTimerDays
{
  tdNone      = 0,
  tdMonday    = 1 << 0,
  tdTuesday   = 1 << 1,
  tdWednesday = 1 << 2,
  tdThursday  = 1 << 3,
  tdFriday    = 1 << 4,
  tdSaturday  = 1 << 5,
  tdSunday    = 1 << 6
};

class cEvent;
class cSchedules;
class TiXmlNode;

class cTimer
{
public:
  cTimer(void);
  cTimer(ChannelPtr channel, time_t startTime, int iDurationSecs, time_t iFirstDay, uint32_t iWeekdaysMask, uint32_t iTimerFlags, uint32_t iPriority, uint32_t iLifetimeDays, const char *strRecordingFilename);
  cTimer(const cEvent *Event);
  cTimer(const cTimer &Timer);
  virtual ~cTimer();

  static TimerPtr EmptyTimer;

  cTimer& operator= (const cTimer &Timer);
  bool operator==(const cTimer &Timer);
  virtual int Compare(const cTimer &Timer) const;

  bool Recording(void) const                { return m_bRecording; }
  bool Pending(void) const                  { return m_bPending; }
  bool InVpsMargin(void) const              { return m_bInVpsMargin; }
  uint Flags(void) const                    { return m_iTimerFlags; }
  ChannelPtr Channel(void) const            { return m_channel; }
  time_t Day(void) const                    { return m_iFirstDay; }
  int WeekDays(void) const                  { return m_iWeekdaysMask; }
  int Start(void) const                     { return m_iStartSecsSinceMidnight; }
  int DurationSecs(void) const              { return m_iDurationSecs; }
  int Priority(void) const                  { return m_iPriority; }
  int LifetimeDays(void) const              { return m_iLifetimeDays; }
  std::string RecordingFilename(void) const { return m_strRecordingFilename; }
  time_t FirstDay(void) const               { return m_iWeekdaysMask ? m_iFirstDay : 0; }
  const cEvent *Event(void) const           { return m_epgEvent; }

  std::string ToDescr(void) const;

  bool SerialiseTimer(TiXmlNode *node) const;
  bool DeserialiseTimer(const TiXmlNode *node);

  bool IsRepeatingEvent(void) const;
  bool DayMatches(time_t t) const;
  bool Matches(time_t t = 0, bool Directly = false, int Margin = 0);
  eTimerMatch MatchesEvent(const cEvent *Event, int *Overlap = NULL);
  bool Expired(void) const;
  time_t StartTime(void) const;
  time_t StopTime(void) const;
  bool HasFlags(uint Flags) const;
  size_t Index(void) const { return m_index; }

  void SetRecordingFilename(const std::string& strFile);
  void SetEventFromSchedule(cSchedules *Schedules = NULL);
  void ClearEvent(void);
  void SetEvent(const cEvent *Event);
  void SetRecording(bool Recording); // XXX this isn't called yet?
  void SetPending(bool Pending);
  void SetInVpsMargin(bool InVpsMargin);
  void SetDay(time_t Day);
  void SetWeekDays(int WeekDays);
  void SetStart(int Start);
  void SetDuration(int iDurationSecs);
  void SetPriority(int Priority);
  void SetLifetimeDays(int iLifetimeDays);
  void SetFlags(uint Flags);
  void ClrFlags(uint Flags);
  void InvFlags(uint Flags);
  void SetIndex(size_t index) { m_index = index; }

  void Skip(void);
  void OnOff(void);

  static int CompareTimers(const cTimer *a, const cTimer *b);

private:
  time_t        m_startTime;               ///< the next timer start time
  time_t        m_stopTime;                ///< the next timer stop time
  time_t        m_lastEPGEventCheck;       ///< last time we searched for a matching event
  bool          m_bRecording;
  bool          m_bPending;
  bool          m_bInVpsMargin;
  uint          m_iTimerFlags;             ///< flags for this timer. see eTimerFlags
  time_t        m_iFirstDay;               ///< midnight of the day this timer shall hit, or of the first day it shall hit in case of a repeating timer
  uint32_t      m_iWeekdaysMask;           ///< bitmask, lowest bits: SSFTWTM  (the 'M' is the LSB)
  uint32_t      m_iStartSecsSinceMidnight; ///< start of the recording in seconds after midnight on m_day
  uint32_t      m_iDurationSecs;           ///< duration of the recording in seconds
  uint32_t      m_iPriority;               ///< lower priority is deleted first when we run out of disk space
  uint32_t      m_iLifetimeDays;           ///< recording is deleted after this many days if lower than MAXLIFETIME
  std::string   m_strRecordingFilename;    ///< filename of the recording or empty if not recording or recorded
  ChannelPtr    m_channel;                 ///< the channel to record
  const cEvent* m_epgEvent;                ///< the EPG event to record (XXX shouldn't we get the start/end from this?)
  size_t        m_index; // XXX (re)move me
};
