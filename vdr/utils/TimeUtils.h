/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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
#pragma once

#include "DateTime.h"

#include <stdint.h>
#include <string>
#include <time.h>

namespace VDR
{
class CTimeUtils
{
public:
  static int GetMDay(time_t t);
  static int GetWDay(const CDateTime& time);
  static int GetWDay(time_t t);
  static time_t IncDay(time_t t, int Days);
  static time_t SetTime(time_t t, int SecondsFromMidnight);
  static CDateTime GetDay(const CDateTime& time);
  static int TimeToInt(int t);
  static int IntToTime(int t);
  static bool ParseDay(const char *s, time_t &Day, uint32_t &WeekDays);
  static std::string PrintDay(time_t Day, int WeekDays, bool SingleByteChars);
};
}
