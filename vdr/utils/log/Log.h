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

#include "ILog.h"
#include "lib/platform/threads/mutex.h"

namespace VDR
{
#ifndef esyslog
#define esyslog(a...) CLog::Get().Log(SYS_LOG_ERROR, a)
#endif

#ifndef isyslog
#define isyslog(a...) CLog::Get().Log(SYS_LOG_INFO, a)
#endif

#ifndef dsyslog
#define dsyslog(a...) CLog::Get().Log(SYS_LOG_DEBUG, a)
#endif

#define LOG_ERROR         esyslog("ERROR (%s,%d): %m", __FILE__, __LINE__)
#define LOG_ERROR_STR(s)  esyslog("ERROR (%s,%d): %s: %m", __FILE__, __LINE__, s)

class CLog
{
public:
  static CLog& Get(void);
  virtual ~CLog(void) {}

  void SetType(sys_log_type_t type);
  void SetPipe(ILog* pipe);

  void Log(sys_log_level_t level, const char* logline, ...);

private:
  CLog(ILog* pipe);

  ILog*         m_logpipe;
  PLATFORM::CMutex m_mutex;
};
}
