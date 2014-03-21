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

#include "sources/linux/DVBTransponderParams.h"
#include "gtest/gtest.h"

#include <string>

using namespace std;

namespace VDR
{

/*
 * Ensure the object is constructed with the expected default values. This will
 * ensure further tests on an object with default values.
 */
TEST(DvbTransponderParams, DefaultValues)
{
  cDvbTransponderParams params;
  EXPECT_EQ(POLARIZATION_HORIZONTAL, params.Polarization());
  EXPECT_EQ(INVERSION_AUTO,          params.Inversion());
  EXPECT_EQ(BANDWIDTH_8_MHZ,         params.Bandwidth());
  EXPECT_EQ(FEC_AUTO,                params.CoderateH());
  EXPECT_EQ(FEC_AUTO,                params.CoderateL());
  EXPECT_EQ(QPSK,                    params.Modulation());
  EXPECT_EQ(DVB_SYSTEM_1,            params.System());
  EXPECT_EQ(TRANSMISSION_MODE_AUTO,  params.Transmission());
  EXPECT_EQ(GUARD_INTERVAL_AUTO,     params.Guard());
  EXPECT_EQ(HIERARCHY_AUTO,          params.Hierarchy());
  EXPECT_EQ(ROLLOFF_AUTO,            params.RollOff());
  EXPECT_EQ(0,                       params.StreamId());
}

/*
TEST(DvbTransponderParams, Serialize)
{
  cDvbTransponderParams params;
  string serialized;

  serialized = params.Serialise('A');
  EXPECT_STREQ(serialized.c_str(), "I999M2");

  serialized = params.Serialise('C');
  EXPECT_STREQ(serialized.c_str(), "C999I999M2");

  serialized = params.Serialise('S');
  EXPECT_STREQ(serialized.c_str(), "C999I999M2S0");

  serialized = params.Serialise('T');
  EXPECT_STREQ(serialized.c_str(), "B8C999D999G999I999M2S0T999Y999");

  serialized = params.Serialise('X');
  EXPECT_STREQ(serialized.c_str(), "");
}
*/

TEST(DvbTransponderParams, Deserialize)
{
  {
    cDvbTransponderParams params;
    EXPECT_TRUE(params.Deserialise(""));
    EXPECT_EQ(0,                      params.Polarization());
    EXPECT_EQ(INVERSION_AUTO,         params.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        params.Bandwidth());
    EXPECT_EQ(FEC_AUTO,               params.CoderateH());
    EXPECT_EQ(FEC_AUTO,               params.CoderateL());
    EXPECT_EQ(QPSK,                   params.Modulation());
    EXPECT_EQ(DVB_SYSTEM_1,           params.System());
    EXPECT_EQ(TRANSMISSION_MODE_AUTO, params.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_AUTO,    params.Guard());
    EXPECT_EQ(HIERARCHY_AUTO,         params.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           params.RollOff());
    EXPECT_EQ(0,                      params.StreamId());
  }

  {
    cDvbTransponderParams params;
    EXPECT_TRUE(params.Deserialise("I1M10"));
    EXPECT_EQ(0,                      params.Polarization());
    EXPECT_EQ(INVERSION_ON,           params.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        params.Bandwidth());
    EXPECT_EQ(FEC_AUTO,               params.CoderateH());
    EXPECT_EQ(FEC_AUTO,               params.CoderateL());
    EXPECT_EQ(VSB_8,                  params.Modulation());
    EXPECT_EQ(DVB_SYSTEM_1,           params.System());
    EXPECT_EQ(TRANSMISSION_MODE_AUTO, params.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_AUTO,    params.Guard());
    EXPECT_EQ(HIERARCHY_AUTO,         params.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           params.RollOff());
    EXPECT_EQ(0,                      params.StreamId());
  }

  {
    cDvbTransponderParams params;
    EXPECT_TRUE(params.Deserialise("C45I0M2"));
    EXPECT_EQ(0,                      params.Polarization());
    EXPECT_EQ(INVERSION_OFF,          params.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        params.Bandwidth());
    EXPECT_EQ(FEC_4_5,                params.CoderateH());
    EXPECT_EQ(FEC_AUTO,               params.CoderateL());
    EXPECT_EQ(QPSK,                   params.Modulation());
    EXPECT_EQ(DVB_SYSTEM_1,           params.System());
    EXPECT_EQ(TRANSMISSION_MODE_AUTO, params.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_AUTO,    params.Guard());
    EXPECT_EQ(HIERARCHY_AUTO,         params.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           params.RollOff());
    EXPECT_EQ(0,                      params.StreamId());
  }

  {
    cDvbTransponderParams params;
    EXPECT_TRUE(params.Deserialise("C910I999M256S1"));
    EXPECT_EQ(0,                      params.Polarization());
    EXPECT_EQ(INVERSION_AUTO,         params.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        params.Bandwidth());
    EXPECT_EQ(FEC_9_10,               params.CoderateH());
    EXPECT_EQ(FEC_AUTO,               params.CoderateL());
    EXPECT_EQ(QAM_256,                params.Modulation());
    EXPECT_EQ(DVB_SYSTEM_2,           params.System());
    EXPECT_EQ(TRANSMISSION_MODE_AUTO, params.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_AUTO,    params.Guard());
    EXPECT_EQ(HIERARCHY_AUTO,         params.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           params.RollOff());
    EXPECT_EQ(0,                      params.StreamId());
  }

  {
    cDvbTransponderParams params;
    EXPECT_TRUE(params.Deserialise("B8C999D0G19128I999M2S0T16Y4"));
    EXPECT_EQ(0,                      params.Polarization());
    EXPECT_EQ(INVERSION_AUTO,         params.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        params.Bandwidth());
    EXPECT_EQ(FEC_AUTO,               params.CoderateH());
    EXPECT_EQ(FEC_NONE,               params.CoderateL());
    EXPECT_EQ(QPSK,                   params.Modulation());
    EXPECT_EQ(DVB_SYSTEM_1,           params.System());
    EXPECT_EQ(TRANSMISSION_MODE_16K,  params.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_19_128,  params.Guard());
    EXPECT_EQ(HIERARCHY_4,            params.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           params.RollOff());
    EXPECT_EQ(0,                      params.StreamId());
  }
}

}
