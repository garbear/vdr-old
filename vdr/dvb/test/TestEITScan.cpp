/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "dvb/EITScan.h"
#include "devices/DeviceManager.h"

#include <gtest/gtest.h>
#include <unistd.h> // for sleep()

using namespace std;

#define DEVICEREADYTIMEOUT 30 // From VDRDaemon.cpp

namespace VDR
{

TEST(EITScan, Process)
{
  EXPECT_TRUE(cChannelManager::Get().Load());
  EXPECT_NE(0, cChannelManager::Get().ChannelCount());
  dsyslog("Loaded %u channels", cChannelManager::Get().ChannelCount());

  EXPECT_NE(0, cDeviceManager::Get().Initialise());

  EXPECT_TRUE(cDeviceManager::Get().WaitForAllDevicesReady(DEVICEREADYTIMEOUT));

  EXPECT_TRUE(cEITScanner::Get().CreateThread());

  while (cEITScanner::Get().IsRunning())
    sleep(1);
}

}
