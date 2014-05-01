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
