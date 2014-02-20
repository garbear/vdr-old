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

#include "scan/CNICodes.h"

#include <gtest/gtest.h>

TEST(CniCodes, GetCniCode)
{
  {
    CniCodes::cCniCode cniCode = CniCodes::GetCniCode(0);
    EXPECT_STREQ("CNNI", cniCode.network);
  }

  // Test foreign characters ('MERICA!!!!)
  {
    CniCodes::cCniCode cniCode = CniCodes::GetCniCode(438);
    EXPECT_STREQ("PEOPLE TV – RETE 7", cniCode.network);
  }
  {
    CniCodes::cCniCode cniCode = CniCodes::GetCniCode(537);
    EXPECT_STREQ("RTL Télé Lëtzebuerg", cniCode.network);
  }
}

TEST(CniCodes, GetCniCodeCount)
{
  EXPECT_EQ(1191, CniCodes::GetCniCodeCount());
}
