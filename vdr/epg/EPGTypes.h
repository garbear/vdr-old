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

#include <shared_ptr/shared_ptr.hpp>
#include <sys/types.h>
#include <vector>

namespace VDR
{

class cSchedule;
typedef VDR::shared_ptr<cSchedule> SchedulePtr;
typedef std::vector<SchedulePtr>   ScheduleVector;

class cEvent;
typedef VDR::shared_ptr<cEvent> EventPtr;
typedef std::vector<EventPtr>   EventVector;

typedef u_int32_t tEventID;

}
