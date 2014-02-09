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

#include "channels/ChannelManager.h"
#include "filesystem/SpecialProtocol.h"

#include "gtest/gtest.h"

#define CHANNELS_XML    "special://vdr/vdr/channels/test/channels.xml"
#define CHANNELS2_XML   "special://temp/channels.xml"

TEST(ChannelManager, ChannelManager)
{
  {
    cChannelManager channels;
    EXPECT_TRUE(channels.Load(CHANNELS_XML));
    EXPECT_TRUE(channels.Save(CHANNELS2_XML));
    // TODO: Compare CHANNELS_CONF and CHANNELS2_CONF
  }
}
