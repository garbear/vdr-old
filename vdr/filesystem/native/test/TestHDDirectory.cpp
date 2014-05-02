/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#include "filesystem/native/HDDirectory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"

#include "gtest/gtest.h"

#include <string>

using namespace std;

namespace VDR
{

TEST(HDDirectory, GetDirectory)
{
  // Populate a test directory
  const string basepath = CSpecialProtocol::TranslatePath("special://temp/HDDirectory.GetDirectory");
  const string testfile = CSpecialProtocol::TranslatePath("special://temp/HDDirectory.GetDirectory/file.txt");
  const string testdir = CSpecialProtocol::TranslatePath("special://temp/HDDirectory.GetDirectory/test");

  {
    CHDDirectory dir;
    EXPECT_TRUE(dir.Create(basepath));
    EXPECT_TRUE(dir.Create(testdir));
    CFile file;
    EXPECT_TRUE(file.OpenForWrite(testfile));
  }

  {
    CHDDirectory dir;
    DirectoryListing items;
    EXPECT_TRUE(dir.GetDirectory(basepath, items));
    EXPECT_EQ(4, items.size());

    set<string> names;
    for (DirectoryListing::const_iterator it = items.begin(); it != items.end(); ++it)
      names.insert(it->Name());

    EXPECT_TRUE(names.find("test") != names.end());
    EXPECT_TRUE(names.find("file.txt") != names.end());
    EXPECT_TRUE(names.find(".") != names.end());
    EXPECT_TRUE(names.find("..") != names.end());
  }

  {
    CHDDirectory dir;
    EXPECT_TRUE(CFile::Delete(testfile));
    EXPECT_TRUE(dir.Remove(testdir));
    EXPECT_TRUE(dir.Remove(basepath));
  }
}

/* TODO
TEST(HDDirectory, Create)
{
}

TEST(HDDirectory, Exists)
{
}

TEST(HDDirectory, Remove)
{
}

TEST(HDDirectory, Rename)
{
}
*/

}
