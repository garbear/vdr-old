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
#include "LogSyslog.h"
#include "settings/Settings.h"

#if !defined(TARGET_ANDROID)
#include <syslog.h>
#endif

namespace VDR
{

#if !defined(TARGET_ANDROID)
CLogSyslog::CLogSyslog(void)
{
  openlog("vdr", LOG_CONS, cSettings::Get().m_SysLogTarget); // LOG_PID doesn't work as expected under NPTL
}

CLogSyslog::~CLogSyslog(void)
{
  closelog();
}

void CLogSyslog::Log(sys_log_level_t level, const char* logline)
{
  int priority = LOG_ERR;
  switch (level)
  {
  case SYS_LOG_ERROR:
    priority = LOG_ERR;
    break;
  case SYS_LOG_INFO:
    priority = LOG_INFO;
    break;
  case SYS_LOG_DEBUG:
    priority = LOG_DEBUG;
    break;
  default:
    return;
  }
  syslog(priority, "%s", logline);
}

#else
CLogSyslog::CLogSyslog(void) {}
CLogSyslog::~CLogSyslog(void) {}
void CLogSyslog::Log(sys_log_level_t level, const char* logline) {}
#endif

}
