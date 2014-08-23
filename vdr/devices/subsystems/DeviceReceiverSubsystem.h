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

#include "devices/DeviceSubsystem.h"
#include "utils/Tools.h"
#include "lib/platform/threads/mutex.h"
#include "lib/platform/threads/threads.h"
#include "vnsi/video/VideoInput.h"

#include <list>
#include <stdint.h>

namespace VDR
{
class cReceiver;

class cDeviceReceiverSubsystem : protected cDeviceSubsystem, public PLATFORM::CThread
{
public:
  cDeviceReceiverSubsystem(cDevice *device);
  virtual ~cDeviceReceiverSubsystem() { }

  /*!
   * \brief Attaches the given receiver to this device
   */
  void AttachReceiver(cReceiver* receiver);

  /*!
   * \brief Detaches the given receiver from this device
   */
  void Detach(cReceiver* receiver);

  /*!
   * \brief Detaches all receivers from this device for this pid
   */
  void DetachAll(int pid);

  /*!
   * \brief Detaches all receivers from this device
   */
  virtual void DetachAllReceivers(void);

  bool OpenVideoInput(const ChannelPtr& channel, cVideoBuffer* videoBuffer);
  void CloseVideoInput(void);

  /*!
   * \brief Returns true if we are currently receiving
   */
  bool Receiving(void) const;

protected:
  virtual void* Process(void);

  /*!
   * \brief Opens the DVR of this device and prepares it to deliver a Transport
   *        Stream for use in a cReceiver
   */
  virtual bool OpenDvr() = 0;

  /*!
   * \brief Shuts down the DVR
   */
  virtual void CloseDvr() = 0;

  /*!
   * \brief Gets exactly one TS packet from the DVR of this device and returns
   *        a pointer to it in data
   * \return False in case of a non-recoverable error, otherwise true (even if
   *         data is NULL)
   *
   * Only the first 188 bytes (TS_SIZE) data points to are valid and may be
   * accessed. If there is currently no new data available, data will be set to
   * NULL.
   */
  virtual bool GetTSPacket(uint8_t *&data) = 0;

private:
  PLATFORM::CMutex  m_mutexReceiver;
public: // TODO
  std::list<cReceiver*> m_receivers;

private:
  cVideoInput   m_VideoInput;
};
}
