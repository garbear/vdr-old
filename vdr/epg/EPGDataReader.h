#pragma once

#include "platform/threads/threads.h"

class cEpgDataReader : public PLATFORM::CThread
{
public:
  cEpgDataReader(void);
  virtual void* Process(void);
};
