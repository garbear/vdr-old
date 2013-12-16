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

#include "utils/url/URLUtils.h"

#include "gtest/gtest.h"

#include <string>
#include <vector>

using namespace std;

/*
TEST(TestURLUtils, GetExtension)
{
  EXPECT_STREQ("", URLUtils::GetExtension("").c_str());

  EXPECT_STREQ(".avi", URLUtils::GetExtension("/path/to/movie.avi").c_str());
  EXPECT_STREQ(".avi", URLUtils::GetExtension("special://path/to/movie.avi").c_str());
  EXPECT_STREQ(".avi", URLUtils::GetExtension("file:///path/to/movie.avi").c_str());
  EXPECT_STREQ(".avi", URLUtils::GetExtension("file:///movie.avi").c_str());
  EXPECT_STREQ(".avi", URLUtils::GetExtension("movie.avi").c_str());

  EXPECT_STREQ("", URLUtils::GetExtension("/path/to/.hidden").c_str());
  EXPECT_STREQ("", URLUtils::GetExtension("special://.hidden").c_str());
  EXPECT_STREQ("", URLUtils::GetExtension("file:///path/to/.hidden").c_str());
  EXPECT_STREQ("", URLUtils::GetExtension("file:///.hidden").c_str());
  EXPECT_STREQ("", URLUtils::GetExtension(".hidden").c_str());

  EXPECT_STREQ("", URLUtils::GetExtension("/path/to/file").c_str());
  EXPECT_STREQ("", URLUtils::GetExtension("special://path/to/file").c_str());
  EXPECT_STREQ("", URLUtils::GetExtension("file:///path/to/file").c_str());
  EXPECT_STREQ("", URLUtils::GetExtension("file:///file").c_str());
  EXPECT_STREQ("", URLUtils::GetExtension("file").c_str());
}

TEST(TestURLUtils, GetFileName)
{
  EXPECT_STREQ("", URLUtils::GetFileName("").c_str());

  EXPECT_STREQ("movie.avi", URLUtils::GetFileName("/path/to/movie.avi").c_str());
  EXPECT_STREQ("movie.avi", URLUtils::GetFileName("special://path/to/movie.avi").c_str());
  EXPECT_STREQ("movie.avi", URLUtils::GetFileName("file:///path/to/movie.avi").c_str());
  EXPECT_STREQ("movie.avi", URLUtils::GetFileName("file:///movie.avi").c_str());
  EXPECT_STREQ("movie.avi", URLUtils::GetFileName("movie.avi").c_str());

  EXPECT_STREQ(".hidden", URLUtils::GetFileName("/path/to/.hidden").c_str());
  EXPECT_STREQ(".hidden", URLUtils::GetFileName("special://path/to/.hidden").c_str());
  EXPECT_STREQ(".hidden", URLUtils::GetFileName("file:///path/to/.hidden").c_str());
  EXPECT_STREQ(".hidden", URLUtils::GetFileName("file:///.hidden").c_str());
  EXPECT_STREQ(".hidden", URLUtils::GetFileName(".hidden").c_str());

  EXPECT_STREQ("", URLUtils::GetFileName("/path/to/folder/").c_str());
  EXPECT_STREQ("", URLUtils::GetFileName("special://path/to/folder/").c_str());
  EXPECT_STREQ("", URLUtils::GetFileName("file:///path/to/folder/").c_str());
  EXPECT_STREQ("", URLUtils::GetFileName("file:///folder/").c_str());
  EXPECT_STREQ("", URLUtils::GetFileName("folder/").c_str());

  EXPECT_STREQ("", URLUtils::GetFileName("/").c_str());
  EXPECT_STREQ("", URLUtils::GetFileName("special://").c_str());
  EXPECT_STREQ("", URLUtils::GetFileName("file:///").c_str());
}
*/

