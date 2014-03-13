#pragma once

#ifdef ANDROID
#define __attribute_format_arg__(x) __attribute__ ((__format_arg__ (x)))
#endif

#ifndef esyslog
#define esyslog(a...) CSysLog::Get().Log(SYS_LOG_ERROR, a)
#endif

#ifndef isyslog
#define isyslog(a...) CSysLog::Get().Log(SYS_LOG_INFO, a)
#endif

#ifndef dsyslog
#define dsyslog(a...) CSysLog::Get().Log(SYS_LOG_DEBUG, a)
#endif

#define LOG_ERROR         esyslog("ERROR (%s,%d): %m", __FILE__, __LINE__)
#define LOG_ERROR_STR(s)  esyslog("ERROR (%s,%d): %s: %m", __FILE__, __LINE__, s)

typedef enum
{
  SYS_LOG_NONE = 0,
  SYS_LOG_ERROR,
  SYS_LOG_INFO,
  SYS_LOG_DEBUG
} sys_log_level_t;

typedef enum
{
  SYS_LOG_TYPE_CONSOLE = 0,
  SYS_LOG_TYPE_SYSLOG,
  SYS_LOG_TYPE_ADDON
} sys_log_type_t;

class CSysLog
{
public:
  static CSysLog& Get(void);
  virtual ~CSysLog(void) {}

  void Log(sys_log_level_t type, const char* format, ...);
  void SetType(sys_log_type_t type);

private:
  CSysLog(void);
};
