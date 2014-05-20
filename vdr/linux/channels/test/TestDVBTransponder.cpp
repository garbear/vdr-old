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

#include "linux/channels/DVBTransponder.h"
#include "gtest/gtest.h"

#include <string>

using namespace std;

namespace VDR
{

/*
 * Ensure the object is constructed with the expected default values. This will
 * ensure further tests on an object with default values.
 */
TEST(DvbTransponder, DefaultValues)
{
  cDvbTransponder transponder;
  EXPECT_EQ(POLARIZATION_HORIZONTAL, transponder.Polarization());
  EXPECT_EQ(INVERSION_AUTO,          transponder.Inversion());
  EXPECT_EQ(BANDWIDTH_8_MHZ,         transponder.Bandwidth());
  EXPECT_EQ(FEC_AUTO,                transponder.CoderateH());
  EXPECT_EQ(FEC_AUTO,                transponder.CoderateL());
  EXPECT_EQ(QPSK,                    transponder.Modulation());
  EXPECT_EQ(DVB_SYSTEM_1,            transponder.System());
  EXPECT_EQ(TRANSMISSION_MODE_AUTO,  transponder.Transmission());
  EXPECT_EQ(GUARD_INTERVAL_AUTO,     transponder.Guard());
  EXPECT_EQ(HIERARCHY_AUTO,          transponder.Hierarchy());
  EXPECT_EQ(ROLLOFF_AUTO,            transponder.RollOff());
  EXPECT_EQ(0,                       transponder.StreamId());
}

/*
TEST(DvbTransponder, Serialize)
{
  cDvbTransponder transponder;
  string serialized;

  serialized = transponder.Serialise('A');
  EXPECT_STREQ(serialized.c_str(), "I999M2");

  serialized = transponder.Serialise('C');
  EXPECT_STREQ(serialized.c_str(), "C999I999M2");

  serialized = transponder.Serialise('S');
  EXPECT_STREQ(serialized.c_str(), "C999I999M2S0");

  serialized = transponder.Serialise('T');
  EXPECT_STREQ(serialized.c_str(), "B8C999D999G999I999M2S0T999Y999");

  serialized = transponder.Serialise('X');
  EXPECT_STREQ(serialized.c_str(), "");
}
*/

TEST(DvbTransponder, Deserialize)
{
  {
    cDvbTransponder transponder;
    EXPECT_TRUE(transponder.Deserialise(""));
    EXPECT_EQ(0,                      transponder.Polarization());
    EXPECT_EQ(INVERSION_AUTO,         transponder.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        transponder.Bandwidth());
    EXPECT_EQ(FEC_AUTO,               transponder.CoderateH());
    EXPECT_EQ(FEC_AUTO,               transponder.CoderateL());
    EXPECT_EQ(QPSK,                   transponder.Modulation());
    EXPECT_EQ(DVB_SYSTEM_1,           transponder.System());
    EXPECT_EQ(TRANSMISSION_MODE_AUTO, transponder.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_AUTO,    transponder.Guard());
    EXPECT_EQ(HIERARCHY_AUTO,         transponder.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           transponder.RollOff());
    EXPECT_EQ(0,                      transponder.StreamId());
  }

  {
    cDvbTransponder transponder;
    EXPECT_TRUE(transponder.Deserialise("I1M10"));
    EXPECT_EQ(0,                      transponder.Polarization());
    EXPECT_EQ(INVERSION_ON,           transponder.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        transponder.Bandwidth());
    EXPECT_EQ(FEC_AUTO,               transponder.CoderateH());
    EXPECT_EQ(FEC_AUTO,               transponder.CoderateL());
    EXPECT_EQ(VSB_8,                  transponder.Modulation());
    EXPECT_EQ(DVB_SYSTEM_1,           transponder.System());
    EXPECT_EQ(TRANSMISSION_MODE_AUTO, transponder.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_AUTO,    transponder.Guard());
    EXPECT_EQ(HIERARCHY_AUTO,         transponder.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           transponder.RollOff());
    EXPECT_EQ(0,                      transponder.StreamId());
  }

  {
    cDvbTransponder transponder;
    EXPECT_TRUE(transponder.Deserialise("C45I0M2"));
    EXPECT_EQ(0,                      transponder.Polarization());
    EXPECT_EQ(INVERSION_OFF,          transponder.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        transponder.Bandwidth());
    EXPECT_EQ(FEC_4_5,                transponder.CoderateH());
    EXPECT_EQ(FEC_AUTO,               transponder.CoderateL());
    EXPECT_EQ(QPSK,                   transponder.Modulation());
    EXPECT_EQ(DVB_SYSTEM_1,           transponder.System());
    EXPECT_EQ(TRANSMISSION_MODE_AUTO, transponder.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_AUTO,    transponder.Guard());
    EXPECT_EQ(HIERARCHY_AUTO,         transponder.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           transponder.RollOff());
    EXPECT_EQ(0,                      transponder.StreamId());
  }

  {
    cDvbTransponder transponder;
    EXPECT_TRUE(transponder.Deserialise("C910I999M256S1"));
    EXPECT_EQ(0,                      transponder.Polarization());
    EXPECT_EQ(INVERSION_AUTO,         transponder.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        transponder.Bandwidth());
    EXPECT_EQ(FEC_9_10,               transponder.CoderateH());
    EXPECT_EQ(FEC_AUTO,               transponder.CoderateL());
    EXPECT_EQ(QAM_256,                transponder.Modulation());
    EXPECT_EQ(DVB_SYSTEM_2,           transponder.System());
    EXPECT_EQ(TRANSMISSION_MODE_AUTO, transponder.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_AUTO,    transponder.Guard());
    EXPECT_EQ(HIERARCHY_AUTO,         transponder.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           transponder.RollOff());
    EXPECT_EQ(0,                      transponder.StreamId());
  }

  {
    cDvbTransponder transponder;
    EXPECT_TRUE(transponder.Deserialise("B8C999D0G19128I999M2S0T16Y4"));
    EXPECT_EQ(0,                      transponder.Polarization());
    EXPECT_EQ(INVERSION_AUTO,         transponder.Inversion());
    EXPECT_EQ(BANDWIDTH_8_MHZ,        transponder.Bandwidth());
    EXPECT_EQ(FEC_AUTO,               transponder.CoderateH());
    EXPECT_EQ(FEC_NONE,               transponder.CoderateL());
    EXPECT_EQ(QPSK,                   transponder.Modulation());
    EXPECT_EQ(DVB_SYSTEM_1,           transponder.System());
    EXPECT_EQ(TRANSMISSION_MODE_16K,  transponder.Transmission());
    EXPECT_EQ(GUARD_INTERVAL_19_128,  transponder.Guard());
    EXPECT_EQ(HIERARCHY_4,            transponder.Hierarchy());
    EXPECT_EQ(ROLLOFF_AUTO,           transponder.RollOff());
    EXPECT_EQ(0,                      transponder.StreamId());
  }
}

}
