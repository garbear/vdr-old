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

#include "devices/subsystems/DevicePIDSubsystem.h"

namespace VDR
{
class cDvbPIDSubsystem : public cDevicePIDSubsystem
{
public:
  cDvbPIDSubsystem(cDevice *device);
  virtual ~cDvbPIDSubsystem() { }

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
  virtual bool SetPid(cPidHandle &handle, ePidType type, bool bOn);
};
}
