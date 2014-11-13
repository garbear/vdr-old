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
#include "devices/DeviceSubsystem.h"
#include "devices/DeviceTypes.h"
#include "devices/PIDResource.h"
#include "lib/platform/threads/mutex.h"
#include "lib/platform/threads/threads.h"

#include <set>
#include <stdint.h>
#include <utility>

namespace VDR
{

class iReceiver;
class cRingBufferLinear;

enum ePidType
{
  ptAudio,
  ptVideo,
  ptPcr,
  ptTeletext,
  ptDolby,
  ptOther
};

class cDeviceReceiverSubsystem : protected cDeviceSubsystem, public PLATFORM::CThread
{
protected:
  class cReceiverHandle
  {
  public:
    cReceiverHandle(iReceiver* rcvr);
    ~cReceiverHandle(void);
    iReceiver* const receiver;
  };

  typedef VDR::shared_ptr<cPidResource>                PidResourcePtr;
  typedef VDR::shared_ptr<cReceiverHandle>             ReceiverHandlePtr;
  typedef std::pair<ReceiverHandlePtr, PidResourcePtr> ReceiverPidEdge;
  typedef std::set<ReceiverPidEdge>                    ReceiverPidTable; // Junction table to store relationships

public:
  cDeviceReceiverSubsystem(cDevice *device);
  virtual ~cDeviceReceiverSubsystem(void);

  void Start(void);
  void Stop(void);

  /*!
   * Attaches the given receiver to this device. Resources for all the streams
   * that channel provides are opened and associated with the receiver.
   */
  bool AttachReceiver(iReceiver* receiver, const ChannelPtr& channel);
  bool AttachReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type = STREAM_TYPE_UNDEFINED);

  /*!
   * Detaches the given receiver from this device.
   */
  void DetachReceiver(iReceiver* receiver);
  void DetachReceiverPid(iReceiver* receiver, uint16_t pid);

  /*!
   * Returns true if any receivers are attached to this device.
   */
  bool Receiving(void) const;

  /*!
   * \brief Detaches all receivers from this device.
   */
  virtual void DetachAllReceivers(void);

protected:
  /*!
   * Inherited from CThread. Read and dispatch TS packets in a loop.
   */
  virtual void* Process(void);

  /*!
   * \brief Opens the DVR of this device and prepares it to deliver a Transport
   *        Stream for use in a iReceiver
   */
  virtual bool Initialise(void) = 0;

  /*!
   * \brief Shuts down the DVR
   */
  virtual void Deinitialise(void) = 0;

  /*!
   * Allocate a new resource. Must not return an empty pointer.
   */
  virtual PidResourcePtr CreateResource(uint16_t pid, STREAM_TYPE streamType) = 0;

  /*!
   * Wait until data is ready to be read.
   */
  virtual bool Poll(void) = 0;

  /*!
   * Gets exactly one TS packet from the DVR of this device, or returns false if
   * no new data is ready.
   */
  virtual TsPacket Read(void) = 0;

  /*!
   * Report that the TS packet delivered by Read() was used.
   */
  virtual void Consumed(void) = 0;

private:
  ReceiverHandlePtr    GetReceiverHandle(iReceiver* receiver) const;
  std::set<iReceiver*> GetReceivers(void) const;
  PidResourcePtr       GetResource(uint16_t pid) const;

  ReceiverPidTable m_receiverPidTable; // Receiver <-> PID associations
  PLATFORM::CMutex m_mutex;
};

}
