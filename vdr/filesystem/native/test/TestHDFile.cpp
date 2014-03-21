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

#include "filesystem/native/HDFile.h"
#include "filesystem/SpecialProtocol.h"

#include "gtest/gtest.h"

#include <string>

using namespace std;

namespace VDR
{

class CHDFileTest : public CHDFile
{
public:
  string GetLocal(const CURL &url) { return CHDFile::GetLocal(url); }
};

TEST(HDFile, Open)
{
  const string path = CSpecialProtocol::TranslatePath("special://temp/HDFile.Open.txt");

  {
    CHDFile file;
    EXPECT_FALSE(file.Exists(path));
    EXPECT_TRUE(file.OpenForWrite(path));
    file.Close();
  }
  {
    CHDFile file;
    EXPECT_TRUE(file.Exists(path));
    EXPECT_TRUE(file.Open(path));
    file.Close();
  }
  {
    CHDFile file;
    EXPECT_TRUE(file.Delete(path));
  }
}

TEST(HDFile, OpenForWrite)
{
  const string path = CSpecialProtocol::TranslatePath("special://temp/HDFile.OpenForWrite.txt");
  const string test = "Test file for test HDFile.OpenForWrite";

  {
    CHDFile file;
    EXPECT_FALSE(file.Exists(path));
    EXPECT_TRUE(file.OpenForWrite(path));
    EXPECT_TRUE(file.Exists(path));
    EXPECT_EQ(test.length(), file.Write(test.data(), test.length()));
    EXPECT_EQ(test.length(), file.GetLength());
    file.Close();
  }
  {
    CHDFile file;
    EXPECT_FALSE(file.OpenForWrite(path));
    EXPECT_TRUE(file.OpenForWrite(path, true));
    EXPECT_EQ(0, file.GetLength());
    file.Close();
  }
  {
    CHDFile file;
    EXPECT_TRUE(file.Delete(path));
  }
}

TEST(HDFile, Read)
{
  const string path = CSpecialProtocol::TranslatePath("special://temp/HDFile.Read.txt");
  const string test = "Test file for test HDFile.Read";

  {
    CHDFile file;
    EXPECT_FALSE(file.Exists(path));
    EXPECT_TRUE(file.OpenForWrite(path));
    EXPECT_EQ(test.length(), file.Write(test.data(), test.length()));
    file.Close();
  }
  {
    char buffer1[100] = {0};
    char buffer2[100] = {0};
    char buffer3[100] = {0};
    char buffer4[100] = {0};
    char buffer5[100] = {0};

    CHDFile file;
    EXPECT_TRUE(file.Exists(path));
    EXPECT_TRUE(file.Open(path));
    EXPECT_EQ(0, file.Read(buffer1, 0));
    EXPECT_EQ(0, file.Read(NULL, 1));
    EXPECT_EQ(10, file.Read(buffer1, 10));
    EXPECT_STREQ(test.substr(0, 10).c_str(), buffer1);
    EXPECT_EQ(10, file.Read(buffer2, 10));
    EXPECT_STREQ(test.substr(10, 10).c_str(), buffer2);
    EXPECT_EQ(test.length() - 20, file.Read(buffer3, 100));
    EXPECT_STREQ(test.substr(20).c_str(), buffer3);
    EXPECT_EQ(0, file.Seek(0, SEEK_SET));
    EXPECT_EQ(test.length(), file.Read(buffer4, 100));
    EXPECT_STREQ(test.c_str(), buffer4);
    EXPECT_EQ(0, file.Seek(0, SEEK_SET));
    file.Close();
    EXPECT_EQ(0, file.Read(buffer5, 100));
  }
  {
    CHDFile file;
    EXPECT_TRUE(file.Delete(path));
  }
}

/*
TEST(HDFile, ReadLine)
{
}
*/

TEST(HDFile, Write)
{
  const string path = CSpecialProtocol::TranslatePath("special://temp/HDFile.Write.txt");
  const string test = "Test file for test HDFile.Write";
  const string test2 = " (with a second write)";
  const string test3 = "file is closed!";

  {
    CHDFile file;
    EXPECT_FALSE(file.Exists(path));
    EXPECT_TRUE(file.OpenForWrite(path));
    EXPECT_EQ(0, file.Write("test", 0));
    EXPECT_EQ(0, file.GetLength());
    EXPECT_EQ(0, file.Write(NULL, 1));
    EXPECT_EQ(0, file.GetLength());
    EXPECT_EQ(test.length(), file.Write(test.data(), test.length()));
    EXPECT_EQ(test.length(), file.GetLength());
    EXPECT_EQ(test2.length(), file.Write(test2.data(), test2.length()));
    EXPECT_EQ(test.length() + test2.length(), file.GetLength());
    file.Close();
    EXPECT_EQ(0, file.Write(test3.data(), test3.length()));
  }
  {
    const string tests = test + test2;
    string input;
    input.resize(tests.length() + 1);
    CHDFile file;
    EXPECT_TRUE(file.Exists(path));
    EXPECT_TRUE(file.Open(path));
    EXPECT_EQ(tests.length(), file.Read(const_cast<char*>(input.data()), tests.length()));
    input[tests.length()] = '\0';
    EXPECT_STREQ(tests.c_str(), input.c_str());
    file.Close();
  }
  {
    CHDFile file;
    EXPECT_TRUE(file.Delete(path));
  }
}

/*
TEST(HDFile, WriteFile)
{
}
*/

TEST(HDFile, Seek)
{
  // TODO
}

/*
TEST(HDFile, Truncate)
{
}
*/

TEST(HDFile, GetPosition)
{
}

TEST(HDFile, GetLength)
{
}

/*
TEST(HDFile, Close)
{
}
*/

TEST(HDFile, Exists)
{
}

TEST(HDFile, Stat)
{
}

TEST(HDFile, Delete)
{
}

TEST(HDFile, Rename)
{
}

TEST(HDFile, SetHidden)
{
}

TEST(HDFile, GetLocal)
{
  //CHDFileTest
}

}
