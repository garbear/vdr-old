#pragma once
//TODO copyright vdr

#include "mutex.h"

namespace PLATFORM
{
class cIoThrottle {
private:
  static CMutex mutex;
  static int count;
  bool active;
public:
  cIoThrottle(void);
  ~cIoThrottle();
  void Activate(void);
       ///< Activates the global I/O throttling mechanism.
       ///< This function may be called any number of times, but only
       ///< the first call after an inactive state will have an effect.
  void Release(void);
       ///< Releases the global I/O throttling mechanism.
       ///< This function may be called any number of times, but only
       ///< the first call after an active state will have an effect.
  bool Active(void) { return active; }
       ///< Returns true if this I/O throttling object is currently active.
  static bool Engaged(void);
       ///< Returns true if any I/O throttling object is currently active.
  };
}
