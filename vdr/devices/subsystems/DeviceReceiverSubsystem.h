/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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
#include "platform/threads/threads.h"

class cReceiver;

class cDeviceReceiverSubsystem : protected cDeviceSubsystem
{
public:
  cDeviceReceiverSubsystem(cDevice *device);
  virtual ~cDeviceReceiverSubsystem() { }

public:
  /*!
   * \brief Returns the priority of the current receiving session
   * \return Value in the range -MAXPRIORITY..MAXPRIORITY, or IDLEPRIORITY if no
   *         receiver is currently active
   */
  int Priority(void) const;

  /*!
   * \brief Returns true if we are currently receiving
   */
  bool Receiving(void) const;

  /*!
   * \brief Attaches the given receiver to this device
   */
  bool AttachReceiver(cReceiver *receiver);

  /*!
   * \brief Detaches the given receiver from this device
   */
  void Detach(cReceiver *receiver);

  /*!
   * \brief Detaches all receivers from this device for this pid
   */
  void DetachAll(int pid);

  /*!
   * \brief Detaches all receivers from this device
   */
  virtual void DetachAllReceivers();

//protected:
  /*!
   * \brief Opens the DVR of this device and prepares it to deliver a Transport
   *        Stream for use in a cReceiver
   */
  virtual bool OpenDvr() { return false; }

  /*!
   * \brief Shuts down the DVR
   */
  virtual void CloseDvr() { }

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
  virtual bool GetTSPacket(uchar *&data) { return false; }

private:
  PLATFORM::CMutex  m_mutexReceiver;
public: // TODO
  std::list<cReceiver*> m_receivers;
};
