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

#include "channels/ChannelID.h"

#include "gtest/gtest.h"

namespace VDR
{

TEST(ChannelID, ChannelID)
{
  EXPECT_EQ(tChannelID(), tChannelID::InvalidID);
  EXPECT_STREQ(tChannelID::InvalidID.Serialize().c_str(), "-0-0-0");

  int source = ('S' << 24) | 1073;
  tChannelID channelId(source, 1, 2, 3);
  EXPECT_EQ(channelId.Source(), source);
  EXPECT_EQ(channelId.Nid(), 1);
  EXPECT_EQ(channelId.Tid(), 2);
  EXPECT_EQ(channelId.Sid(), 3);

  tChannelID channelId2 = channelId;
  EXPECT_EQ(channelId2.Source(), source);
  EXPECT_EQ(channelId2.Nid(), 1);
  EXPECT_EQ(channelId2.Tid(), 2);
  EXPECT_EQ(channelId2.Sid(), 3);
  EXPECT_EQ(channelId2, channelId);
  EXPECT_NE(channelId2, tChannelID::InvalidID);

  EXPECT_STREQ(channelId.Serialize().c_str(), "S107.3W-1-2-3");
  tChannelID channelId3 = tChannelID::Deserialize(channelId.Serialize());
  //EXPECT_EQ(channelId3, channelId); // TODO
  //EXPECT_EQ(channelId3.Source(), source); // TODO
  EXPECT_EQ(channelId3.Nid(), 1);
  EXPECT_EQ(channelId3.Tid(), 2);
  EXPECT_EQ(channelId3.Sid(), 3);

  EXPECT_EQ(tChannelID::Deserialize(""), tChannelID::InvalidID);
  EXPECT_EQ(tChannelID::Deserialize("S107.3W-1-2-"), tChannelID::InvalidID);

  // TODO: More ser/des tests

  // TODO: Test ClrPolarization() once its functionality is understood
}

}
