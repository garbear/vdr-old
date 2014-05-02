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

namespace VDR
{
#define MAXPIDHANDLES      64 // the maximum number of different PIDs per device

enum ePidType
{
  ptAudio,
  ptVideo,
  ptPcr,
  ptTeletext,
  ptDolby,
  ptOther
};

class cDevice;

class cDevicePIDSubsystem : protected cDeviceSubsystem
{
public:
  cDevicePIDSubsystem(cDevice *device);
  virtual ~cDevicePIDSubsystem() { }

  /*!
   * \brief Deletes the live viewing PIDs
   */
  void DelLivePids();

protected:
  class cPidHandle
  {
  public:
    cPidHandle(void) : pid(0), streamType(0), handle(-1), used(0) { }
    int pid;
    int streamType;
    int handle;
    int used;
  };

  /*!
   * \brief Returns true if this device is currently receiving the given PID
   */
public: // TODO
  bool HasPid(int pid) const;

  /*!
   * \brief Adds a PID to the set of PIDs this device shall receive
   */
  bool AddPid(int pid, ePidType pidType = ptOther, int streamType = 0);

  /*!
   * \brief Deletes a PID from the set of PIDs this device shall receive
   */
  void DelPid(int pid, ePidType pidType = ptOther);

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
  virtual bool SetPid(cPidHandle &handle, ePidType type, bool bOn) { return false; }

private:
  void PrintPIDs(const char *s);

public: // TODO
  cPidHandle m_pidHandles[MAXPIDHANDLES];
};
}
