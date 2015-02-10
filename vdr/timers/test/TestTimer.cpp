/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "timers/Timer.h"
#include "utils/DateTime.h"

#include "gtest/gtest.h"

namespace VDR
{

static const CDateTimeSpan TEST_ONE_SECOND = CDateTimeSpan(0, 0, 0, 1);
static const CDateTimeSpan TEST_ONE_HOUR   = CDateTimeSpan(0, 1, 0, 0);
static const CDateTimeSpan TEST_ONE_DAY    = CDateTimeSpan(1, 0, 0, 0);
static const CDateTimeSpan TEST_ONE_WEEK   = CDateTimeSpan(7, 0, 0, 0);

TEST(TestTimer, SetID)
{
  cTimer timer;
  EXPECT_EQ(timer.ID(), TIMER_INVALID_ID);

  timer.SetID(42);
  EXPECT_EQ(timer.ID(), 42);
}

TEST(TestTimer, SetFilename)
{
  cTimer timer;
  EXPECT_STREQ(timer.Filename().c_str(), "");

  timer.SetFilename("test");
  EXPECT_STREQ(timer.Filename().c_str(), "test");
}

TEST(TestTimer, SetTime)
{
  const CDateTime now = CDateTime::GetUTCDateTime();

  cTimer timer;
  timer.SetTime(now, now + TEST_ONE_HOUR, TIMER_MASK_SATURDAY | TIMER_MASK_SUNDAY, now + TEST_ONE_WEEK);
  EXPECT_TRUE(timer.StartTime() == now);
  EXPECT_EQ((timer.EndTime() - now).GetHours(), 1);
  EXPECT_EQ(timer.WeekdayMask(), TIMER_MASK_SATURDAY | TIMER_MASK_SUNDAY);
  EXPECT_EQ(timer.Duration().GetHours(), 1);
  EXPECT_TRUE(timer.IsRepeatingEvent());
  EXPECT_TRUE(timer.OccursOnWeekday(0)); // Sunday
  EXPECT_FALSE(timer.OccursOnWeekday(1)); // Monday
  EXPECT_TRUE(timer.OccursOnWeekday(6)); // Saturday

  timer.SetTime(now, now + TEST_ONE_HOUR, 0, now);
  EXPECT_EQ(timer.Expires(), now);
  EXPECT_EQ(timer.Lifetime().GetDays(), 0);
  EXPECT_FALSE(timer.IsRepeatingEvent());

  timer.SetTime(now, now + TEST_ONE_HOUR, 0, now + TEST_ONE_WEEK);
  EXPECT_TRUE(timer.Expires() == now); // non-repeating
  EXPECT_EQ(timer.Lifetime().GetDays(), 0);
  EXPECT_FALSE(timer.IsRepeatingEvent());

  timer.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_DAY); // repeats next day
  EXPECT_EQ((timer.Expires() - now).GetDays(), 1);
  EXPECT_EQ(timer.Lifetime().GetDays(), 1);
  EXPECT_TRUE(timer.IsRepeatingEvent());

  timer.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_WEEK); // repeats for a week
  EXPECT_EQ((timer.Expires() - now).GetDays(), 7);
  EXPECT_EQ(timer.Lifetime().GetDays(), 7);
  EXPECT_TRUE(timer.IsRepeatingEvent());
}

TEST(TestTimer, SetActive)
{
  cTimer timer;
  EXPECT_TRUE(timer.IsActive());

  timer.SetActive(false);
  EXPECT_FALSE(timer.IsActive());
}

TEST(TestTimer, IsValid)
{
  const CDateTime now = CDateTime::GetUTCDateTime();

  cTimer timer;
  EXPECT_FALSE(timer.IsValid(false));

  // Invalid start time
  timer.SetTime(CDateTime(), now, 0, now);
  EXPECT_FALSE(timer.IsValid(false));

  // Invalid end time
  timer.SetTime(now, CDateTime(), 0, now);
  EXPECT_FALSE(timer.IsValid(false));

  // start time >= end time
  timer.SetTime(now, now, 0, now);
  EXPECT_FALSE(timer.IsValid(false));
  timer.SetTime(now + TEST_ONE_HOUR, now, 0, now);
  EXPECT_FALSE(timer.IsValid(false));
}

