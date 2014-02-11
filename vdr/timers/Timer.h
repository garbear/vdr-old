#pragma once

#include <stddef.h>
#include <limits.h>
#include "channels/Channel.h"

enum eTimerFlags { tfNone      = 0x0000,
                   tfActive    = 0x0001,
                   tfInstant   = 0x0002,
                   tfVps       = 0x0004,
                   tfRecording = 0x0008,
                   tfAll       = 0xFFFF,
                 };
enum eTimerMatch { tmNone, tmPartial, tmFull };

class cEvent;
class cSchedules;
class TiXmlNode;

class cTimer
{
public:
  cTimer(bool Instant = false, bool Pause = false, ChannelPtr Channel = cChannel::EmptyChannel);
  cTimer(const cEvent *Event);
  cTimer(const cTimer &Timer);
  virtual ~cTimer();

  static TimerPtr EmptyTimer;

  cTimer& operator= (const cTimer &Timer);
  bool operator==(const cTimer &Timer);
  virtual int Compare(const cTimer &Timer) const;

  bool Recording(void) const      { return recording; }
  bool Pending(void) const        { return pending; }
  bool InVpsMargin(void) const    { return inVpsMargin; }
  uint Flags(void) const          { return m_flags; }
  ChannelPtr Channel(void) const  { return m_channel; }
  time_t Day(void) const          { return m_day; }
  int WeekDays(void) const        { return m_weekdays; }
  int Start(void) const           { return m_start; }
  int Stop(void) const            { return m_stop; }
  int Priority(void) const        { return m_priority; }
  int Lifetime(void) const        { return m_lifetime; }
  std::string File(void) const    { return m_file; }
  time_t FirstDay(void) const     { return m_weekdays ? m_day : 0; }
  const char *Aux(void) const     { return m_aux; }
  time_t Deferred(void) const     { return deferred; }
  const cEvent *Event(void) const { return event; }
  std::string Serialise(bool UseChannelID = false) const;
  bool SerialiseTimer(TiXmlNode *node) const;
  bool DeserialiseTimer(const TiXmlNode *node);
  std::string ToDescr(void) const;
  bool Parse(const char *s);
  bool IsSingleEvent(void) const;
  static int GetMDay(time_t t);
  static int GetWDay(time_t t);
  bool DayMatches(time_t t) const;
  static time_t IncDay(time_t t, int Days);
  static time_t SetTime(time_t t, int SecondsFromMidnight);
  void SetFile(const std::string& strFile);
  bool Matches(time_t t = 0, bool Directly = false, int Margin = 0);
  eTimerMatch MatchesEvent(const cEvent *Event, int *Overlap = NULL);
  bool Expired(void) const;
  time_t StartTime(void) const;
  time_t StopTime(void) const;
  void SetEventFromSchedule(cSchedules *Schedules = NULL);
  void SetEvent(const cEvent *Event);
  void SetRecording(bool Recording);
  void SetPending(bool Pending);
  void SetInVpsMargin(bool InVpsMargin);
  void SetDay(time_t Day);
  void SetWeekDays(int WeekDays);
  void SetStart(int Start);
  void SetStop(int Stop);
  void SetPriority(int Priority);
  void SetLifetime(int Lifetime);
  void SetAux(const char *Aux);
  void SetDeferred(int Seconds);
  void SetFlags(uint Flags);
  void ClrFlags(uint Flags);
  void InvFlags(uint Flags);
  bool HasFlags(uint Flags) const;
  void Skip(void);
  void OnOff(void);
  size_t Index(void) const { return m_index; }
  void SetIndex(size_t index) { m_index = index; }
  cString PrintFirstDay(void) const;
  static int TimeToInt(int t);
  static bool ParseDay(const char *s, time_t &Day, int &WeekDays);
  static cString PrintDay(time_t Day, int WeekDays, bool SingleByteChars);

  static int CompareTimers(const cTimer *a, const cTimer *b);

private:
  time_t startTime, stopTime;
  time_t lastSetEvent;
  time_t deferred; ///< Matches(time_t, ...) will return false if the current time is before this value
  bool recording, pending, inVpsMargin;
  uint m_flags;
  ChannelPtr m_channel;
  time_t m_day; ///< midnight of the day this timer shall hit, or of the first day it shall hit in case of a repeating timer
  int m_weekdays;       ///< bitmask, lowest bits: SSFTWTM  (the 'M' is the LSB)
  int m_start;
  int m_stop;
  int m_priority;
  int m_lifetime;
  std::string m_file;
  char *m_aux;
  const cEvent *event;
  size_t m_index;
  };
