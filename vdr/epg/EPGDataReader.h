#pragma once

#include "Types.h"
#include "platform/threads/threads.h"

class cEpgDataReader : public PLATFORM::CThread
{
public:
  cEpgDataReader(void);
  virtual void* Process(void);
};
