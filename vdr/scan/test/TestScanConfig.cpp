/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "scan/ScanConfig.h"

#include <gtest/gtest.h>

TEST(ScanConfig, GetCniCode)
{
  EXPECT_EQ(6900000, cScanConfig::TranslateSymbolRate(eSR_6900000));
  EXPECT_EQ(6875000, cScanConfig::TranslateSymbolRate(eSR_6875000));
  EXPECT_EQ(6111000, cScanConfig::TranslateSymbolRate(eSR_6111000));
  EXPECT_EQ(6250000, cScanConfig::TranslateSymbolRate(eSR_6250000));
  EXPECT_EQ(6790000, cScanConfig::TranslateSymbolRate(eSR_6790000));
  EXPECT_EQ(6811000, cScanConfig::TranslateSymbolRate(eSR_6811000));
  EXPECT_EQ(5900000, cScanConfig::TranslateSymbolRate(eSR_5900000));
  EXPECT_EQ(5000000, cScanConfig::TranslateSymbolRate(eSR_5000000));
  EXPECT_EQ(3450000, cScanConfig::TranslateSymbolRate(eSR_3450000));
  EXPECT_EQ(4000000, cScanConfig::TranslateSymbolRate(eSR_4000000));
  EXPECT_EQ(6950000, cScanConfig::TranslateSymbolRate(eSR_6950000));
  EXPECT_EQ(7000000, cScanConfig::TranslateSymbolRate(eSR_7000000));
  EXPECT_EQ(6952000, cScanConfig::TranslateSymbolRate(eSR_6952000));
  EXPECT_EQ(5156000, cScanConfig::TranslateSymbolRate(eSR_5156000));
  EXPECT_EQ(5483000, cScanConfig::TranslateSymbolRate(eSR_5483000));
}
