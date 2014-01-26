#pragma once

#include "platform/threads/threads.h"

class cEpgDataWriter : public PLATFORM::CThread
{
public:
  static cEpgDataWriter& Get(void);
  void SetDump(bool Dump) { dump = Dump; }
  void Perform(void);

protected:
  virtual void* Process(void);

private:
  cEpgDataWriter(void);

  PLATFORM::CMutex mutex;
  bool             dump;
};
