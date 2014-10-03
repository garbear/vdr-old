/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "channels/ChannelTypes.h"
#include "devices/TunerHandle.h"

#include <stdint.h>
#include <vector>

#define TRANSPONDER_TIMEOUT 5000

namespace VDR
{

class iReceiver : public iTunerHandleCallbacks
{
public:
  virtual ~iReceiver(void) { }

  /*!
   * Called just before the receiver is attached to a device. It can be used to
   * do things like starting a thread. It is guaranteed that Receive() will not
   * be called before Start().
   */
  virtual void Start(void) = 0;

  /*!
   * Called just before the receiver is removed from a device. It can be used to
   * do things like stopping a thread.
   */
  virtual void Stop(void) = 0;

  /*!
   * This function is called from the cDevice we are attached to, and delivers
   * one TS packet from the set of PIDs the iReceiver has requested. The data
   * packet must be accepted immediately, and the call must return as soon as
   * possible, without any unnecessary delay. Each TS packet will be delivered
   * only ONCE, so the iReceiver must make sure that it will be able to buffer
   * the data if necessary.
   */
  virtual void Receive(const std::vector<uint8_t>& data) = 0;

  virtual void LockAcquired(void) {}

  virtual void LockLost(void) {}

  virtual void LostPriority(void) {}

  std::string DumpData(const std::vector<uint8_t>& data)
  {
    char dump[1000];
    int wptr = 0;
    for (unsigned int ptr = 0; ptr < 188 && ptr < data.size(); ++ptr)
      wptr += snprintf(dump + wptr, 4, "%02X:", data[ptr]);
    dump[wptr] = 0;
    return dump;
  }
private:
};

}
