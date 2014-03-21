#include "LogConsole.h"
#include <stdio.h>

namespace VDR
{

void CLogConsole::Log(sys_log_level_t level, const char* logline)
{
  PLATFORM::CLockObject lock(m_mutex);
  printf("%s\n", logline);
}

}
