#pragma once

#include "platform/threads/threads.h"

class cEpgDataWriter : public PLATFORM::CThread
{
public:
  static cEpgDataWriter& Get(void);
  void Perform(void);

protected:
  virtual void* Process(void);

private:
  cEpgDataWriter(void);

  PLATFORM::CMutex mutex;
};
