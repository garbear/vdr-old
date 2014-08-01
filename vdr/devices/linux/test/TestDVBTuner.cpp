/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#include "devices/linux/DVBTuner.h"
#include "DVBDeviceNames.h"
#include "channels/ChannelManager.h"
#include "devices/linux/DVBDevice.h"
#include "devices/linux/subsystems/DVBChannelSubsystem.h"
#include "devices/linux/test/DVBDeviceNames.h"
#include "filesystem/SpecialProtocol.h"
#include "gtest/gtest.h"

#include <vector>

using namespace std;

#define CHANNELS_XML    "special://vdr/vdr/channels/test/channels.xml"

namespace VDR
{

TEST(DvbTuner, Open)
{
  DeviceVector devices = cDvbDevice::FindDevices();
  ASSERT_FALSE(devices.empty());
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    cDvbTuner& tuner = device->m_dvbTuner;

    EXPECT_FALSE(tuner.IsOpen());
    EXPECT_TRUE(tuner.Open());
    EXPECT_TRUE(tuner.IsOpen());
    tuner.Close();
    EXPECT_FALSE(tuner.IsOpen());
  }
}

TEST(DvbTuner, Getters)
{
  DeviceVector devices = cDvbDevice::FindDevices();
  ASSERT_FALSE(devices.empty());
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    cDvbTuner& tuner = device->m_dvbTuner;

    ASSERT_TRUE(tuner.Open());

    EXPECT_STRNE("", tuner.GetName().c_str());
    EXPECT_EQ(SYS_UNDEFINED, tuner.FrontendType()); // Haven't set channel yet

    if (cDvbDevices::IsATSC(tuner.GetName()))
    {
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_UNDEFINED));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DVBC_ANNEX_AC));
      EXPECT_TRUE(tuner.HasDeliverySystem(SYS_DVBC_ANNEX_B));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DVBT));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DSS));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DVBS));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DVBS2));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DVBH));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_ISDBT));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_ISDBS));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_ISDBC));
      EXPECT_TRUE(tuner.HasDeliverySystem(SYS_ATSC));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_ATSCMH));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DTMB));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_CMMB));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DAB));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DVBT2));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_TURBO));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DVBC_ANNEX_C));

      EXPECT_FALSE(tuner.HasCapability(FE_IS_STUPID));
      EXPECT_TRUE(tuner.HasCapability(FE_CAN_INVERSION_AUTO));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_1_2));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_2_3));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_3_4));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_4_5));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_5_6));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_6_7));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_7_8));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_8_9));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_FEC_AUTO));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_QPSK));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_QAM_16));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_QAM_32));
      EXPECT_TRUE(tuner.HasCapability(FE_CAN_QAM_64));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_QAM_128));
      EXPECT_TRUE(tuner.HasCapability(FE_CAN_QAM_256));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_QAM_AUTO));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_TRANSMISSION_MODE_AUTO));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_BANDWIDTH_AUTO));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_GUARD_INTERVAL_AUTO));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_HIERARCHY_AUTO));
      EXPECT_TRUE(tuner.HasCapability(FE_CAN_8VSB));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_16VSB));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_MULTISTREAM));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_TURBO_FEC));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_2G_MODULATION));
      EXPECT_FALSE(tuner.HasCapability(FE_NEEDS_BENDING));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_RECOVER));
      EXPECT_FALSE(tuner.HasCapability(FE_CAN_MUTE_TS));
    }
  }
}

TEST(DvbTuner, HasLock)
{
  // Load a channel
  cChannelManager channelManager;
  ASSERT_TRUE(channelManager.Load(CHANNELS_XML));
  ChannelVector channels = channelManager.GetCurrent();
  ASSERT_FALSE(channels.empty());
  ChannelPtr channel = channels[0];
  ASSERT_TRUE(channel.get());

  DeviceVector devices = cDvbDevice::FindDevices();
  ASSERT_FALSE(devices.empty());
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    cDvbTuner& tuner = device->m_dvbTuner;

    ASSERT_TRUE(tuner.Open());

    EXPECT_STRNE("", tuner.GetName().c_str());

    EXPECT_FALSE(tuner.HasLock());
    EXPECT_FALSE(tuner.IsTunedTo(*channel));
    EXPECT_TRUE(tuner.SwitchChannel(channel));
    EXPECT_TRUE(tuner.HasLock());
    EXPECT_TRUE(tuner.IsTunedTo(*channel));
  }
}

}
