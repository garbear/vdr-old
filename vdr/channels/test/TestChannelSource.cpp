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

#include "channels/ChannelSource.h"

#include "gtest/gtest.h"

namespace VDR
{

TEST(ChannelSource, operator_equals)
{
  cChannelSource sourceNone;
  cChannelSource sourceNone2(SOURCE_TYPE_NONE);
  cChannelSource sourceAtsc(SOURCE_TYPE_ATSC);
  cChannelSource sourceSatellite(SOURCE_TYPE_SATELLITE);
  cChannelSource sourceSatellite110W(SOURCE_TYPE_SATELLITE, 110);
  cChannelSource sourceSatellite110E(SOURCE_TYPE_SATELLITE, 110, DIRECTION_EAST);
  cChannelSource sourceSatellite1105E(SOURCE_TYPE_SATELLITE, 110.5, DIRECTION_EAST);

  EXPECT_TRUE(sourceNone == SOURCE_TYPE_NONE);
  EXPECT_FALSE(sourceNone != SOURCE_TYPE_NONE);
  EXPECT_TRUE(sourceNone2 == SOURCE_TYPE_NONE);
  EXPECT_FALSE(sourceNone2 == SOURCE_TYPE_ATSC);
  EXPECT_TRUE(sourceAtsc == SOURCE_TYPE_ATSC);
  EXPECT_FALSE(sourceAtsc != SOURCE_TYPE_ATSC);
  EXPECT_TRUE(sourceAtsc != SOURCE_TYPE_NONE);
  EXPECT_TRUE(sourceAtsc != SOURCE_TYPE_SATELLITE);
  EXPECT_TRUE(sourceSatellite == SOURCE_TYPE_SATELLITE);
  EXPECT_TRUE(sourceSatellite110W == SOURCE_TYPE_SATELLITE);
  EXPECT_TRUE(sourceSatellite110E == SOURCE_TYPE_SATELLITE);
  EXPECT_FALSE(sourceSatellite != SOURCE_TYPE_SATELLITE);
  EXPECT_FALSE(sourceSatellite110W != SOURCE_TYPE_SATELLITE);
  EXPECT_FALSE(sourceSatellite110E != SOURCE_TYPE_SATELLITE);

  EXPECT_TRUE(sourceNone == sourceNone2);
  EXPECT_FALSE(sourceNone != sourceNone2);
  EXPECT_FALSE(sourceAtsc == sourceNone);
  EXPECT_TRUE(sourceAtsc == sourceAtsc);
  EXPECT_FALSE(sourceAtsc != sourceAtsc);
  EXPECT_FALSE(sourceAtsc == sourceSatellite);

  EXPECT_TRUE(sourceSatellite == cChannelSource(SOURCE_TYPE_SATELLITE));
  EXPECT_FALSE(sourceSatellite110W == cChannelSource(SOURCE_TYPE_SATELLITE));
  EXPECT_FALSE(sourceSatellite110E == cChannelSource(SOURCE_TYPE_SATELLITE));

  EXPECT_FALSE(sourceSatellite == cChannelSource(SOURCE_TYPE_SATELLITE, 110));
  EXPECT_TRUE(sourceSatellite110W == cChannelSource(SOURCE_TYPE_SATELLITE, 110));
  EXPECT_FALSE(sourceSatellite110E == cChannelSource(SOURCE_TYPE_SATELLITE, 110));

  EXPECT_FALSE(sourceSatellite == cChannelSource(SOURCE_TYPE_SATELLITE, 110, DIRECTION_EAST));
  EXPECT_FALSE(sourceSatellite110W == cChannelSource(SOURCE_TYPE_SATELLITE, 110, DIRECTION_EAST));
  EXPECT_TRUE(sourceSatellite110E == cChannelSource(SOURCE_TYPE_SATELLITE, 110, DIRECTION_EAST));

  EXPECT_TRUE(sourceSatellite1105E == cChannelSource(SOURCE_TYPE_SATELLITE, 110.5, DIRECTION_EAST));
  EXPECT_FALSE(sourceSatellite1105E == cChannelSource(SOURCE_TYPE_SATELLITE, 110, DIRECTION_EAST));
}

TEST(ChannelSource, ToString)
{
  cChannelSource sourceNone;
  cChannelSource sourceNone2(SOURCE_TYPE_NONE);
  cChannelSource sourceAtsc(SOURCE_TYPE_ATSC);
  cChannelSource sourceSatellite(SOURCE_TYPE_SATELLITE);
  cChannelSource sourceSatellite110W(SOURCE_TYPE_SATELLITE, 110);
  cChannelSource sourceSatellite110E(SOURCE_TYPE_SATELLITE, 110, DIRECTION_EAST);
  cChannelSource sourceSatelliteFloat(SOURCE_TYPE_SATELLITE, 110.5f);
  cChannelSource sourceSatelliteNegative(SOURCE_TYPE_SATELLITE, -110.5f);

  EXPECT_STREQ("", sourceNone.ToString().c_str());
  EXPECT_STREQ("A", sourceAtsc.ToString().c_str());
  EXPECT_STREQ("S0.0W", sourceSatellite.ToString().c_str());
  EXPECT_STREQ("S110.0W", sourceSatellite110W.ToString().c_str());
  EXPECT_STREQ("S110.0E", sourceSatellite110E.ToString().c_str());
  EXPECT_STREQ("S110.5W", sourceSatelliteFloat.ToString().c_str());
  EXPECT_STREQ("S110.5E", sourceSatelliteNegative.ToString().c_str());
}

}
