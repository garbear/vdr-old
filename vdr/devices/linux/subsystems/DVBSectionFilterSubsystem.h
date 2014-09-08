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

#include "devices/subsystems/DeviceSectionFilterSubsystem.h"
#include "devices/DeviceTypes.h"

namespace VDR
{
class cDvbSectionFilterSubsystem : public cDeviceSectionFilterSubsystem
{
public:
  cDvbSectionFilterSubsystem(cDevice *device);
  virtual ~cDvbSectionFilterSubsystem() { }

protected:
  virtual bool Initialise(void) { return true; }
  virtual void Deinitialise(void) { }

  virtual PidResourcePtr CreateResource(uint16_t pid, uint8_t tid, uint8_t mask);
  virtual bool ReadResource(const PidResourcePtr& handle, std::vector<uint8_t>& data);
  virtual PidResourcePtr Poll(const PidResourceSet& filterResources);
};
}
