#include "TimerTime.h"
#include "Timer.h"
#include "Config.h"
#include "epg/Event.h"
#include "epg/Schedule.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "settings/Settings.h"

#define EITPRESENTFOLLOWINGRATE 10 // max. seconds between two occurrences of the "EIT present/following table for the actual multiplex" (2s by the standard, using some more for safety)

CTimerTime::CTimerTime(void) :
  m_firstStartTime(CDateTime::GetCurrentDateTime()),
  m_iWeekdaysMask(tdNone),
  m_iDurationSecs(cSettings::Get().m_iInstantRecordTime ? cSettings::Get().m_iInstantRecordTime : DEFINSTRECTIME),
  m_event(NULL),
  m_bUseVPS(false)
{
}

CTimerTime::CTimerTime(const CDateTime& firstStartTime, uint32_t iWeekdaysMask, uint32_t iDurationSecs) :
  m_firstStartTime(firstStartTime),
  m_iWeekdaysMask(iWeekdaysMask),
  m_iDurationSecs(iDurationSecs),
  m_event(NULL),
  m_bUseVPS(false)
{
}

CTimerTime::CTimerTime(const cEvent* event) :
  m_firstStartTime(CDateTime::GetCurrentDateTime()),
  m_iWeekdaysMask(tdNone),
  m_iDurationSecs(cSettings::Get().m_iInstantRecordTime ? cSettings::Get().m_iInstantRecordTime : DEFINSTRECTIME),
  m_event(event),
  m_bUseVPS(event->HasVps() && cSettings::Get().m_bUseVps)
{
  m_firstStartTime = m_bUseVPS ? event->Vps() : event->StartTime() - CDateTimeSpan(0, 0, cSettings::Get().m_iMarginStart, 0);
  m_iDurationSecs = event->Duration();

  if (!m_bUseVPS)
    m_iDurationSecs += (cSettings::Get().m_iMarginStart * 60) + (cSettings::Get().m_iMarginEnd * 60);
}

CTimerTime& CTimerTime::operator=(const CTimerTime &time)
{
  m_nextStartTime  = time.m_nextStartTime;
  m_nextEndTime    = time.m_nextEndTime;
  m_firstStartTime = time.m_firstStartTime;
  m_iWeekdaysMask  = time.m_iWeekdaysMask;
  m_iDurationSecs  = time.m_iDurationSecs;
  m_event          = time.m_event;
  m_bUseVPS        = time.m_bUseVPS;

  return *this;
}

CTimerTime::~CTimerTime(void)
{
}

time_t CTimerTime::FirstStartAsTime(void) const
{
  time_t retval;
  FirstStart().GetAsTime(retval);
  return retval;
}

CDateTime CTimerTime::Day(void) const
{
  return CTimeUtils::GetDay(m_firstStartTime);
}

time_t CTimerTime::DayAsTime(void) const
{
  time_t retval;
  Day().GetAsTime(retval);
  return retval;
}

CTimerTime CTimerTime::FromVNSI(time_t startTime, time_t stopTime, time_t day, uint32_t weekdays)
{
  CTimerTime retval;

  retval.m_firstStartTime = startTime <= 0 ?
      CDateTime::GetCurrentDateTime() :
      CDateTime(startTime);
  retval.m_iDurationSecs = stopTime - startTime;
  retval.m_iWeekdaysMask = weekdays;

  return retval;
}

bool CTimerTime::DayMatches(const CDateTime& day) const
{
  return !IsRepeatingEvent() ?
      CTimeUtils::GetDay(day) == m_firstStartTime :
      (m_iWeekdaysMask & (1 << CTimeUtils::GetWDay(day))) != 0;
}

bool CTimerTime::Matches(CDateTime checkTime, bool bDirectly, int iMarginSeconds)
{
  uint32_t iDurationSecs = m_iDurationSecs;
  if (iDurationSecs == 0)
    iDurationSecs = SECSINDAY;

  if (m_bUseVPS && m_event && m_event->HasVps())
  {
    if (iMarginSeconds || !bDirectly)
    {
      m_nextStartTime = m_event->StartTime();
      m_nextEndTime   = m_event->EndTime();

      if (!iMarginSeconds)
      { // this is an actual check
        if (m_event->Schedule()->PresentSeenWithin(EITPRESENTFOLLOWINGRATE))
        { // VPS control can only work with up-to-date events...
          if (m_event->StartTime().IsValid()) // checks for "phased out" events
            return m_event->IsRunning(true);
        }

        return m_nextStartTime <= checkTime && checkTime < m_nextEndTime; // ...otherwise we fall back to normal timer handling
      }
    }
  }

  if (!IsRepeatingEvent())
  {
    m_nextStartTime = m_firstStartTime;
    m_nextEndTime   = m_nextStartTime + CDateTimeSpan(0, 0, 0, iDurationSecs);
  }
  else
  {
    CDateTime firstStart = m_firstStartTime;
    if (checkTime < firstStart)
      return false;

    while (checkTime - firstStart > CDateTimeSpan(7, 0, 0, 0))
      firstStart += CDateTimeSpan(7, 0, 0, 0);

    m_nextStartTime.Reset();
    m_nextEndTime.Reset();

    for (unsigned int iDay = 0; iDay <= 7; iDay++)
    {
      if (DayMatches(firstStart))
      {
        m_nextStartTime = firstStart;
        m_nextEndTime   = firstStart + CDateTimeSpan(0, 0, 0, iDurationSecs);
        break;
      }
      firstStart += CDateTimeSpan(1, 0, 0, 0);
    }
  }

  return m_nextStartTime <= checkTime + CDateTimeSpan(0, 0, 0, iMarginSeconds) &&
      checkTime < m_nextEndTime; // must stop *before* stopTime to allow adjacent timers
}

void CTimerTime::Skip(void)
{
  m_firstStartTime += CDateTimeSpan(1, 0, 0, 0);
  m_nextStartTime.Reset();
  m_nextEndTime.Reset();
}

std::string CTimerTime::GetTimeDescription(void) const
{
  CDateTime end = m_firstStartTime + CDateTimeSpan(0, 0, 0, m_iDurationSecs);
  return StringUtils::Format("%02u%02u-%02u%02u", m_firstStartTime.GetHour(), m_firstStartTime.GetMinute(), end.GetHour(), end.GetMinute());
}
