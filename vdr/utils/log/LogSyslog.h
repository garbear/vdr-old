#pragma once

#include "ILog.h"

class CLogSyslog : public ILog
{
public:
  CLogSyslog(void);
  virtual ~CLogSyslog(void);

  void Log(sys_log_level_t level, const char* logline);
  sys_log_type_t Type(void) const { return SYS_LOG_TYPE_SYSLOG; }
};
