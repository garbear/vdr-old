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

#include <map>
#include <stdint.h>

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
public:
  cDeviceReceiverSubsystem(cDevice *device);
  virtual ~cDeviceReceiverSubsystem(void);

  void Start(void);
  void Stop(void);

  /*!
   * Attaches the given receiver to this device. Resources for all the streams
   * that channel provides are associated with the receiver in the m_receiverResources
   * map.
   */
  bool AttachReceiver(iReceiver* receiver, const ChannelPtr& channel);
  bool AttachReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type = STREAM_TYPE_UNDEFINED);

  /*!
   * Detaches the given receiver from this device. Pointer is removed from
   * m_receiverResources, and all resources that fall out of scope will be
   * closed RAII-style.
   */
  void DetachReceiver(iReceiver* receiver);
  void DetachReceiverPid(iReceiver* receiver, uint16_t pid);

  /*!
   * Returns true if any receivers are attached to this device.
   */
  bool Receiving(void) const;

  bool HasReceiver(iReceiver* receiver) const;
  bool HasReceiverPid(iReceiver* receiver, uint16_t pid) const;

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
   * \brief Does the actual PID setting on this device.
   * \param bOn Indicates whether the PID shall be added or deleted
   * \param handle The PID handle
   *        * handle->handle can be used by the device to store information it
   *          needs to receive this PID (for instance a file handle)
   *        * handle->used indicates how many receivers are using this PID
   * \param type Indicates some special types of PIDs, which the device may need
   *        to set in a specific way.
   */
  virtual PidResourcePtr CreateResource(uint16_t pid, STREAM_TYPE streamType) = 0;

  virtual bool Poll(void) = 0;

  /*!
   * Gets exactly one TS packet from the DVR of this device, or returns false if
   * no new data is ready.
   */
  virtual bool Read(std::vector<uint8_t>& data) = 0;

private:
  /*!
   * \brief Detaches all receivers from this device.
   */
  virtual void DetachAllReceivers(void);

  bool OpenResourceForReceiver(uint16_t pid, STREAM_TYPE streamType, iReceiver* receiver);
  PidReceivers* GetReceivers(uint16_t pid, STREAM_TYPE streamType);
  void CloseResourceForReceiver(iReceiver* receiver);
  void CloseResourceForReceiver(uint16_t pid, iReceiver* receiver);
  bool SyncResources(void);

  typedef struct
  {
    iReceiver*  receiver;
    STREAM_TYPE type;
    uint16_t    pid;
  } addedReceiver;

  PidResourceMap                    m_resourcesActive;
  std::vector<addedReceiver>        m_resourcesAdded;
  std::map<uint16_t, iReceiver*>    m_resourcesRemoved;
  std::set<iReceiver*>              m_receiversRemoved;
  PLATFORM::CMutex    m_mutexReceiverRead;
  PLATFORM::CMutex    m_mutexReceiverWrite;
};

}
