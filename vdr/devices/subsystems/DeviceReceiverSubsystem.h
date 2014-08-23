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
#include "utils/Tools.h"
#include "lib/platform/threads/mutex.h"
#include "lib/platform/threads/threads.h"

#include <map>
#include <set>
#include <stdint.h>

#define MAXPIDHANDLES  64 // TODO: Convert to std::vector

namespace VDR
{

class iReceiver;
class cVideoBuffer;

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

  /*!
   * \brief Attaches the given receiver to this device
   */
  bool AttachReceiver(iReceiver* receiver, const ChannelPtr& channel);

  /*!
   * \brief Detaches the given receiver from this device
   */
  void DetachReceiver(iReceiver* receiver);

  /*!
   * \brief Returns true if we are currently receiving
   */
  bool Receiving(void) const;

  /*!
   * \brief Returns true if this device is currently receiving the given PID
   */
  bool HasPid(uint16_t pid) const;

protected:
  class cPidHandle
  {
  public:
    cPidHandle(void) : pid(0), streamType(0), handle(-1), used(0) { }
    uint16_t pid;
    uint8_t  streamType;
    int      handle;
    int      used;
  };

  /*!
   * \brief Opens the DVR of this device and prepares it to deliver a Transport
   *        Stream for use in a iReceiver
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
  virtual bool SetPid(cPidHandle &handle, ePidType type, bool bOn) = 0;

  /*!
   * Inherited from CThread. Read and dispatch TS packets in a loop.
   */
  virtual void* Process(void);

private:
  /*!
   * \brief Adds a PID to the set of PIDs this device shall receive
   */
  bool AddPid(uint16_t pid, ePidType pidType = ptOther, uint8_t streamType = 0);

  /*!
   * \brief Deletes a PID from the set of PIDs this device shall receive
   */
  void DeletePid(uint16_t pid, ePidType pidType = ptOther);

  /*!
   * \brief Detaches all receivers from this device
   */
  virtual void DetachAllReceivers(void);

  /*!
   * \brief Detaches all receivers from this device for this pid
   */
  void DetachAll(uint16_t pid);

  typedef std::map<iReceiver*, std::set<uint16_t> > ReceiverPidMap; // receiver -> pids

  ReceiverPidMap   m_receiverPids;
  PLATFORM::CMutex m_mutexReceiver;
  cPidHandle       m_pidHandles[MAXPIDHANDLES];
};

}
