#pragma once

#include "ILog.h"
#include "platform/threads/mutex.h"

namespace VDR
{
class CLogConsole : public ILog
{
public:
  virtual ~CLogConsole(void) {}

  void Log(sys_log_level_t level, const char* logline);
  sys_log_type_t Type(void) const { return SYS_LOG_TYPE_CONSOLE; }

private:
  PLATFORM::CMutex m_mutex;
};
}
