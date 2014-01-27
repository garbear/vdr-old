#include "EPGDataWriter.h"
#include "EPG.h"
#include <time.h>

cEpgDataWriter::cEpgDataWriter(void)
{
  dump = false;
}

cEpgDataWriter& cEpgDataWriter::Get(void)
{
  static cEpgDataWriter _instance;
  return _instance;
}

void* cEpgDataWriter::Process(void)
{
  Perform();
  return NULL;
}

void cEpgDataWriter::Perform(void)
{
  PLATFORM::CLockObject lock(mutex); // to make sure fore- and background calls don't cause parellel dumps!
  cSchedulesLock SchedulesLock(true, 1000);
  cSchedules *s = SchedulesLock.Get();
  if (s)
  {
    s->CleanTables();

    if (dump)
      s->Save();
  }
}
