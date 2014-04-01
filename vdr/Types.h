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

#include "platform/os.h"
#include "utils/log/Log.h"
#include <shared_ptr/shared_ptr.hpp>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <iconv.h>
#include <math.h>
#include <poll.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(TARGET_ANDROID)
#include "utils/android/sort.h"
#include "utils/android/strchrnul.h"

// Android does not declare mkdtemp even though it's implemented.
extern "C" char *mkdtemp(char *path);

#endif

namespace VDR
{
class cChannel;
typedef VDR::shared_ptr<cChannel> ChannelPtr;

class cDevice;
typedef VDR::shared_ptr<cDevice> DevicePtr;

class cSchedule;
typedef VDR::shared_ptr<cSchedule> SchedulePtr;

class cTimer;
typedef VDR::shared_ptr<cTimer> TimerPtr;
}
