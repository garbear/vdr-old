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

#include "Log.h"
#include "settings/Settings.h"
#include "LogConsole.h"
#include "LogSyslog.h"
#include "platform/threads/threads.h"
#include <stdarg.h>

#define MAXSYSLOGBUF (256)

namespace VDR
{

CLog::CLog(ILog* pipe) :
    m_logpipe(pipe)
{
}

CLog& CLog::Get(void)
{
  static CLog _instance(new CLogConsole);
  return _instance;
}

void CLog::Log(sys_log_level_t level, const char* format, ...)
{
  if (level > cSettings::Get().m_SysLogLevel)
    return;

  char fmt[MAXSYSLOGBUF];
  char buf[MAXSYSLOGBUF];
  va_list ap;

  va_start(ap, format);
  snprintf(fmt, sizeof(fmt), "[%ld] %s", PLATFORM::CThread::ThreadId(), format);
  vsnprintf(buf, MAXSYSLOGBUF - 1, fmt, ap);
  va_end(ap);

  PLATFORM::CLockObject lock(m_mutex);
  if (m_logpipe)
    m_logpipe->Log(level, buf);
}

void CLog::SetType(sys_log_type_t type)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (!m_logpipe || m_logpipe->Type() != type)
  {
    switch (type)
    {
    case SYS_LOG_TYPE_CONSOLE:
      SetPipe(new CLogConsole);
      break;
    case SYS_LOG_TYPE_SYSLOG:
      SetPipe(new CLogSyslog);
      break;
    default:
      SetPipe(NULL);
      break;
    }
  }
}

void CLog::SetPipe(ILog* pipe)
{
  PLATFORM::CLockObject lock(m_mutex);
  delete m_logpipe;
  m_logpipe = pipe;
}

}