TEST(TestTimer, IsExpired)
{
  const CDateTime now = CDateTime::GetUTCDateTime();

  cTimer timer;
  timer.SetTime(now, now + TEST_ONE_HOUR, 0, now); // non-repeating
  EXPECT_FALSE(timer.IsExpired(now - TEST_ONE_DAY));
  EXPECT_FALSE(timer.IsExpired(now));
  EXPECT_FALSE(timer.IsExpired(now + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsExpired(now + TEST_ONE_HOUR));
  EXPECT_TRUE(timer.IsExpired(now + TEST_ONE_DAY));

  timer.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_DAY); // repeats for a day
  EXPECT_FALSE(timer.IsExpired(now - TEST_ONE_DAY));
  EXPECT_FALSE(timer.IsExpired(now));
  EXPECT_FALSE(timer.IsExpired(now + TEST_ONE_DAY));
  EXPECT_FALSE(timer.IsExpired(now + TEST_ONE_DAY + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsExpired(now + TEST_ONE_DAY + TEST_ONE_HOUR));
  EXPECT_TRUE(timer.IsExpired(now + TEST_ONE_DAY + TEST_ONE_DAY));

  timer.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_WEEK); // repeats for a week
  EXPECT_FALSE(timer.IsExpired(now - TEST_ONE_WEEK));
  EXPECT_FALSE(timer.IsExpired(now));
  EXPECT_FALSE(timer.IsExpired(now + TEST_ONE_WEEK));
  EXPECT_FALSE(timer.IsExpired(now + TEST_ONE_WEEK + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsExpired(now + TEST_ONE_WEEK + TEST_ONE_HOUR));
  EXPECT_TRUE(timer.IsExpired(now + TEST_ONE_WEEK + TEST_ONE_DAY));
}

TEST(TestTimer, IsOccurring)
{
  const CDateTime now = CDateTime::GetUTCDateTime();

  cTimer timer;
  timer.SetTime(now, now + TEST_ONE_HOUR, 0, now); // non-repeating
  EXPECT_FALSE(timer.IsOccurring(now - TEST_ONE_DAY));
  EXPECT_TRUE(timer.IsOccurring(now));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_HOUR));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_DAY));

  timer.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_DAY); // repeats for a day
  EXPECT_FALSE(timer.IsOccurring(now - TEST_ONE_DAY));
  EXPECT_TRUE(timer.IsOccurring(now));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_HOUR));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_DAY - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_DAY));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_DAY + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_DAY + TEST_ONE_HOUR));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_DAY + TEST_ONE_DAY));

  timer.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_WEEK); // repeats for a week
  EXPECT_FALSE(timer.IsOccurring(now - TEST_ONE_DAY));
  EXPECT_TRUE(timer.IsOccurring(now));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_HOUR));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_DAY - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_DAY));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_DAY + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_DAY + TEST_ONE_HOUR));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_DAY + TEST_ONE_DAY));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_WEEK - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_WEEK));
  EXPECT_TRUE(timer.IsOccurring(now + TEST_ONE_WEEK + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_WEEK + TEST_ONE_HOUR));
  EXPECT_FALSE(timer.IsOccurring(now + TEST_ONE_WEEK + TEST_ONE_DAY));
}

