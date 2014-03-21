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
#pragma once

#include "Types.h"
#include <string>
#include <time.h>

namespace VDR
{
class CalendarUtils
{
public:
  /*!
   * \brief  Converts the given WeekDay (0=Sunday, 1=Monday, ...) to a three letter day name
   */
  static std::string WeekDayName(int WeekDay);

  /*!
   * \brief Converts the week day of the given time to a three letter day name
   */
  static std::string WeekDayName(time_t t);

  /*!
   * \brief Converts the given WeekDay (0=Sunday, 1=Monday, ...) to a full day name
   */
  static std::string WeekDayNameFull(int WeekDay);

  /*!
   * \brief Converts the week day of the given time to a full day name
   */
  static std::string WeekDayNameFull(time_t t);

  /*!
   * \brief Converts the given time to a string of the form "www dd.mm. hh:mm"
   *
   * If no time is given, the current time is taken.
   */
  static std::string DayDateTime(time_t t = 0);

  /*!
   * \brief Converts the given time to a string of the form "www mmm dd hh:mm:ss yyyy"
   */
  static std::string TimeToString(time_t t);

  /*!
   * \brief Converts the given time to a string of the form "www dd.mm.yyyy"
   */
  static std::string DateString(time_t t);

  /*!
   * \brief Converts the given time to a string of the form "dd.mm.yy"
   */
  static std::string ShortDateString(time_t t);

  /*!
   * \brief Converts the given time to a string of the form "hh:mm"
   */
  static std::string TimeString(time_t t);
};
}
