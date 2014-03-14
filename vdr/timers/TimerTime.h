#pragma once

#include "Types.h"
#include "utils/DateTime.h"

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

class CTimerTime
{
public:
  CTimerTime(void);
  CTimerTime(const CDateTime& firstStartTime, uint32_t iWeekdaysMask, uint32_t iDurationSecs);
  CTimerTime(const cEvent* event);
  virtual ~CTimerTime(void);

  CTimerTime& operator=(const CTimerTime &time);

  CDateTime Start(void) const { return m_nextStartTime; }
  CDateTime End(void) const { return m_nextEndTime; }

  CDateTime FirstStart(void) const { return m_firstStartTime; }
  time_t    FirstStartAsTime(void) const;
  void      SetFirstStart(const CDateTime& start) { m_firstStartTime = start; }
  CDateTime Day(void) const;
  time_t    DayAsTime(void) const;
  bool      IsRepeatingEvent(void) const { return m_iWeekdaysMask; }
  uint32_t  WeekDayMask(void) const { return m_iWeekdaysMask; }
  void      SetWeekDayMask(uint32_t mask) { m_iWeekdaysMask = mask; }
  uint32_t  DurationSecs(void) const { return m_iDurationSecs; }
  void      SetDurationSecs(uint32_t val) { m_iDurationSecs = val; }

  bool      DayMatches(const CDateTime& day) const;
  bool      Matches(CDateTime time = CDateTime::GetCurrentDateTime(), bool Directly = false, int Margin = 0);
  void      Skip(void);
  std::string GetTimeDescription(void) const;

  void          ClearEPGEvent(void) { m_event = NULL; }
  void          SetEPGEvent(const cEvent* event) { m_event = event; }
  const cEvent* EPGEvent(void) const { return m_event; }

  static CTimerTime FromVNSI(time_t startTime, time_t stopTime, time_t day, uint32_t weekdays);
private:
  CDateTime     m_nextStartTime;   ///< the next timer start time
  CDateTime     m_nextEndTime;     ///< the next timer stop time

  CDateTime     m_firstStartTime;  ///< start time of the first recording
  uint32_t      m_iWeekdaysMask;   ///< bitmask, lowest bits: SSFTWTM  (the 'M' is the LSB)
  uint32_t      m_iDurationSecs;   ///< duration of the recording in seconds
  const cEvent* m_event;
  bool          m_bUseVPS;
};