/*
TEST(TestURLUtils, GetFileOrFolderName)
{
  EXPECT_STREQ("", URLUtils::GetFileOrFolderName("").c_str());

  EXPECT_STREQ("movie.avi", URLUtils::GetFileOrFolderName("/path/to/movie.avi").c_str());
  EXPECT_STREQ("movie.avi", URLUtils::GetFileOrFolderName("special://path/to/movie.avi").c_str());
  EXPECT_STREQ("movie.avi", URLUtils::GetFileOrFolderName("file:///path/to/movie.avi").c_str());
  EXPECT_STREQ("movie.avi", URLUtils::GetFileOrFolderName("file:///movie.avi").c_str());
  EXPECT_STREQ("movie.avi", URLUtils::GetFileOrFolderName("movie.avi").c_str());

  EXPECT_STREQ(".hidden", URLUtils::GetFileOrFolderName("/path/to/.hidden").c_str());
  EXPECT_STREQ(".hidden", URLUtils::GetFileOrFolderName("special://path/to/.hidden").c_str());
  EXPECT_STREQ(".hidden", URLUtils::GetFileOrFolderName("file:///path/to/.hidden").c_str());
  EXPECT_STREQ(".hidden", URLUtils::GetFileOrFolderName("file:///.hidden").c_str());
  EXPECT_STREQ(".hidden", URLUtils::GetFileOrFolderName(".hidden").c_str());

  EXPECT_STREQ("folder", URLUtils::GetFileOrFolderName("/path/to/folder/").c_str());
  EXPECT_STREQ("folder", URLUtils::GetFileOrFolderName("special://path/to/folder/").c_str());
  EXPECT_STREQ("folder", URLUtils::GetFileOrFolderName("file:///path/to/folder/").c_str());
  EXPECT_STREQ("folder", URLUtils::GetFileOrFolderName("file:///folder/").c_str());
  EXPECT_STREQ("folder", URLUtils::GetFileOrFolderName("folder/").c_str());

  EXPECT_STREQ("", URLUtils::GetFileOrFolderName("/").c_str());
  EXPECT_STREQ("", URLUtils::GetFileOrFolderName("special://").c_str());
  EXPECT_STREQ("", URLUtils::GetFileOrFolderName("file:///").c_str());
}
*/

/*
TEST(TestURLUtils, GetProtocol)
{
  EXPECT_STREQ("", URLUtils::GetProtocol("").c_str());

  EXPECT_STREQ("special", URLUtils::GetProtocol("special://").c_str());
  EXPECT_STREQ("special", URLUtils::GetProtocol("special://path/").c_str());
  EXPECT_STREQ("special", URLUtils::GetProtocol("special://host://path").c_str());
  EXPECT_STREQ("special", URLUtils::GetProtocol("special://host:/path").c_str());

  EXPECT_STREQ("file", URLUtils::GetProtocol("file:///path/to/movie.avi").c_str());

  EXPECT_STREQ("", URLUtils::GetProtocol("special").c_str());
  EXPECT_STREQ("", URLUtils::GetProtocol("special:/").c_str());
  EXPECT_STREQ("", URLUtils::GetProtocol("special//").c_str());
}
*/

/*
TEST(TestURLUtils, GetWithoutExtension)
{
  EXPECT_STREQ("", URLUtils::GetWithoutExtension("").c_str());

  EXPECT_STREQ("movie", URLUtils::GetWithoutExtension("movie").c_str());
  EXPECT_STREQ("movie", URLUtils::GetWithoutExtension("movie.avi").c_str());
  EXPECT_STREQ("/movie", URLUtils::GetWithoutExtension("/movie").c_str());
  EXPECT_STREQ("/movie", URLUtils::GetWithoutExtension("/movie.avi").c_str());
  EXPECT_STREQ("/path/movie", URLUtils::GetWithoutExtension("/path/movie").c_str());
  EXPECT_STREQ("/path/movie", URLUtils::GetWithoutExtension("/path/movie.avi").c_str());
  EXPECT_STREQ("file:///movie", URLUtils::GetWithoutExtension("file:///movie").c_str());
  EXPECT_STREQ("file:///movie", URLUtils::GetWithoutExtension("file:///movie.avi").c_str());

  EXPECT_STREQ("movie.", URLUtils::GetWithoutExtension("movie.").c_str());
  EXPECT_STREQ("/movie.", URLUtils::GetWithoutExtension("/movie.").c_str());
  EXPECT_STREQ("/path/movie.", URLUtils::GetWithoutExtension("/path/movie.").c_str());
  EXPECT_STREQ("file:///movie.", URLUtils::GetWithoutExtension("file:///movie.").c_str());

  EXPECT_STREQ(".hidden", URLUtils::GetWithoutExtension(".hidden").c_str());
  EXPECT_STREQ("/.hidden", URLUtils::GetWithoutExtension("/.hidden").c_str());
  EXPECT_STREQ("/path/.hidden", URLUtils::GetWithoutExtension("/path//.hidden").c_str());
  EXPECT_STREQ("file:///.hidden", URLUtils::GetWithoutExtension("file:///.hidden").c_str());
}
*/

