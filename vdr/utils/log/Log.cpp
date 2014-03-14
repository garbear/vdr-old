#include "Log.h"
#include "settings/Settings.h"
#include "LogConsole.h"
#include "LogSyslog.h"
#include "platform/threads/threads.h"
#include <stdarg.h>

#define MAXSYSLOGBUF (256)

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
