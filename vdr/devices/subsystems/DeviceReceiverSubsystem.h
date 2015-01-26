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
#include <memory>
#include <list>
#include <map>
#include <queue>

namespace VDR
{

class iReceiver;

enum POLL_RESULT
{
  POLL_RESULT_NOT_READY,
  POLL_RESULT_STREAMING_READY,
  POLL_RESULT_MULTIPLEXED_READY
};

enum RECEIVER_CHANGE
{
  RCV_CHANGE_ATTACH_MULTIPLEXED,
  RCV_CHANGE_ATTACH_STREAMING,
  RCV_CHANGE_DETACH_ALL,
  RCV_CHANGE_DETACH,
  RCV_CHANGE_DETACH_MULTIPLEXED,
  RCV_CHANGE_DETACH_STREAMING
};

class cDeviceReceiverSubsystem : protected cDeviceSubsystem, public PLATFORM::CThread
{
protected:
  class cReceiverChange
  {
  public:
    cReceiverChange(RECEIVER_CHANGE type) :
      m_type(type),
      m_receiver(NULL),
      m_pid(~0),
      m_tid(~0),
      m_mask(~0),
      m_streamType(STREAM_TYPE_UNDEFINED) {}
    cReceiverChange(RECEIVER_CHANGE type, iReceiver* receiver) :
      m_type(type),
      m_receiver(receiver),
      m_pid(~0),
      m_tid(~0),
      m_mask(~0),
      m_streamType(STREAM_TYPE_UNDEFINED) {}
    cReceiverChange(RECEIVER_CHANGE type, iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask) :
      m_type(type),
      m_receiver(receiver),
      m_pid(pid),
      m_tid(tid),
      m_mask(mask),
      m_streamType(STREAM_TYPE_UNDEFINED) {}
    cReceiverChange(RECEIVER_CHANGE type, iReceiver* receiver, uint16_t pid, STREAM_TYPE streamType = STREAM_TYPE_UNDEFINED) :
      m_type(type),
      m_receiver(receiver),
      m_pid(pid),
      m_tid(~0),
      m_mask(~0),
      m_streamType(streamType) {}

    virtual ~cReceiverChange(void) {}

    RECEIVER_CHANGE m_type;
    iReceiver*      m_receiver;
    uint16_t        m_pid;
    uint8_t         m_tid;
    uint8_t         m_mask;
    STREAM_TYPE     m_streamType;
  };

  class cReceiverHandle
  {
  public:
    cReceiverHandle(iReceiver* rcvr);
    virtual ~cReceiverHandle(void);

    bool Start(void);

    iReceiver* const receiver;
  };

  typedef std::shared_ptr<cPidResource>                PidResourcePtr;
  typedef std::shared_ptr<cReceiverHandle>             ReceiverHandlePtr;
  typedef std::pair<ReceiverHandlePtr, PidResourcePtr> ReceiverPidEdge;

  typedef std::list<ReceiverPidEdge>                   ReceiverList;
  typedef std::map<uint16_t, ReceiverList>             ReceiverPidTable;

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
  void DetachReceiver(iReceiver* receiver, bool wait);
  void DetachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask, bool wait);
  void DetachMultiplexedReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type = STREAM_TYPE_UNDEFINED, bool wait = false);

  /*!
   * \brief Detaches all receivers from this device.
   */
  virtual void DetachAllReceivers(bool wait);

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

  void ProcessChanges(void);

protected:
  ReceiverHandlePtr    GetReceiverHandle(iReceiver* receiver) const;
  std::set<iReceiver*> GetReceiversForPid(uint16_t pid) const;
  PidResourcePtr       GetResource(const PidResourcePtr& needle) const;
  PidResourcePtr       GetMultiplexedResource(uint16_t pid) const; // Streaming resources can't be identified by PID alone
  bool                 AttachReceiver(iReceiver* receiver, const PidResourcePtr& resource);

  ReceiverPidTable m_receiverPidTable;// Receiver <-> PID associations

  PLATFORM::CMutex             m_mutex;
  std::queue<cReceiverChange*> m_receiverChanges;
  PLATFORM::CCondition<bool>   m_pidChange;
  PLATFORM::CCondition<bool>   m_pidChangeProcessed;
  bool                         m_changed;
  bool                         m_changeProcessed;
};

}