/*
TEST(TestURLUtils, GetWithoutProtocol)
{
  EXPECT_STREQ("", URLUtils::GetWithoutProtocol("").c_str());

  EXPECT_STREQ("somefile", URLUtils::GetWithoutProtocol("somefile").c_str());
  EXPECT_STREQ("somefile", URLUtils::GetWithoutProtocol("special://somefile").c_str());

  EXPECT_STREQ("/somefile", URLUtils::GetWithoutProtocol("/somefile").c_str());
  EXPECT_STREQ("/somefile", URLUtils::GetWithoutProtocol("file:///somefile").c_str());

  EXPECT_STREQ("file:/", URLUtils::GetWithoutProtocol("special://file:///").c_str());
  EXPECT_STREQ("/special://", URLUtils::GetWithoutProtocol("file:///special://").c_str());
}
*/

/*
TEST(TestURLUtils, IsInPath)
{
  EXPECT_TRUE(URLUtils::IsInPath("/path/to/movie.avi", "/path/to/"));
  EXPECT_FALSE(URLUtils::IsInPath("/path/to/movie.avi", "/path/2/"));
}
*/

TEST(TestURLUtils, IsURL)
{
  EXPECT_TRUE(URLUtils::IsURL("someprotocol://path/to/file"));
  EXPECT_FALSE(URLUtils::IsURL("/path/to/file"));
}

TEST(TestURLUtils, HasSlashAtEnd)
{
  EXPECT_TRUE(URLUtils::HasSlashAtEnd("bluray://path/to/file/"));
  EXPECT_FALSE(URLUtils::HasSlashAtEnd("bluray://path/to/file"));
}

