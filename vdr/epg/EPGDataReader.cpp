#include "EPGDataReader.h"
#include "EPG.h"

cEpgDataReader::cEpgDataReader(void)
{
}

void* cEpgDataReader::Process(void)
{
  cSchedulesLock lock(true);
  cSchedules* schedules = lock.Get();
  if (schedules)
    schedules->Read();
  return NULL;
}