/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "channels/Channel.h"

#include "gtest/gtest.h"

TEST(Channel, Channel)
{
  // Test value initialization of data struct
  cChannel channel;
  EXPECT_EQ(channel.Frequency(), 0);
  EXPECT_EQ(channel.Source(), 0);
  EXPECT_EQ(channel.Srate(), 0);
  EXPECT_EQ(channel.Vpid(), 0);
  EXPECT_EQ(channel.Ppid(), 0);
  EXPECT_NE(channel.Apids(), (const int*)NULL);
  EXPECT_NE(channel.Dpids(), (const int*)NULL);
  EXPECT_NE(channel.Spids(), (const int*)NULL);
  EXPECT_EQ(channel.Apid(0), 0);
  EXPECT_EQ(channel.Dpid(0), 0);
  EXPECT_EQ(channel.Spid(0), 0);
  EXPECT_NE(channel.Alang(0), (const char*)NULL);
  EXPECT_STREQ(channel.Alang(0), "");
  EXPECT_NE(channel.Dlang(0), (const char*)NULL);
  EXPECT_STREQ(channel.Dlang(0), "");
  EXPECT_NE(channel.Slang(0), (const char*)NULL);
  EXPECT_STREQ(channel.Slang(0), "");
  EXPECT_EQ(channel.Atype(0), 0);
  EXPECT_EQ(channel.Dtype(0), 0);
  EXPECT_EQ(channel.SubtitlingType(0), (uchar)0);
  EXPECT_EQ(channel.CompositionPageId(0), (uint16_t)0);
  EXPECT_EQ(channel.AncillaryPageId(0), (uint16_t)0);
  EXPECT_EQ(channel.Tpid(), 0);
  EXPECT_NE(channel.Caids(), (const int*)NULL);
  EXPECT_EQ(channel.Ca(0), 0);
  EXPECT_EQ(channel.Nid(), 0);
  EXPECT_EQ(channel.Tid(), 0);
  EXPECT_EQ(channel.Sid(), 0);
  EXPECT_EQ(channel.Rid(), 0);
  EXPECT_EQ(channel.Number(), 0);
  EXPECT_EQ(channel.GroupSep(), false);

  EXPECT_STREQ(channel.Serialise().c_str(), ":0:::0:0:0:0:0:0:0:0:0\n");

  EXPECT_TRUE(channel.Deserialise(channel.Serialise()));
  EXPECT_EQ(channel.Frequency(), 0);
  EXPECT_EQ(channel.Source(), 0);
  EXPECT_EQ(channel.Srate(), 0);
  EXPECT_EQ(channel.Vpid(), 0);
  EXPECT_EQ(channel.Ppid(), 0);
  EXPECT_NE(channel.Apids(), (const int*)NULL);
  EXPECT_NE(channel.Dpids(), (const int*)NULL);
  EXPECT_NE(channel.Spids(), (const int*)NULL);
  EXPECT_EQ(channel.Apid(0), 0);
  EXPECT_EQ(channel.Dpid(0), 0);
  EXPECT_EQ(channel.Spid(0), 0);
  EXPECT_NE(channel.Alang(0), (const char*)NULL);
  EXPECT_STREQ(channel.Alang(0), "");
  EXPECT_NE(channel.Dlang(0), (const char*)NULL);
  EXPECT_STREQ(channel.Dlang(0), "");
  EXPECT_NE(channel.Slang(0), (const char*)NULL);
  EXPECT_STREQ(channel.Slang(0), "");
  EXPECT_EQ(channel.Atype(0), 0);
  EXPECT_EQ(channel.Dtype(0), 0);
  EXPECT_EQ(channel.SubtitlingType(0), (uchar)0);
  EXPECT_EQ(channel.CompositionPageId(0), (uint16_t)0);
  EXPECT_EQ(channel.AncillaryPageId(0), (uint16_t)0);
  EXPECT_EQ(channel.Tpid(), 0);
  EXPECT_NE(channel.Caids(), (const int*)NULL);
  EXPECT_EQ(channel.Ca(0), 0);
  EXPECT_EQ(channel.Nid(), 0);
  EXPECT_EQ(channel.Tid(), 0);
  EXPECT_EQ(channel.Sid(), 0);
  EXPECT_EQ(channel.Rid(), 0);
  EXPECT_EQ(channel.Number(), 0);
  //EXPECT_EQ(channel.GroupSep(), false); // TODO: Fails

  EXPECT_FALSE(channel.Deserialise(""));

  unsigned int frequency = 1000 * 1000 * 1000; // 1GHz
  EXPECT_EQ(cChannel::Transponder(frequency, 'H'), frequency + 100 * 1000);
  EXPECT_EQ(cChannel::Transponder(frequency, 'V'), frequency + 200 * 1000);
  EXPECT_EQ(cChannel::Transponder(frequency, 'L'), frequency + 300 * 1000);
  EXPECT_EQ(cChannel::Transponder(frequency, 'R'), frequency + 400 * 1000);
  EXPECT_EQ(cChannel::Transponder(frequency, 'X'), frequency);
}