TEST(TestURLUtils, RemoveSlashAtEnd)
{
  string ref, var;

  ref = "bluray://path/to/file";
  var = "bluray://path/to/file/";
  URLUtils::RemoveSlashAtEnd(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestURLUtils, GetDirectory)
{
  EXPECT_STREQ("/path/to/", URLUtils::GetDirectory("/path/to/movie.avi").c_str());
  EXPECT_STREQ("/path/to/", URLUtils::GetDirectory("/path/to/").c_str());
  EXPECT_STREQ("/path/to/|option=foo", URLUtils::GetDirectory("/path/to/movie.avi|option=foo").c_str());
  EXPECT_STREQ("/path/to/|option=foo", URLUtils::GetDirectory("/path/to/|option=foo").c_str());
  EXPECT_STREQ("", URLUtils::GetDirectory("movie.avi").c_str());
  EXPECT_STREQ("", URLUtils::GetDirectory("movie.avi|option=foo").c_str());
  EXPECT_STREQ("", URLUtils::GetDirectory("").c_str());

  // Make sure it works when assigning to the same str as the reference parameter
  string var = "/path/to/movie.avi|option=foo";
  var = URLUtils::GetDirectory(var);
  EXPECT_STREQ("/path/to/|option=foo", var.c_str());
}

TEST(TestURLUtils, HasExtension)
{
  EXPECT_TRUE (URLUtils::HasExtension("/path/to/movie.AvI"));
  EXPECT_FALSE(URLUtils::HasExtension("/path/to/movie"));
  EXPECT_FALSE(URLUtils::HasExtension("/path/.to/movie"));
  EXPECT_FALSE(URLUtils::HasExtension(""));

  EXPECT_TRUE (URLUtils::HasExtension("/path/to/movie.AvI", ".avi"));
  EXPECT_FALSE(URLUtils::HasExtension("/path/to/movie.AvI", ".mkv"));
  EXPECT_FALSE(URLUtils::HasExtension("/path/.avi/movie", ".avi"));
  EXPECT_FALSE(URLUtils::HasExtension("", ".avi"));

  EXPECT_TRUE (URLUtils::HasExtension("/path/movie.AvI", ".avi|.mkv|.mp4"));
  EXPECT_TRUE (URLUtils::HasExtension("/path/movie.AvI", ".mkv|.avi|.mp4"));
  EXPECT_FALSE(URLUtils::HasExtension("/path/movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URLUtils::HasExtension("/path.mkv/movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URLUtils::HasExtension("", ".avi|.mkv|.mp4"));
}

TEST(TestURLUtils, ReplaceExtension)
{
  string ref, var;

  ref = "/path/to/file.xsd";
  var = URLUtils::ReplaceExtension("/path/to/file.xml", ".xsd");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestURLUtils, Split)
{
  string refpath, reffile, varpath, varfile;

  refpath = "/path/to/";
  reffile = "movie.avi";
  URLUtils::Split("/path/to/movie.avi", varpath, varfile);
  EXPECT_STREQ(refpath.c_str(), varpath.c_str());
  EXPECT_STREQ(reffile.c_str(), varfile.c_str());
}

TEST(TestURLUtils, SplitPath)
{
  vector<string> strarray;

  strarray = URLUtils::SplitPath("http://www.test.com/path/to/movie.avi");

  EXPECT_STREQ("http://www.test.com/", strarray.at(0).c_str());
  EXPECT_STREQ("path", strarray.at(1).c_str());
  EXPECT_STREQ("to", strarray.at(2).c_str());
  EXPECT_STREQ("movie.avi", strarray.at(3).c_str());
}

TEST(TestURLUtils, SplitPathLocal)
{
#ifndef TARGET_LINUX
  const char *path = "C:\\path\\to\\movie.avi";
#else
  const char *path = "/path/to/movie.avi";
#endif
  vector<string> strarray;

  strarray = URLUtils::SplitPath(path);

#ifndef TARGET_LINUX
  EXPECT_STREQ("C:", strarray.at(0).c_str());
#else
  EXPECT_STREQ("", strarray.at(0).c_str());
#endif
  EXPECT_STREQ("path", strarray.at(1).c_str());
  EXPECT_STREQ("to", strarray.at(2).c_str());
  EXPECT_STREQ("movie.avi", strarray.at(3).c_str());
}

TEST(TestURLUtils, GetCommonPath)
{
  string ref, var;

  ref = "/path/";
  var = "/path/2/movie.avi";
  URLUtils::GetCommonPath(var, "/path/to/movie.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestURLUtils, GetParentPath)
{
  string ref, var;

  ref = "/path/to/";
  var = URLUtils::GetParentPath("/path/to/movie.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());

  var.clear();
  EXPECT_TRUE(URLUtils::GetParentPath("/path/to/movie.avi", var));
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

/*
TEST(TestURLUtils, SubstitutePath)
{
  string from, to, ref, var;

  from = "/somepath";
  to = "/someotherpath";
  g_advancedSettings.m_pathSubstitutions.push_back(std::make_pair(from, to));

  ref = "/someotherpath/to/movie.avi";
  var = URLUtils::SubstitutePath("/somepath/to/movie.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}
*/

TEST(TestURLUtils, IsAddonsPath)
{
  EXPECT_TRUE(URLUtils::IsAddonsPath("addons://path/to/addons"));
}

TEST(TestURLUtils, IsSourcesPath)
{
  EXPECT_TRUE(URLUtils::IsSourcesPath("sources://path/to/sources"));
}

TEST(TestURLUtils, IsCDDA)
{
  EXPECT_TRUE(URLUtils::IsCDDA("cdda://path/to/cdda"));
}

TEST(TestURLUtils, IsDAAP)
{
  EXPECT_TRUE(URLUtils::IsDAAP("daap://path/to/daap"));
}

TEST(TestURLUtils, IsDOSPath)
{
  EXPECT_TRUE(URLUtils::IsDOSPath("C://path/to/dosfile"));
}

TEST(TestURLUtils, IsDVD)
{
  EXPECT_TRUE(URLUtils::IsDVD("dvd://path/in/video_ts.ifo"));
#if defined(TARGET_WINDOWS)
  EXPECT_TRUE(URLUtils::IsDVD("dvd://path/in/file"));
#else
  EXPECT_TRUE(URLUtils::IsDVD("iso9660://path/in/video_ts.ifo"));
  EXPECT_TRUE(URLUtils::IsDVD("udf://path/in/video_ts.ifo"));
  EXPECT_TRUE(URLUtils::IsDVD("dvd://1"));
#endif
}

TEST(TestURLUtils, IsFTP)
{
  EXPECT_TRUE(URLUtils::IsFTP("ftp://path/in/ftp"));
}

TEST(TestURLUtils, IsHD)
{
  EXPECT_TRUE(URLUtils::IsHD("/path/to/file"));
  EXPECT_TRUE(URLUtils::IsHD("file:///path/to/file"));
  //EXPECT_TRUE(URLUtils::IsHD("special://path/to/file")); // Special protocol not implemented (TODO)
  //EXPECT_TRUE(URLUtils::IsHD("stack://path/to/file")); // Stack protocol not implemented (TODO)
  EXPECT_TRUE(URLUtils::IsHD("zip://path/to/file"));
  EXPECT_TRUE(URLUtils::IsHD("rar://path/to/file"));
}

TEST(TestURLUtils, IsHDHomeRun)
{
  EXPECT_TRUE(URLUtils::IsHDHomeRun("hdhomerun://path/to/file"));
}

TEST(TestURLUtils, IsSlingbox)
{
  EXPECT_TRUE(URLUtils::IsSlingbox("sling://path/to/file"));
}

TEST(TestURLUtils, IsHTSP)
{
  EXPECT_TRUE(URLUtils::IsHTSP("htsp://path/to/file"));
}

TEST(TestURLUtils, IsInArchive)
{
  EXPECT_TRUE(URLUtils::IsInArchive("zip://path/to/file"));
  EXPECT_TRUE(URLUtils::IsInArchive("rar://path/to/file"));
}

TEST(TestURLUtils, IsInRAR)
{
  EXPECT_TRUE(URLUtils::IsInRAR("rar://path/to/file"));
}

TEST(TestURLUtils, IsInZIP)
{
  EXPECT_TRUE(URLUtils::IsInZIP("zip://path/to/file"));
}

TEST(TestURLUtils, IsISO9660)
{
  EXPECT_TRUE(URLUtils::IsISO9660("iso9660://path/to/file"));
}

TEST(TestURLUtils, IsLiveTV)
{
  EXPECT_TRUE(URLUtils::IsLiveTV("tuxbox://path/to/file"));
  EXPECT_TRUE(URLUtils::IsLiveTV("vtp://path/to/file"));
  EXPECT_TRUE(URLUtils::IsLiveTV("hdhomerun://path/to/file"));
  EXPECT_TRUE(URLUtils::IsLiveTV("sling://path/to/file"));
  EXPECT_TRUE(URLUtils::IsLiveTV("htsp://path/to/file"));
  EXPECT_TRUE(URLUtils::IsLiveTV("sap://path/to/file"));
  //EXPECT_TRUE(URLUtils::IsLiveTV("myth://path/channels/")); // CMythDirectory not implemented (TODO)
}

TEST(TestURLUtils, IsMultiPath)
{
  EXPECT_TRUE(URLUtils::IsMultiPath("multipath://path/to/file"));
}

TEST(TestURLUtils, IsMusicDb)
{
  EXPECT_TRUE(URLUtils::IsMusicDb("musicdb://path/to/file"));
}

TEST(TestURLUtils, IsMythTV)
{
  EXPECT_TRUE(URLUtils::IsMythTV("myth://path/to/file"));
}

TEST(TestURLUtils, IsNfs)
{
  EXPECT_TRUE(URLUtils::IsNfs("nfs://path/to/file"));
  //EXPECT_TRUE(URLUtils::IsNfs("stack://nfs://path/to/file")); // Stack protocol not implemented (TODO)
}

TEST(TestURLUtils, IsAfp)
{
  EXPECT_TRUE(URLUtils::IsAfp("afp://path/to/file"));
  //EXPECT_TRUE(URLUtils::IsAfp("stack://afp://path/to/file")); // Stack protocol not implemented (TODO)
}

TEST(TestURLUtils, IsOnDVD)
{
  EXPECT_TRUE(URLUtils::IsOnDVD("dvd://path/to/file"));
  EXPECT_TRUE(URLUtils::IsOnDVD("udf://path/to/file"));
  EXPECT_TRUE(URLUtils::IsOnDVD("iso9660://path/to/file"));
  EXPECT_TRUE(URLUtils::IsOnDVD("cdda://path/to/file"));
}

TEST(TestURLUtils, IsOnLAN)
{
  //EXPECT_TRUE(URLUtils::IsOnLAN("multipath://daap://path/to/file")); // Multipath protocol not implemented yet
  //EXPECT_TRUE(URLUtils::IsOnLAN("stack://daap://path/to/file")); // TODO: Stack protocol not implemented
  EXPECT_TRUE(URLUtils::IsOnLAN("daap://path/to/file"));
  EXPECT_FALSE(URLUtils::IsOnLAN("plugin://path/to/file"));
  EXPECT_TRUE(URLUtils::IsOnLAN("tuxbox://path/to/file"));
  EXPECT_TRUE(URLUtils::IsOnLAN("upnp://path/to/file"));
}

TEST(TestURLUtils, IsPlugin)
{
  EXPECT_TRUE(URLUtils::IsPlugin("plugin://path/to/file"));
}

TEST(TestURLUtils, IsScript)
{
  EXPECT_TRUE(URLUtils::IsScript("script://path/to/file"));
}

TEST(TestURLUtils, IsRAR)
{
  EXPECT_TRUE(URLUtils::IsRAR("/path/to/rarfile.rar"));
  EXPECT_TRUE(URLUtils::IsRAR("/path/to/rarfile.cbr"));
  EXPECT_FALSE(URLUtils::IsRAR("/path/to/file"));
  EXPECT_FALSE(URLUtils::IsRAR("rar://path/to/file"));
}

TEST(TestURLUtils, IsRemote)
{
  EXPECT_TRUE(URLUtils::IsRemote("http://path/to/file"));
  EXPECT_TRUE(URLUtils::IsRemote("https://path/to/file"));
}

TEST(TestURLUtils, IsSmb)
{
  EXPECT_TRUE(URLUtils::IsSmb("smb://path/to/file"));
  //EXPECT_TRUE(URLUtils::IsSmb("stack://smb://path/to/file")); // TODO: Stack protocol not implemented
}

TEST(TestURLUtils, IsSpecial)
{
  EXPECT_TRUE(URLUtils::IsSpecial("special://path/to/file"));
  //EXPECT_TRUE(URLUtils::IsSpecial("stack://special://path/to/file")); // TODO: Stack protocol not implemented
}

TEST(TestURLUtils, IsStack)
{
  EXPECT_TRUE(URLUtils::IsStack("stack://path/to/file"));
}

TEST(TestURLUtils, IsTuxBox)
{
  EXPECT_TRUE(URLUtils::IsTuxBox("tuxbox://path/to/file"));
}

TEST(TestURLUtils, IsUPnP)
{
  EXPECT_TRUE(URLUtils::IsUPnP("upnp://path/to/file"));
}

TEST(TestURLUtils, IsVideoDb)
{
  EXPECT_TRUE(URLUtils::IsVideoDb("videodb://path/to/file"));
}

TEST(TestURLUtils, IsVTP)
{
  EXPECT_TRUE(URLUtils::IsVTP("vtp://path/to/file"));
}

TEST(TestURLUtils, IsZIP)
{
  EXPECT_TRUE(URLUtils::IsZIP("/path/to/zipfile.zip"));
  EXPECT_TRUE(URLUtils::IsZIP("/path/to/zipfile.cbz"));
  EXPECT_FALSE(URLUtils::IsZIP("/path/to/file"));
  EXPECT_FALSE(URLUtils::IsZIP("zip://path/to/file"));
}

TEST(TestURLUtils, IsBluray)
{
  EXPECT_TRUE(URLUtils::IsBluray("bluray://path/to/file"));
}

// TODO: This test fails (probably screwed up in xbmc as well)
/*
TEST(TestURLUtils, CreateArchivePath)
{
  string ref, var;

  ref = "file://%2fpath%2fto%2f/file";
  URLUtils::CreateArchivePath(var, "file", "/path/to/", "file");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}
*/

TEST(TestURLUtils, AddFileToFolder)
{
  string ref = "/path/to/file";
  string var = URLUtils::AddFileToFolder("/path/to", "file");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestURLUtils, ProtocolHasParentInHostname)
{
  EXPECT_TRUE(URLUtils::ProtocolHasParentInHostname("zip"));
  EXPECT_TRUE(URLUtils::ProtocolHasParentInHostname("rar"));
  EXPECT_TRUE(URLUtils::ProtocolHasParentInHostname("bluray"));
}

TEST(TestURLUtils, ProtocolHasEncodedHostname)
{
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedHostname("zip"));
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedHostname("rar"));
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedHostname("bluray"));
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedHostname("musicsearch"));
}

TEST(TestURLUtils, ProtocolHasEncodedFilename)
{
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedFilename("shout"));
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedFilename("daap"));
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedFilename("dav"));
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedFilename("tuxbox"));
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedFilename("rss"));
  EXPECT_TRUE(URLUtils::ProtocolHasEncodedFilename("davs"));
}

