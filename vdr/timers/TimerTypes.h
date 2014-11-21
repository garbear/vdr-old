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

#include <map>
#include <memory>
#include <vector>

#define TIMER_INVALID_ID      0

#define TIMER_MASK_SUNDAY     (1 << 0)
#define TIMER_MASK_MONDAY     (1 << 1)
#define TIMER_MASK_TUESDAY    (1 << 2)
#define TIMER_MASK_WEDNESDAY  (1 << 3)
#define TIMER_MASK_THURSDAY   (1 << 4)
#define TIMER_MASK_FRIDAY     (1 << 5)
#define TIMER_MASK_SATURDAY   (1 << 6)

namespace VDR
{

class cTimer;
typedef std::shared_ptr<cTimer> TimerPtr;
typedef std::vector<TimerPtr>   TimerVector;
typedef std::map<unsigned int, TimerPtr> TimerMap; // ID -> timer

}
