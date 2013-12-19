/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "CalendarUtils.h"
#include "StringUtils.h"
#include "UTF8Utils.h"
#include "I18N.h"

using namespace std;

string CalendarUtils::WeekDayName(int WeekDay)
{
  if (WeekDay == 0)
    WeekDay = 6;
  else
    WeekDay -= 1; // we start with Monday==0!

  if (WeekDay <= 6)
  {
    // TRANSLATORS: abbreviated weekdays, beginning with monday (must all be 3 letters!)
    string week = tr("MonTueWedThuFriSatSun");

    // Remove the first N days from week
    unsigned int offset = cUtf8Utils::Utf8SymChars(week.c_str(), WeekDay * 3);
    week = week.substr(offset);

    // Extract the first day
    return week.substr(0, cUtf8Utils::Utf8SymChars(week.c_str(), 3));
  }
  else
    return "???";
}

string CalendarUtils::WeekDayName(time_t t)
{
  struct tm tm_r;
  return WeekDayName(localtime_r(&t, &tm_r)->tm_wday);
}

string CalendarUtils::WeekDayNameFull(int WeekDay)
{
  if (WeekDay == 0)
    WeekDay = 6;
  else
    WeekDay -= 1; // we start with Monday==0!

  switch (WeekDay)
  {
    case 0: return tr("Monday");
    case 1: return tr("Tuesday");
    case 2: return tr("Wednesday");
    case 3: return tr("Thursday");
    case 4: return tr("Friday");
    case 5: return tr("Saturday");
    case 6: return tr("Sunday");
    default: return "???";
  }
}

string CalendarUtils::WeekDayNameFull(time_t t)
{
  struct tm tm_r;
  return WeekDayNameFull(localtime_r(&t, &tm_r)->tm_wday);
}

string CalendarUtils::DayDateTime(time_t t)
{
  if (t == 0)
    time(&t);

  struct tm tm_r;
  tm *tm = localtime_r(&t, &tm_r);

  return StringUtils::Format("%s %02d.%02d. %02d:%02d", WeekDayName(tm->tm_wday).c_str(), tm->tm_mday, tm->tm_mon + 1, tm->tm_hour, tm->tm_min);
}

string CalendarUtils::TimeToString(time_t t)
{
  char buffer[32];
  if (ctime_r(&t, buffer))
  {
    buffer[strlen(buffer) - 1] = 0; // strip trailing newline
    return buffer;
  }
  return "???";
}

string CalendarUtils::DateString(time_t t)
{
  struct tm tm_r;
  tm *tm = localtime_r(&t, &tm_r);

  char buf[32];
  strftime(buf, sizeof(buf), "%d.%m.%Y", tm);

  return WeekDayName(tm->tm_wday) + " " + ShortDateString(t);
}

string CalendarUtils::ShortDateString(time_t t)
{
  struct tm tm_r;
  tm *tm = localtime_r(&t, &tm_r);

  char buf[32];
  strftime(buf, sizeof(buf), "%d.%m.%y", tm);

  return buf;
}

string CalendarUtils::TimeString(time_t t)
{
  char buf[25];
  struct tm tm_r;
  strftime(buf, sizeof(buf), "%R", localtime_r(&t, &tm_r));
  return buf;
}