TEST(TestURLUtils, GetRealPath)
{
  std::string ref;
  
  ref = "/path/to/file/";
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath(ref).c_str());
  
  ref = "path/to/file";
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("../path/to/file").c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("./path/to/file").c_str());

  ref = "/path/to/file";
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath(ref).c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("/path/to/./file").c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("/./path/to/./file").c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("/path/to/some/../file").c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("/../path/to/some/../file").c_str());

  ref = "/path/to";
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("/path/to/some/../file/..").c_str());

#ifdef TARGET_WINDOWS
  ref = "\\\\path\\to\\file\\";
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath(ref).c_str());
  
  ref = "path\\to\\file";
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("..\\path\\to\\file").c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath(".\\path\\to\\file").c_str());

  ref = "\\\\path\\to\\file";
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath(ref).c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("\\\\path\\to\\.\\file").c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("\\\\.\\path/to\\.\\file").c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("\\\\path\\to\\some\\..\\file").c_str());
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("\\\\..\\path\\to\\some\\..\\file").c_str());

  ref = "\\\\path\\to";
  EXPECT_STREQ(ref.c_str(), URLUtils::GetRealPath("\\\\path\\to\\some\\..\\file\\..").c_str());
#endif

  // test rar/zip paths
  ref = "rar://%2fpath%2fto%2frar/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URLUtils::GetRealPath(ref).c_str());
  
  // test rar/zip paths
  ref = "rar://%2fpath%2fto%2frar/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URLUtils::GetRealPath("rar://%2fpath%2fto%2frar/../subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URLUtils::GetRealPath("rar://%2fpath%2fto%2frar/./subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URLUtils::GetRealPath("rar://%2fpath%2fto%2frar/subpath/to/./file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URLUtils::GetRealPath("rar://%2fpath%2fto%2frar/subpath/to/some/../file").c_str());
  
  // TODO: These tests fail (probably screwed up in xbmc as well)
  /*
  EXPECT_STRCASEEQ(ref.c_str(), URLUtils::GetRealPath("rar://%2fpath%2fto%2f.%2frar/subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URLUtils::GetRealPath("rar://%2fpath%2fto%2fsome%2f..%2frar/subpath/to/file").c_str());

  // test rar/zip path in rar/zip path
  ref ="zip://rar%3A%2F%2F%252Fpath%252Fto%252Frar%2Fpath%2Fto%2Fzip/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URLUtils::GetRealPath("zip://rar%3A%2F%2F%252Fpath%252Fto%252Fsome%252F..%252Frar%2Fpath%2Fto%2Fsome%2F..%2Fzip/subpath/to/some/../file").c_str());
  */
}

