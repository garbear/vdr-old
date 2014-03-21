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

#include "Types.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"

namespace VDR
{
class cTSBuffer;

class cDvbReceiverSubsystem : public cDeviceReceiverSubsystem
{
public:
  cDvbReceiverSubsystem(cDevice *device);
  virtual ~cDvbReceiverSubsystem() { }

  virtual bool OpenDvr();
  virtual void CloseDvr();
  virtual bool GetTSPacket(uchar *&data);

  // Overrides public method as protected
  virtual void DetachAllReceivers();

private:
  cTSBuffer *m_tsBuffer;

  // The DVR device (will be opened and closed as needed)
  int m_fd_dvr;
};
}
