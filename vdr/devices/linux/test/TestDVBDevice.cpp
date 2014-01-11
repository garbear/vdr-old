/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "devices/linux/DVBDevice.h"
#include "gtest/gtest.h"

#include <vector>

using namespace std;

// Hauppauge WinTV-HVR 950Q hybrid TV stick
#define WINTVHVR950Q "Auvitek AU8522 QAM/8VSB Frontend"

TEST(DvbDevice, Initialize)
{
  DeviceVector devices = cDvbDevice::InitialiseDevices();
  ASSERT_FALSE(devices.empty());
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    EXPECT_EQ(0, device->Frontend());
    EXPECT_EQ(0, device->Adapter());

    EXPECT_STRNE("", device->DeviceName().c_str());
    EXPECT_STRNE("", device->DeviceType().c_str());

    if (device->DeviceName() == WINTVHVR950Q)
    {
      EXPECT_STREQ("ATSC", device->DeviceType().c_str());
    }
  }
}

TEST(DvbDevice, GetSubsystemId)
{
  DeviceVector devices = cDvbDevice::InitialiseDevices();
  ASSERT_FALSE(devices.empty());
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    if (device->DeviceName() == WINTVHVR950Q)
    {
      EXPECT_EQ(0, device->GetSubsystemId());
    }
    else
    {
      EXPECT_NE(0, device->GetSubsystemId()); // TODO
    }
  }
}
