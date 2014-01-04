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

DeviceVector g_devices;

cDvbDevice *GetDevice()
{
  g_devices = cDvbDevice::InitialiseDevices();
  if (!g_devices.empty())
  {
    cDvbDevice* firstDevice = dynamic_cast<cDvbDevice*>(g_devices[0].get());
    return firstDevice;
  }
  return NULL;
}

TEST(DVBDevice, Exists)
{
  EXPECT_TRUE(cDvbDevice::Exists(0, 0));
}

TEST(DVBDevice, Initialize)
{
  cDvbDevice *device = GetDevice();
  ASSERT_TRUE(device);

  EXPECT_EQ(0, device->Frontend());
  EXPECT_EQ(0, device->Adapter());

  EXPECT_GT(device->m_deliverySystems.size(), 0);
  EXPECT_GT(device->m_numModulations, 0);

  g_devices.clear();
}

TEST(DVBDevice, GetSubsystemId)
{
  cDvbDevice *device = GetDevice();
  ASSERT_TRUE(device);

  EXPECT_EQ(0, device->GetSubsystemId());
  g_devices.clear();
}

TEST(DVBDevice, GetDvbApiVersion)
{
  cDvbDevice *device = GetDevice();
  ASSERT_TRUE(device);

  EXPECT_NE(0, device->GetDvbApiVersion());
  g_devices.clear();
}
