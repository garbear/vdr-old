#pragma once

#include "ILog.h"
#include "platform/threads/mutex.h"

#ifdef ANDROID
#define __attribute_format_arg__(x) __attribute__ ((__format_arg__ (x)))
#endif

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
