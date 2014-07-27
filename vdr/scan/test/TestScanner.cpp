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

#include "scan/Scanner.h"
#include "scan/ScanConfig.h"
#include "channels/ChannelManager.h"
#include "devices/linux/DVBDevice.h"
#include "devices/linux/test/DVBDeviceNames.h"
#include "filesystem/File.h"

#include <gtest/gtest.h>
#include <unistd.h> // for sleep()

#define CHANNELS_XML  "special://vdr/vdr/scan/test/channel-scan-results.xml"

namespace VDR
{

TEST(Scanner, Scan)
{
  CFile::Delete(CHANNELS_XML);

  // Initialize cChannelManager with the proper path
  cChannelManager::Get().Load(CHANNELS_XML);
  cChannelManager::Get().Clear();
  EXPECT_EQ(0, cChannelManager::Get().ChannelCount());

  DeviceVector devices = cDvbDevice::FindDevices();
  ASSERT_FALSE(devices.empty());
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    ASSERT_TRUE(device->Initialise());

    if (cDvbDevices::IsATSC(device->DeviceName()))
    {
      cScanConfig config;
      config.dvbType = TRANSPONDER_ATSC;
      config.atscModulation = ATSC_MODULATION_VSB_8;
      config.countryIndex = COUNTRY::US;
      config.device = *it;

      cScanner scanner;

      scanner.Start(config);
      while (scanner.IsRunning())
        sleep(5);

      EXPECT_NE(0, cChannelManager::Get().ChannelCount());

      cChannelManager::Get().Clear();

      break;
    }
  }
}

}
