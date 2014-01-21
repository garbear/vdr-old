//TODO copyright vdr

#include "throttle.h"

using namespace PLATFORM;

CMutex cIoThrottle::mutex;
int cIoThrottle::count = 0;

cIoThrottle::cIoThrottle(void)
{
  active = false;
}

cIoThrottle::~cIoThrottle()
{
  Release();
}

void cIoThrottle::Activate(void)
{
  if (!active)
  {
    mutex.Lock();
    count++;
    active = true;
    mutex.Unlock();
  }
}

void cIoThrottle::Release(void)
{
  if (active)
  {
    mutex.Lock();
    count--;
    active = false;
    mutex.Unlock();
  }
}

bool cIoThrottle::Engaged(void)
{
  return count > 0;
}

