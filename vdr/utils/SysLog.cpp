#include "SysLog.h"
#include "settings/Settings.h"
#include "platform/threads/threads.h"
#include <stdarg.h>

#define MAXSYSLOGBUF (256)
#define LOG_STDOUT   (1)
#define LOG_SYSLOG   (0)

#if LOG_SYSLOG
#include <syslog.h>
#endif

using namespace PLATFORM;

// linux only
#ifndef LOG_EMERG
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */
#endif

CSysLog::CSysLog(void)
{
}

CSysLog& CSysLog::Get(void)
{
  static CSysLog _instance;
  return _instance;
}

void CSysLog::Log(sys_log_level_t type, const char* format, ...)
{
  //TODO
  if (type > cSettings::Get().m_SysLogLevel)
    return;

  va_list ap;
  char fmt[MAXSYSLOGBUF];
  snprintf(fmt, sizeof(fmt), "[%ld] %s", PLATFORM::CThread::ThreadId(), format);
  va_start(ap, format);

  switch (cSettings::Get().m_SysLogType)
  {
  case SYS_LOG_TYPE_CONSOLE:
#if LOG_STDOUT
    {
      static PLATFORM::CMutex g_logMutex;
      PLATFORM::CLockObject lock(g_logMutex);
      vprintf(fmt, ap);
      printf("\n");
    }
#endif
    break;
  case SYS_LOG_TYPE_SYSLOG:
#if LOG_SYSLOG
    {
      int priority = LOG_ERR;
      switch (type)
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
      vsyslog(priority, fmt, ap);
    }
#endif
    break;
  case SYS_LOG_TYPE_ADDON:
    // TODO
    break;
  default:
    break;
  }

  va_end(ap);
}

void CSysLog::SetType(sys_log_type_t type)
{
#if LOG_SYSLOG
  if (cSettings::Get().m_SysLogType == SYS_LOG_TYPE_SYSLOG && cSettings::Get().m_SysLogLevel != SYS_LOG_NONE)
    openlog("vdr", LOG_CONS, cSettings::Get().m_SysLogTarget); // LOG_PID doesn't work as expected under NPTL
#endif
}
