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

#include "DVBDeviceNames.h"
#include "devices/linux/DVBDevice.h"
#include "devices/linux/DVBTuner.h"
#include "devices/linux/subsystems/DVBChannelSubsystem.h"
#include "gtest/gtest.h"

#include <vector>

using namespace std;

TEST(DvbTuner, cDvbTuner)
{
  DeviceVector devices = cDvbDevice::FindDevices();
  ASSERT_FALSE(devices.empty());
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    ASSERT_TRUE(device->Initialise());

    cDvbTuner& tuner = device->m_dvbTuner;
    ASSERT_TRUE(tuner.IsOpen());
    EXPECT_EQ(device->Adapter(), tuner.Adapter());
    EXPECT_EQ(device->Frontend(), tuner.Frontend());
    EXPECT_STRNE("", tuner.Name().c_str());

    if (tuner.Name() == WINTVHVR950Q)
    {
      EXPECT_EQ(SYS_UNDEFINED, tuner.FrontendType());

      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_UNDEFINED));
      EXPECT_FALSE(tuner.HasDeliverySystem(SYS_DVBC_ANNEX_A));
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

      EXPECT_EQ(0x0509, tuner.GetDvbApiVersion());
    }
    else
    {
      EXPECT_NE(SYS_UNDEFINED, tuner.FrontendType());
    }
  }
}