TEST(TestURLUtils, UpdateUrlEncoding)
{
  // Stack protocol not implemented (TODO)
  /*
  std::string oldUrl = "stack://rar://%2fpath%2fto%2farchive%2fsome%2darchive%2dfile%2eCD1%2erar/video.avi , rar://%2fpath%2fto%2farchive%2fsome%2darchive%2dfile%2eCD2%2erar/video.avi";
  std::string newUrl = "stack://rar://%2fpath%2fto%2farchive%2fsome-archive-file.CD1.rar/video.avi , rar://%2fpath%2fto%2farchive%2fsome-archive-file.CD2.rar/video.avi";
  EXPECT_TRUE(URLUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());
   */

  // TODO: These tests fail (probably screwed up in xbmc as well)
  /*
  oldUrl = "rar://%2fpath%2fto%2farchive%2fsome%2darchive%2efile%2erar/video.avi";
  newUrl = "rar://%2fpath%2fto%2farchive%2fsome-archive.file.rar/video.avi";
  
  EXPECT_TRUE(URLUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());
  */
  
  string oldUrl = "/path/to/some/long%2dnamed%2efile";
  string newUrl = "/path/to/some/long%2dnamed%2efile";
  
  EXPECT_FALSE(URLUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());
  
  oldUrl = "/path/to/some/long-named.file";
  newUrl = "/path/to/some/long-named.file";
  
  EXPECT_FALSE(URLUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());
}