TEST(TestTimer, IsPending)
{
  const CDateTime now = CDateTime::GetUTCDateTime();

  cTimer timer;
  timer.SetTime(now, now + TEST_ONE_HOUR, 0, now); // non-repeating
  EXPECT_TRUE(timer.IsPending(now - TEST_ONE_DAY));
  EXPECT_TRUE(timer.IsPending(now - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_HOUR));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_DAY));

  timer.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_DAY); // repeats for a day
  EXPECT_TRUE(timer.IsPending(now - TEST_ONE_DAY));
  EXPECT_TRUE(timer.IsPending(now - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsPending(now + TEST_ONE_HOUR));
  EXPECT_TRUE(timer.IsPending(now + TEST_ONE_DAY - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_DAY));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_DAY + TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_DAY + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_DAY + TEST_ONE_HOUR));

  timer.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_WEEK + TEST_ONE_DAY); // repeats for a week and a day
  EXPECT_TRUE(timer.IsPending(now - TEST_ONE_DAY));
  EXPECT_TRUE(timer.IsPending(now - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsPending(now + TEST_ONE_HOUR));
  EXPECT_TRUE(timer.IsPending(now + TEST_ONE_DAY - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_DAY));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_DAY + TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_DAY + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_WEEK));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_WEEK + TEST_ONE_HOUR - TEST_ONE_SECOND));
  EXPECT_TRUE(timer.IsPending(now + TEST_ONE_WEEK + TEST_ONE_HOUR));
  EXPECT_TRUE(timer.IsPending(now + TEST_ONE_WEEK + TEST_ONE_DAY - TEST_ONE_SECOND));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_WEEK + TEST_ONE_DAY));
  EXPECT_FALSE(timer.IsPending(now + TEST_ONE_WEEK + TEST_ONE_DAY + TEST_ONE_HOUR));
}

TEST(TestTimer, ConflictsNonRepeating)
{
  const CDateTime now = CDateTime::GetUTCDateTime();

  cTimer timer1;
  timer1.SetTime(now, now + TEST_ONE_HOUR, 0, now);

  cTimer timer2;
  timer2 = timer1;
  EXPECT_TRUE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_HOUR, now, 0, now);
  EXPECT_FALSE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_HOUR, now + TEST_ONE_SECOND, 0, now);
  EXPECT_TRUE(timer1.Conflicts(timer2));

  timer2.SetTime(now + TEST_ONE_HOUR - TEST_ONE_SECOND, now + TEST_ONE_HOUR, 0, now + TEST_ONE_HOUR);
  EXPECT_TRUE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_DAY, now - TEST_ONE_DAY + TEST_ONE_HOUR, 0, now);
  EXPECT_FALSE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_DAY, now - TEST_ONE_DAY + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_DAY);
  EXPECT_TRUE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_DAY - TEST_ONE_HOUR, now - TEST_ONE_DAY, 0x7f, now + TEST_ONE_DAY);
  EXPECT_FALSE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_DAY - TEST_ONE_HOUR, now - TEST_ONE_DAY + TEST_ONE_SECOND, 0x7f, now + TEST_ONE_DAY);
  EXPECT_TRUE(timer1.Conflicts(timer2));
}

TEST(TestTimer, ConflictsRepeating)
{
  const CDateTime now = CDateTime::GetUTCDateTime();

  cTimer timer1;
  timer1.SetTime(now, now + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_WEEK);

  cTimer timer2;
  timer2 = timer1;
  EXPECT_TRUE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_HOUR, now, 0, now);
  EXPECT_FALSE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_HOUR, now + TEST_ONE_SECOND, 0, now);
  EXPECT_TRUE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_DAY, now - TEST_ONE_DAY + TEST_ONE_HOUR, 0, now);
  EXPECT_FALSE(timer1.Conflicts(timer2));

  timer2.SetTime(now - TEST_ONE_DAY, now - TEST_ONE_DAY + TEST_ONE_HOUR, 0x7f, now + TEST_ONE_WEEK);
  EXPECT_TRUE(timer1.Conflicts(timer2));

  timer2.SetTime(now + TEST_ONE_WEEK + TEST_ONE_HOUR - TEST_ONE_SECOND, now + TEST_ONE_WEEK + TEST_ONE_HOUR, 0, now + TEST_ONE_WEEK + TEST_ONE_HOUR);
  EXPECT_TRUE(timer1.Conflicts(timer2));

  timer2.SetTime(now + TEST_ONE_WEEK + TEST_ONE_HOUR, now + TEST_ONE_WEEK + TEST_ONE_HOUR + TEST_ONE_SECOND, 0, now + TEST_ONE_WEEK + TEST_ONE_HOUR);
  EXPECT_FALSE(timer1.Conflicts(timer2));

  timer2.SetTime(now + TEST_ONE_WEEK + TEST_ONE_DAY, now + TEST_ONE_WEEK + TEST_ONE_DAY + TEST_ONE_HOUR, 0, now + TEST_ONE_WEEK + TEST_ONE_DAY);
  EXPECT_FALSE(timer1.Conflicts(timer2));
}

}
