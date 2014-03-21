#pragma once

#include "Types.h"
#include "platform/threads/threads.h"

namespace VDR
{
class cEpgDataReader : public PLATFORM::CThread
{
public:
  cEpgDataReader(void);
  virtual void* Process(void);
};
}
