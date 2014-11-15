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

enum POLL_RESULT
{
  POLL_RESULT_NOT_READY,
  POLL_RESULT_STREAMING_READY,
  POLL_RESULT_MULTIPLEXED_READY
};

class cDeviceReceiverSubsystem : protected cDeviceSubsystem, public PLATFORM::CThread
{
protected:
  class cReceiverHandle
  {
  public:
    cReceiverHandle(iReceiver* rcvr);
    ~cReceiverHandle(void);
    bool Start(void);
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
  bool AttachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask);
  bool AttachMultiplexedReceiver(iReceiver* receiver, const ChannelPtr& channel);
  bool AttachMultiplexedReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type = STREAM_TYPE_UNDEFINED);

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
  virtual PidResourcePtr CreateStreamingResource(uint16_t pid, uint8_t tid, uint8_t mask) = 0;
  virtual PidResourcePtr CreateMultiplexedResource(uint16_t pid, STREAM_TYPE streamType) = 0;

  /*!
   * Wait until data is ready to be read. Poll() returns three conditions:
   *
   * POLL_RESULT_NOT_READY:
   *     Poll() timed out and no streaming or multiplexed resources are ready to
   *     be read.
   *
   * POLL_RESULT_STREAMING_READY:
   *     A streaming resource is ready to be read. streamingResource is set to
   *     the ready resource.
   *
   * POLL_RESULT_MULTIPLEXED_READY:
   *     A multiplexed resources is ready to be ready. streamingResource is ignored.
   */
  virtual POLL_RESULT Poll(PidResourcePtr& streamingResource) = 0;

  /*!
   * Gets exactly one TS packet from the DVR of this device, or returns NULL if
   * no new data is ready.
   */
  virtual TsPacket ReadMultiplexed(void) = 0;

  /*!
   * Report that the TS packet delivered by Read() was used.
   */
  virtual void Consumed(void) = 0;

  std::set<PidResourcePtr> GetResources(void) const;

private:
  ReceiverHandlePtr    GetReceiverHandle(iReceiver* receiver) const;
  std::set<iReceiver*> GetReceivers(void) const;
  std::set<iReceiver*> GetReceivers(const PidResourcePtr& resource) const;
  PidResourcePtr       GetResource(uint16_t pid) const;

  ReceiverPidTable m_receiverPidTable; // Receiver <-> PID associations
  PLATFORM::CMutex m_mutex;
};

}
