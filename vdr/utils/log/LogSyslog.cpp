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
