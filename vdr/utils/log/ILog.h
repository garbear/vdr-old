#pragma once

namespace VDR
{
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

class ILog
{
public:
  virtual ~ILog(void) {}

  virtual void Log(sys_log_level_t type, const char* logline) = 0;
  virtual sys_log_type_t Type(void) const = 0;
};
}
