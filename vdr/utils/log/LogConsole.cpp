#include "LogConsole.h"
#include "platform/threads/threads.h"
#include <stdio.h>

void CLogConsole::Log(sys_log_level_t level, const char* logline)
{
  PLATFORM::CLockObject lock(m_mutex);
  printf("[%ld] %s\n", PLATFORM::CThread::ThreadId(), logline);
}
