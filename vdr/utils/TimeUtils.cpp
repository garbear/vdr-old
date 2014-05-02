#include "TimeUtils.h"
#include "I18N.h"
#include "UTF8Utils.h"

#include <stdlib.h>

namespace VDR
{

int CTimeUtils::GetMDay(time_t t)
{
  struct tm tm_r;
  return localtime_r(&t, &tm_r)->tm_mday;
}

int CTimeUtils::GetWDay(time_t t)
{
  struct tm tm_r;
  int weekday = localtime_r(&t, &tm_r)->tm_wday;
  return weekday == 0 ? 6 : weekday - 1; // we start with Monday==0!
}

int CTimeUtils::GetWDay(const CDateTime& time)
{
  int weekday = time.GetDayOfWeek();
  return weekday == 0 ? 6 : weekday - 1; // we start with Monday==0!
}

time_t CTimeUtils::IncDay(time_t t, int Days)
{
  struct tm tm_r;
  tm tm = *localtime_r(&t, &tm_r);
  tm.tm_mday += Days; // now tm_mday may be out of its valid range
  int h = tm.tm_hour; // save original hour to compensate for DST change
  tm.tm_isdst = -1;   // makes sure mktime() will determine the correct DST setting
  t = mktime(&tm);    // normalize all values
  tm.tm_hour = h;     // compensate for DST change
  return mktime(&tm); // calculate final result
}

time_t CTimeUtils::SetTime(time_t t, int SecondsFromMidnight)
{
  struct tm tm_r;
  tm tm = *localtime_r(&t, &tm_r);
  tm.tm_hour = SecondsFromMidnight / 3600;
  tm.tm_min = (SecondsFromMidnight % 3600) / 60;
  tm.tm_sec =  SecondsFromMidnight % 60;
  tm.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
  return mktime(&tm);
}

int CTimeUtils::TimeToInt(int t)
{
  return (t / 100 * 60 + t % 100) * 60;
}

int CTimeUtils::IntToTime(int t)
{
  return (t / 3600) * 100 + (t % 3600) / 60;
}

bool CTimeUtils::ParseDay(const char *s, time_t &Day, uint32_t &WeekDays)
{
  // possible formats are:
  // 19
  // 2005-03-19
  // MTWTFSS
  // MTWTFSS@19
  // MTWTFSS@2005-03-19

  Day = 0;
  WeekDays = 0;
  s = skipspace(s);
  if (!*s)
    return false;
  const char *a = strchr(s, '@');
  const char *d = a ? a + 1 : isdigit(*s) ? s : NULL;
  if (d)
  {
    if (strlen(d) == 10)
    {
      struct tm tm_r;
      if (3
          == sscanf(d, "%d-%d-%d", &tm_r.tm_year, &tm_r.tm_mon, &tm_r.tm_mday))
      {
        tm_r.tm_year -= 1900;
        tm_r.tm_mon--;
        tm_r.tm_hour = tm_r.tm_min = tm_r.tm_sec = 0;
        tm_r.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
        Day = mktime(&tm_r);
      }
      else
        return false;
    }
    else
    {
      // handle "day of month" for compatibility with older versions:
      char *tail = NULL;
      int day = strtol(d, &tail, 10);
      if (tail && *tail || day < 1 || day > 31)
        return false;
      time_t t = time(NULL);
      int DaysToCheck = 61; // 61 to handle months with 31/30/31
      for (int i = -1; i <= DaysToCheck; i++)
      {
        time_t t0 = IncDay(t, i);
        if (GetMDay(t0) == day)
        {
          Day = SetTime(t0, 0);
          break;
        }
      }
    }
  }
  if (a || !isdigit(*s))
  {
    if ((a && a - s == 7) || strlen(s) == 7)
    {
      for (const char *p = s + 6; p >= s; p--)
      {
        WeekDays <<= 1;
        WeekDays |= (*p != '-');
      }
    }
    else
      return false;
  }
  return true;
}

std::string CTimeUtils::PrintDay(time_t Day, int WeekDays, bool SingleByteChars)
{
#define DAYBUFFERSIZE 64
  char buffer[DAYBUFFERSIZE];
  char *b = buffer;
  if (WeekDays)
  {
    // TRANSLATORS: the first character of each weekday, beginning with monday
    const char *w = trNOOP("MTWTFSS");
    if (!SingleByteChars)
      w = tr(w);
    while (*w)
    {
      int sl = cUtf8Utils::Utf8CharLen(w);
      if (WeekDays & 1)
      {
        for (int i = 0; i < sl; i++)
          b[i] = w[i];
        b += sl;
      }
      else
        *b++ = '-';
      WeekDays >>= 1;
      w += sl;
    }
    if (Day)
      *b++ = '@';
  }
  if (Day)
  {
    struct tm tm_r;
    localtime_r(&Day, &tm_r);
    b += strftime(b, DAYBUFFERSIZE - (b - buffer), "%Y-%m-%d", &tm_r);
  }
  *b = 0;
  return buffer;
}

CDateTime CTimeUtils::GetDay(const CDateTime& time)
{
  CDateTime retval(time);
  retval -= CDateTimeSpan(0, retval.GetHour(), retval.GetMinute(), retval.GetSecond());
  return retval;
}

}
