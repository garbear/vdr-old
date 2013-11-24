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

#include "sources/linux/DVBSourceParams.h"

#include "gtest/gtest.h"

#include <string>

using namespace std;

/*
 * Ensure the object is constructed with the expected default values. This will
 * ensure further tests on an object with default values.
 */
TEST(DvbTransponderParams, DefaultValues)
{
  cDvbTransponderParams params;
  EXPECT_EQ(params.Polarization(), 0);
  EXPECT_EQ(params.Inversion(),    INVERSION_AUTO);
  EXPECT_EQ(params.Bandwidth(),    8000000);
  EXPECT_EQ(params.CoderateH(),    FEC_AUTO);
  EXPECT_EQ(params.CoderateL(),    FEC_AUTO);
  EXPECT_EQ(params.Modulation(),   QPSK);
  EXPECT_EQ(params.System(),       DVB_SYSTEM_1);
  EXPECT_EQ(params.Transmission(), TRANSMISSION_MODE_AUTO);
  EXPECT_EQ(params.Guard(),        GUARD_INTERVAL_AUTO);
  EXPECT_EQ(params.Hierarchy(),    HIERARCHY_AUTO);
  EXPECT_EQ(params.RollOff(),      ROLLOFF_AUTO);
  EXPECT_EQ(params.StreamId(),     0);
}

/*
 * Test serialization of default parameters.
 */
TEST(DvbTransponderParams, Serialize)
{
  cDvbTransponderParams params;
  string serialized;

  serialized = params.Serialize('A');
  EXPECT_STREQ(serialized.c_str(), "I999M2");

  serialized = params.Serialize('C');
  EXPECT_STREQ(serialized.c_str(), "C999I999M2");

  serialized = params.Serialize('S');
  EXPECT_STREQ(serialized.c_str(), "C999I999M2S0");

  serialized = params.Serialize('T');
  EXPECT_STREQ(serialized.c_str(), "B8C999D999G999I999M2S0T999Y999");

  serialized = params.Serialize('X');
  EXPECT_STREQ(serialized.c_str(), "");
}

/*
 * Test serialization of default parameters.
 */
TEST(DvbTransponderParams, Deserialize)
{
  cDvbTransponderParams params;

  EXPECT_TRUE(params.Deserialize(""));
  EXPECT_TRUE(params.Deserialize("I999M2"));
  EXPECT_TRUE(params.Deserialize("C999I999M2"));
  EXPECT_TRUE(params.Deserialize("C999I999M2S0"));
  EXPECT_TRUE(params.Deserialize("B8C999D999G999I999M2S0T999Y999"));

  // TODO: More tests
}
