#include "LogSyslog.h"
#include "platform/threads/threads.h"
#include "settings/Settings.h"
#include <syslog.h>

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
  syslog(priority, "[%ld] %s", PLATFORM::CThread::ThreadId(), logline);
}
