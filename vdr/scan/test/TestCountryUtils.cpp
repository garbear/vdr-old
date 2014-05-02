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

#include "scan/CountryUtils.h"

#include <gtest/gtest.h>

namespace VDR
{

TEST(CountryUtils, GetFrontendType)
{
  eFrontendType frontendType = eFT_ATSC;
  EXPECT_TRUE(CountryUtils::GetFrontendType(COUNTRY::DE, frontendType) && frontendType == eFT_DVB);
  EXPECT_TRUE(CountryUtils::GetFrontendType(COUNTRY::CA, frontendType) && frontendType == eFT_ATSC);
}

TEST(CountryUtils, GetChannelList)
{
  // Test independence of atscModulation for European countries
  {
    eChannelList channelList1 = ATSC_VSB, channelList2 = ATSC_VSB;
    EXPECT_TRUE(CountryUtils::GetChannelList(COUNTRY::FR, FE_QAM, VSB_8, channelList1) && channelList1 == DVBC_FR);
    EXPECT_TRUE(CountryUtils::GetChannelList(COUNTRY::FR, FE_QAM, QAM_256, channelList2) && channelList2 == DVBC_FR);
  }

  // Test independence of dvbType for ATSC countries
  {
    eChannelList channelList1 = ATSC_VSB, channelList2 = ATSC_VSB;
    EXPECT_TRUE(CountryUtils::GetChannelList(COUNTRY::TW, FE_QAM, QAM_256, channelList1) && channelList1 == ATSC_QAM);
    EXPECT_TRUE(CountryUtils::GetChannelList(COUNTRY::TW, FE_QPSK, QAM_256, channelList2) && channelList2 == ATSC_QAM);
  }
}

TEST(CountryUtils, GetBaseOffset)
{
  int offset = 0;
  EXPECT_TRUE(CountryUtils::GetBaseOffset(95, ATSC_QAM, offset) && offset == -477000000);
}

}
