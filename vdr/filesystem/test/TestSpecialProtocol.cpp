/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "filesystem/SpecialProtocol.h"

#include "gtest/gtest.h"

#include <string>

using namespace std;

class TestSpecialProtocol : public testing::Test
{
protected:
  TestSpecialProtocol()
  {
    vdr = CSpecialProtocol::TranslatePath("special://vdr");
    home = CSpecialProtocol::TranslatePath("special://home");
    profile = CSpecialProtocol::TranslatePath("special://profile");
    temp = CSpecialProtocol::TranslatePath("special://temp");
    xbmchome = CSpecialProtocol::TranslatePath("special://xbmc-home");
    xbmctemp = CSpecialProtocol::TranslatePath("special://xbmc-temp");
  }

  ~TestSpecialProtocol()
  {
    CSpecialProtocol::SetVDRPath(vdr);
    CSpecialProtocol::SetHomePath(home);
    CSpecialProtocol::SetProfilePath(profile);
    CSpecialProtocol::SetTempPath(temp);
    CSpecialProtocol::SetXBMCHomePath(xbmchome);
    CSpecialProtocol::SetXBMCTempPath(xbmctemp);
  }

private:
  string vdr;
  string home;
  string profile;
  string temp;
  string xbmchome;
  string xbmctemp;
};

TEST_F(TestSpecialProtocol, SetVDRPath)
{
  {
    const char *path = "special://vdr";
    const char *ref = "/path/to/vdr/";
    CSpecialProtocol::SetVDRPath(ref);
    EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
  }
  {
    const char *path = "";
    const char *ref = "";
    CSpecialProtocol::SetVDRPath(ref);
    EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
  }
}

TEST_F(TestSpecialProtocol, SetHomePath)
{
  const char *path = "special://home";
  const char *ref = "/path/to/home/";

  CSpecialProtocol::SetHomePath(ref);
  EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
}

TEST_F(TestSpecialProtocol, SetProfilePath)
{
  const char *path = "special://profile";
  const char *ref = "/path/to/profile/";

  CSpecialProtocol::SetProfilePath(ref);
  EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
}

TEST_F(TestSpecialProtocol, SetTempPath)
{
  const char *path = "special://temp";
  const char *ref = "/path/to/temp/";

  CSpecialProtocol::SetTempPath(ref);
  EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
}

TEST_F(TestSpecialProtocol, SetXBMCHomePath)
{
  const char *path = "special://xbmc-home";
  const char *ref = "/path/to/xbmc-home/";

  CSpecialProtocol::SetXBMCHomePath(ref);
  EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
}

TEST_F(TestSpecialProtocol, SetXBMCTempPath)
{
  const char *path = "special://xbmc-temp";
  const char *ref = "/path/to/xbmc-temp/";

  CSpecialProtocol::SetXBMCTempPath(ref);
  EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
}

TEST_F(TestSpecialProtocol, ComparePath)
{
  const char *path_vdr = "special://vdr";
  const char *path_home = "special://home";
  const char *ref = "/path/to/vdr/";
  const char *ref2 = "/path/to/home/";

  CSpecialProtocol::SetVDRPath(ref);
  CSpecialProtocol::SetHomePath(ref);
  EXPECT_TRUE(CSpecialProtocol::ComparePath(path_vdr, path_home));

  CSpecialProtocol::SetHomePath(ref2);
  EXPECT_FALSE(CSpecialProtocol::ComparePath(path_vdr, path_home));
}

/*
TEST_F(TestSpecialProtocol, LogPaths)
{
}
*/

TEST_F(TestSpecialProtocol, TranslatePath)
{
  {
    const char *path = "special://vdr/";
    const char *ref = "/path/to/vdr/";
    CSpecialProtocol::SetVDRPath(ref);
    EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
  }
  {
    const char *path = "special://vdr/folder/";
    const char *target = "/path/to/vdr/";
    const char *ref = "/path/to/vdr/folder/";
    CSpecialProtocol::SetVDRPath(target);
    EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
  }
  {
    const char *path = "special://vdr2";
    const char *ref = "";
    CSpecialProtocol::SetVDRPath(ref);
    EXPECT_STREQ(ref, CSpecialProtocol::TranslatePath(path).c_str());
  }
  // TODO: Set ref to a different special path
}

/* TODO
TEST_F(TestSpecialProtocol, TranslatePathConvertCase)
{
}
*/
