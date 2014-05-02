/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2014 Team XBMC
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

#include "devices/DeviceTypes.h"
#include "devices/commoninterface/CI.h"

namespace VDR
{
class cDvbCiAdapter : public cCiAdapter
{
public:
  virtual ~cDvbCiAdapter();
  static cDvbCiAdapter *CreateCiAdapter(cDevice *Device, int Fd);

protected:
  cDvbCiAdapter(cDevice *device, int fd);

  virtual int Read(uint8_t *Buffer, int MaxLength);
  virtual void Write(const uint8_t *Buffer, int Length);

  virtual bool Reset(int Slot);
  virtual eModuleStatus ModuleStatus(int Slot);
  virtual bool Assign(DevicePtr Device, bool Query = false);

private:
  cDevice *m_device;
  int      m_fd;
};
}
