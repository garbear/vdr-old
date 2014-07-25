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

#include "dvb/filters/PAT.h"
#include "devices/linux/DVBDevice.h"
#include "devices/linux/test/DVBDeviceNames.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "scan/ScanTask.h"
#include "scan/ScanConfig.h"
#include "transponders/Transponder.h"

#include <gtest/gtest.h>

namespace VDR
{

TEST(PAT, GetChannels)
{
  DeviceVector devices = cDvbDevice::FindDevices();
  ASSERT_FALSE(devices.empty());
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    ASSERT_TRUE(device->Initialise());

    ChannelPtr channel;

    if (cDvbDevices::IsATSC(device->DeviceName()))
    {
      cFrontendCapabilities caps(device->m_dvbTuner.GetCapabilities());

      cTransponder transponder(TRANSPONDER_ATSC);
      transponder.SetFrequencyMHz(177);
      transponder.SetInversion(caps.caps_inversion);
      transponder.SetModulation(VSB_8);

      channel = ChannelPtr(new cChannel);
      channel->SetTransponder(transponder);
    }

    if (channel)
    {
      EXPECT_TRUE(device->Channel()->SwitchChannel(channel));
      EXPECT_TRUE(device->Channel()->HasLock());

      cPat pat(device);
      ChannelVector channels = pat.GetChannels();

      EXPECT_NE(0, channels.size());
    }
  }
}

}
