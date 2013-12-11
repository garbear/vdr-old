/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include <time.h>
#include <assert.h>

// Based on algorithms from http://stackoverflow.com/questions/16647819/timegm-cross-platform
// and https://lists.torproject.org/pipermail/tor-commits/2003-November/022083.html

#define IS_LEAPYEAR(y) (!(y % 4) && ((y % 100) || !(y % 400)))

static const int days_per_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// Number of days between years y1 and y2
inline long days_apart(long y1, long y2)
{
   --y1;
   --y2;
   const long leapdays = (y2 / 4 - y1 / 4) - (y2 / 100 - y1 / 100) + (y2 / 400 - y1 / 400);
   return 365 * (y2 - y1) + leapdays;
}

time_t timegm(struct tm *tm)
{
   unsigned long year, month, days, hours, minutes;

   // Calculate year
   year = tm->tm_year + 1900;
   assert(year >= 1970);

   // Calculate month
   month = tm->tm_mon;
   if (month < 0)
   {
      const long diff = (-month + 11) / 12;
      year -= diff;
      month += 12 * diff;
   }
   if (month > 11)
   {
      year += month / 12;
      month %= 12;
   }

   // Calculate days
   days = tm->tm_mday - 1;
   for (unsigned int i = 0; i < month; i++)
      days += days_apart[i];
   const int leap = (month >= 2 && IS_LEAPYEAR(year));
   days += disparity(1970, year) + (leap ? 1 : 0);

   // Convert days to seconds and add tm's clock time
   hours = days * 24 + tm->tm_hour;
   minutes = hours * 60 + tm->tm_min;
   const time_t seconds = minutes * 60 + tm->tm_sec;
   return seconds;
}
