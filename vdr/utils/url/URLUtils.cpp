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

#include "URLUtils.h"
//#include "filesystem/SpecialProtocol.h" // TODO
#include "platform/os.h"
#include "utils/StringUtils.h"
/*
#include "network/Network.h"
#include "Application.h"
#include "FileItem.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/MythDirectory.h"
#include "filesystem/StackDirectory.h"
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "URL.h"
*/

#include <algorithm>
#include <stdio.h> // for sscanf()
#include <string.h>

//#include <netinet/in.h>
//#include <arpa/inet.h>

using namespace std;

string URLUtils::AddFileToFolder(const string& strFolder, const string& strFile)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    if (url.GetFileName() != strFolder)
    {
      url.SetFileName(AddFileToFolder(url.GetFileName(), strFile));
      return url.Get();
    }
  }

  string strResult = strFolder;
  if (!strResult.empty())
    AddSlashAtEnd(strResult);

  // Remove any slash at the start of the file
  if (strFile.size() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.substr(1);
  else
    strResult += strFile;

  // correct any slash directions
  if (!IsDOSPath(strFolder))
    StringUtils::Replace(strResult, '\\', '/');
  else
    StringUtils::Replace(strResult, '/', '\\');

  return strResult;
}

string URLUtils::CreateArchivePath(const string& strType, const string& strArchivePath, const string& strFilePathInArchive, const string& strPwd)
{
  string strBuffer;

  string strUrlPath = strType+"://";

  if( !strPwd.empty() )
  {
    strBuffer = strPwd;
    Encode(strBuffer);
    strUrlPath += strBuffer;
    strUrlPath += "@";
  }

  strBuffer = strArchivePath;
  Encode(strBuffer);

  strUrlPath += strBuffer;

  strBuffer = strFilePathInArchive;
  StringUtils::Replace(strBuffer, '\\', '/');
  StringUtils::TrimLeft(strBuffer, "/");

  strUrlPath += "/";
  strUrlPath += strBuffer;

#if 0 // options are not used
  strBuffer = strCachePath;
  Encode(strBuffer);

  strUrlPath += "?cache=";
  strUrlPath += strBuffer;

  strBuffer = StringUtils::Format("%i", wOptions);
  strUrlPath += "&flags=";
  strUrlPath += strBuffer;
#endif

  return strUrlPath;
}

// Modified to be more accommodating - if a non hex value follows a % take the characters directly and don't raise an error.
// However % characters should really be escaped like any other non safe character (www.rfc-editor.org/rfc/rfc1738.txt)
string URLUtils::Decode(const string& strURLData)
{
  string strResult;

  /* result will always be less than source */
  strResult.reserve( strURLData.length() );

  for (unsigned int i = 0; i < strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == '+') strResult += ' ';
    else if (kar == '%')
    {
      if (i < strURLData.size() - 2)
      {
        string strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num=-1;
        sscanf(strTmp.c_str(), "%x", (unsigned int *)&dec_num);
        if (dec_num<0 || dec_num>255)
          strResult += kar;
        else
        {
          strResult += (char)dec_num;
          i += 2;
        }
      }
      else
        strResult += kar;
    }
    else strResult += kar;
  }
  return strResult;
}

string URLUtils::Encode(const string& strURLData)
{
  string strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strURLData.length() * 2 );

  for (int i = 0; i < (int)strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    //if (kar == ' ') strResult += '+'; // obsolete
    if (isalnum(kar) || strchr("-_.!()" , kar) ) // Don't URL encode these according to RFC1738
    {
      strResult += kar;
    }
    else
    {
      string strTmp = StringUtils::Format("%%%02.2x", kar);
      strResult += strTmp;
    }
  }
  return strResult;
}

string URLUtils::FixSlashesAndDups(const string& path, const char slashCharacter /* = '/' */, const size_t startFrom /*= 0*/)
{
  const size_t len = path.length();
  if (startFrom >= len)
    return path;

  string result(path, 0, startFrom);
  result.reserve(len);

  const char* const str = path.c_str();
  size_t pos = startFrom;
  do
  {
    if (str[pos] == '\\' || str[pos] == '/')
    {
      result.push_back(slashCharacter);  // append one slash
      pos++;
      // skip any following slashes
      while (str[pos] == '\\' || str[pos] == '/') // str is null-terminated, no need to check for buffer overrun
        pos++;
    }
    else
      result.push_back(str[pos++]);   // append current char and advance pos to next char

  } while (pos < len);

  return result;
}

string URLUtils::GetDirectory(const string &strFilePath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end and possible |option=foo options.

  size_t iPosSlash = strFilePath.find_last_of("/\\");
  if (iPosSlash == string::npos)
    return ""; // No slash, so no path (ignore any options)

  size_t iPosBar = strFilePath.rfind('|');
  if (iPosBar == string::npos)
    return strFilePath.substr(0, iPosSlash + 1); // Only path

  return strFilePath.substr(0, iPosSlash + 1) + strFilePath.substr(iPosBar); // Path + options
}

string URLUtils::GetExtension(const string& strFileName)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return GetExtension(url.GetFileName());
  }

  size_t period = strFileName.find_last_of("./\\");
  if (period == string::npos || strFileName[period] != '.')
    return StringUtils::Empty;

  return strFileName.substr(period);
}

string URLUtils::GetFileName(const string& strFileNameAndPath)
{
  if (IsURL(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    return GetFileName(url.GetFileName());
  }

  /* find the last slash */
  const size_t slash = strFileNameAndPath.find_last_of("/\\");
  return strFileNameAndPath.substr(slash+1);
}

string URLUtils::GetParentPath(const string& strPath)
{
  string strReturn;
  GetParentPath(strPath, strReturn);
  return strReturn;
}

string URLUtils::GetRealPath(const string &path)
{
  if (path.empty())
    return path;

  CURL url(path);
  url.SetHostName(GetRealPath(url.GetHostName()));
  url.SetFileName(resolvePath(url.GetFileName()));

  return url.Get();
}

string URLUtils::GetRedacted(const string& path)
{
  return CURL(path).GetRedacted();
}

void URLUtils::AddSlashAtEnd(string& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    std::string file = url.GetFileName();
    if(!file.empty() && file != strFolder)
    {
      AddSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
    }
    return;
  }

  if (!HasSlashAtEnd(strFolder))
  {
    if (IsDOSPath(strFolder))
      strFolder += '\\';
    else
      strFolder += '/';
  }
}

void URLUtils::RemoveSlashAtEnd(string& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    string file = url.GetFileName();
    if (!file.empty() && file != strFolder)
    {
      RemoveSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
      return;
    }
    if(url.GetHostName().empty())
      return;
  }

  while (HasSlashAtEnd(strFolder))
    strFolder.erase(strFolder.size()-1, 1);
}

string URLUtils::ReplaceExtension(const string& filename, const string& newExtension)
{
  string extension = GetExtension(filename);
  if (!extension.empty())
  {
    string newFile(filename);
    newFile.replace(newFile.end() - extension.length(), newFile.end(), newExtension);
    return newFile;
  }
  return filename;
}

vector<string> URLUtils::SplitPath(const string& strPath)
{
  CURL url(strPath);

  // silly string can't take a char in the constructor
  string sep(1, url.GetDirectorySeparator());

  // split the filename portion of the URL up into separate dirs
  vector<string> dirs;
  StringUtils::Split(url.GetFileName(), sep, dirs);

  // we start with the root path
  string dir = url.GetWithoutFilename();

  if (!dir.empty())
    dirs.insert(dirs.begin(), dir);

  // we don't need empty token on the end
  if (dirs.size() > 1 && dirs.back().empty())
    dirs.erase(dirs.end() - 1);

  return dirs;
}

string URLUtils::SubstitutePath(const string& strPath, bool reverse /* = false */)
{
  // Not implemented (TODO)
  /*
  for (CAdvancedSettings::StringMapping::iterator i = g_advancedSettings.m_pathSubstitutions.begin(); i != g_advancedSettings.m_pathSubstitutions.end(); i++)
  {
    if (!reverse)
    {
      if (strncmp(strPath.c_str(), i->first.c_str(), HasSlashAtEnd(i->first.c_str()) ? i->first.size()-1 : i->first.size()) == 0)
      {
        if (strPath.size() > i->first.size())
          return AddFileToFolder(i->second, strPath.substr(i->first.size()));
        else
          return i->second;
      }
    }
    else
    {
      if (strncmp(strPath.c_str(), i->second.c_str(), HasSlashAtEnd(i->second.c_str()) ? i->second.size()-1 : i->second.size()) == 0)
      {
        if (strPath.size() > i->second.size())
          return AddFileToFolder(i->first, strPath.substr(i->second.size()));
        else
          return i->first;
      }
    }
  }
  */
  return strPath;
}

string URLUtils::TranslateProtocol(const string& protocol)
{
  if (protocol == "shout"
   || protocol == "daap"
   || protocol == "dav"
   || protocol == "tuxbox"
   || protocol == "rss")
   return "http";

  if (protocol == "davs")
    return "https";

  return protocol;
}

string URLUtils::ValidatePath(const string &path, bool bFixDoubleSlashes /* = false */)
{
  string result = path;

  // Don't do any stuff on URLs containing %-characters or protocols that embed
  // filenames. NOTE: Don't use IsInZip or IsInRar here since it will infinitely
  // recurse and crash XBMC
  if (IsURL(path) &&
      (path.find('%') != string::npos ||
      StringUtils::StartsWithNoCase(path, "apk:") ||
      StringUtils::StartsWithNoCase(path, "zip:") ||
      StringUtils::StartsWithNoCase(path, "rar:") ||
      StringUtils::StartsWithNoCase(path, "stack:") ||
      StringUtils::StartsWithNoCase(path, "bluray:") ||
      StringUtils::StartsWithNoCase(path, "multipath:") ))
    return result;

  // check the path for incorrect slashes
#ifdef TARGET_WINDOWS
  if (IsDOSPath(path))
  {
    StringUtils::Replace(result, '/', '\\');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes && !result.empty())
    {
      // Fixup for double back slashes (but ignore the \\ of unc-paths)
      for (size_t x = 1; x < result.size() - 1; x++)
      {
        if (result[x] == '\\' && result[x+1] == '\\')
          result.erase(x);
      }
    }
  }
  else if (path.find("://") != string::npos || path.find(":\\\\") != string::npos)
#endif
  {
    StringUtils::Replace(result, '\\', '/');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes && !result.empty())
    {
      // Fixup for double forward slashes(/) but don't touch the :// of URLs
      for (size_t x = 2; x < result.size() - 1; x++)
      {
        if ( result[x] == '/' && result[x + 1] == '/' && !(result[x - 1] == ':' || (result[x - 1] == '/' && result[x - 2] == ':')) )
          result.erase(x);
      }
    }
  }
  return result;
}

bool URLUtils::Compare(const string& strPath1, const string& strPath2)
{
  // TODO: Canonicalize paths
  string strc1(strPath1);
  string strc2(strPath2);
  RemoveSlashAtEnd(strc1);
  RemoveSlashAtEnd(strc2);
  return StringUtils::EqualsNoCase(strc1, strc2);
}

bool URLUtils::GetParentPath(const string& strPath, string& strParent)
{
  strParent = "";

  CURL url(strPath);
  string strFile = url.GetFileName();
  if (ProtocolHasParentInHostname(url.GetProtocol()) && strFile.empty())
  {
    strFile = url.GetHostName();
    return GetParentPath(strFile, strParent);
  }
  else if (url.GetProtocol() == "stack")
  {
    // stack:// not implemented (TODO)
    /*
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(strPath,items);
    items[0]->m_strDVDLabel = GetDirectory(items[0]->GetPath());
    if (StringUtils::StartsWithNoCase(items[0]->m_strDVDLabel, "rar://") || StringUtils::StartsWithNoCase(items[0]->m_strDVDLabel, "zip://"))
      GetParentPath(items[0]->m_strDVDLabel, strParent);
    else
      strParent = items[0]->m_strDVDLabel;
    for( int i=1;i<items.Size();++i)
    {
      items[i]->m_strDVDLabel = GetDirectory(items[i]->GetPath());
      if (StringUtils::StartsWithNoCase(items[0]->m_strDVDLabel, "rar://") || StringUtils::StartsWithNoCase(items[0]->m_strDVDLabel, "zip://"))
        items[i]->SetPath(GetParentPath(items[i]->m_strDVDLabel));
      else
        items[i]->SetPath(items[i]->m_strDVDLabel);

      GetCommonPath(strParent,items[i]->GetPath());
    }
    */
    return true;
  }
  else if (url.GetProtocol() == "multipath")
  {
    // get the parent path of the first item
    return false; // GetParentPath(CMultiPathDirectory::GetFirstPath(strPath), strParent); // TODO
  }
  else if (url.GetProtocol() == "plugin")
  {
    if (!url.GetOptions().empty())
    {
      url.SetOptions("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetFileName().empty())
    {
      url.SetFileName("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetHostName().empty())
    {
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return true;  // already at root
  }
  else if (url.GetProtocol() == "special")
  {
    if (HasSlashAtEnd(strFile))
      strFile.erase(strFile.size() - 1);
    if(strFile.rfind('/') == string::npos)
      return false;
  }
  else if (strFile.size() == 0)
  {
    if (url.GetHostName().size() > 0)
    {
      // we have an share with only server or workgroup name
      // set hostname to "" and return true to get back to root
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return false;
  }

  if (HasSlashAtEnd(strFile) )
  {
    strFile.erase(strFile.size() - 1);
  }

  size_t iPos = strFile.rfind('/');
#ifndef TARGET_POSIX
  if (iPos == string::npos)
  {
    iPos = strFile.rfind('\\');
  }
#endif
  if (iPos == string::npos)
  {
    url.SetFileName("");
    strParent = url.Get();
    return true;
  }

  strFile.erase(iPos);

  AddSlashAtEnd(strFile);

  url.SetFileName(strFile);
  strParent = url.Get();
  return true;
}

bool URLUtils::HasExtension(const string &uri, const string &extensions /* = "" */)
{
  string extension(GetExtension(uri));

  if (extensions.empty())
    return !extension.empty();

  StringUtils::ToLower(extension);

  string extensionsLower(extensions);
  StringUtils::ToLower(extensionsLower); // TODO: Make ToLower() functional and add a new MakeLower();

  vector<string> vecExtensions;
  StringUtils::Tokenize(extensionsLower, vecExtensions, "|");
  return find(vecExtensions.begin(), vecExtensions.end(), extension) != vecExtensions.end();
}

bool URLUtils::HasSlashAtEnd(const string& strFile, bool checkURL /* = false */)
{
  if (strFile.empty()) return false;
  if (checkURL && IsURL(strFile))
  {
    CURL url(strFile);
    string file = url.GetFileName();
    return file.empty() || HasSlashAtEnd(file, false);
  }
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

bool URLUtils::IsAddonsPath(const string& strFile)
{
  CURL url(strFile);
  return StringUtils::EqualsNoCase(url.GetProtocol(), "addons");
}

bool URLUtils::IsAfp(const string& strFile)
{
  string strFile2(strFile);

  // CStackDirectory not implemented (TODO: decide that we don't want stacks)
  /*
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  */

  return StringUtils::StartsWithNoCase(strFile2, "afp:");
}


bool URLUtils::IsAndroidApp(const string &path)
{
  return StringUtils::StartsWithNoCase(path, "androidapp:");
}

bool URLUtils::IsAPK(const string& strFile)
{
  return HasExtension(strFile, ".apk");
}

bool URLUtils::IsArchive(const string& strFile)
{
  return HasExtension(strFile, ".zip|.rar|.apk|.cbz|.cbr");
}

bool URLUtils::IsBluray(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "bluray:");
}

bool URLUtils::IsCDDA(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "cdda:");
}

bool URLUtils::IsDAAP(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "daap:");
}

bool URLUtils::IsDAV(const string& strFile)
{
  string strFile2(strFile);

  // CStackDirectory not implemented (TODO)
  /*
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  */

  return StringUtils::StartsWithNoCase(strFile2, "dav:")  ||
         StringUtils::StartsWithNoCase(strFile2, "davs:");
}

bool URLUtils::IsDOSPath(const string &path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  // windows network drives
  if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
    return true;

  return false;
}

bool URLUtils::IsDVD(const string& strFile)
{
  string strFileLow = strFile;
  StringUtils::ToLower(strFileLow);
  if (strFileLow.find("video_ts.ifo") != string::npos && IsOnDVD(strFile))
    return true;

#if defined(TARGET_WINDOWS)
  if (StringUtils::StartsWithNoCase(strFile, "dvd://"))
    return true;

  if(strFile.size() < 2 || (strFile.substr(1) != ":\\" && strFile.substr(1) != ":"))
    return false;

  if(GetDriveType(strFile.c_str()) == DRIVE_CDROM)
    return true;
#else
  if (strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;
#endif

  return false;
}

bool URLUtils::IsFileOnly(const string &url)
{
  return url.find_first_of("/\\") == string::npos;
}

bool URLUtils::IsFTP(const string& strFile)
{
  string strFile2(strFile);

  // CStackDirectory not implemented (TODO)
  /*
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  */

  return StringUtils::StartsWithNoCase(strFile2, "ftp:")  ||
         StringUtils::StartsWithNoCase(strFile2, "ftps:");
}

bool URLUtils::IsFullPath(const string &url)
{
  if (url.size() && url[0] == '/') return true;     //   /foo/bar.ext
  if (url.find("://") != string::npos) return true;                 //   foo://bar.ext
  if (url.size() > 1 && url[1] == ':') return true; //   c:\\foo\\bar\\bar.ext
  if (StringUtils::StartsWith(url, "\\\\")) return true;    //   \\UNC\path\to\file
  return false;
}

bool URLUtils::IsHD(const string& strFileName)
{
  CURL url(strFileName);

  // Directory classes not implemented (TODO)
  /*
  if (IsSpecial(strFileName))
    return IsHD(CSpecialProtocol::TranslatePath(strFileName));

  if(IsStack(strFileName))
    return IsHD(CStackDirectory::GetFirstStackedFile(strFileName));
  */

  if (ProtocolHasParentInHostname(url.GetProtocol()))
    return IsHD(url.GetHostName());

  return url.GetProtocol().empty() || url.GetProtocol() == "file";
}

bool URLUtils::IsHDHomeRun(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "hdhomerun:");
}

bool URLUtils::IsHostOnLAN(const string& host, bool offLineCheck)
{
  if(host.length() == 0)
    return false;

  // assume a hostname without dot's
  // is local (smb netbios hostnames)
  if(host.find('.') == string::npos)
    return true;

  // Network not implemented (TODO)
  /*
  uint32_t address = ntohl(inet_addr(host.c_str()));
  if(address == INADDR_NONE)
  {
    string ip;
    if(CDNSNameCache::Lookup(host, ip))
      address = ntohl(inet_addr(ip.c_str()));
  }

  if(address != INADDR_NONE)
  {
    if (offLineCheck) // check if in private range, ref https://en.wikipedia.org/wiki/Private_network
    {
      if (
        addr_match(address, "192.168.0.0", "255.255.0.0") ||
        addr_match(address, "10.0.0.0", "255.0.0.0") ||
        addr_match(address, "172.16.0.0", "255.240.0.0")
        )
        return true;
    }
    // check if we are on the local subnet
    if (!g_application.getNetwork().GetFirstConnectedInterface())
      return false;

    if (g_application.getNetwork().HasInterfaceForIP(address))
      return true;
  }
  */

  return false;
}

bool URLUtils::IsHTSP(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "htsp:");
}

bool URLUtils::IsInAPK(const string& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "apk" && url.GetFileName() != "";
}

bool URLUtils::IsInArchive(const string &strFile)
{
  return IsInZIP(strFile) || IsInRAR(strFile) || IsInAPK(strFile);
}

bool URLUtils::IsInPath(const string &uri, const string &baseURI)
{
  string uriPath;// = CSpecialProtocol::TranslatePath(uri); // TODO
  string basePath;// = CSpecialProtocol::TranslatePath(baseURI); // TODO
  return StringUtils::StartsWith(uriPath, basePath);
}

bool URLUtils::IsInRAR(const string& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "rar" && url.GetFileName() != "";
}

bool URLUtils::IsInternetStream(const CURL& url, bool bStrictCheck /* = false */)
{
  string strProtocol = url.GetProtocol();

  if (strProtocol.empty())
    return false;

  // CStackDirectory not implemented (TODO)
  /*
  // there's nothing to stop internet streams from being stacked
  if (strProtocol == "stack")
    return IsInternetStream(CStackDirectory::GetFirstStackedFile(url.Get()));
  */

  string strProtocol2 = url.GetTranslatedProtocol();

  // Special case these
  if (strProtocol  == "ftp"   || strProtocol  == "ftps"   ||
      strProtocol  == "dav"   || strProtocol  == "davs")
    return bStrictCheck;

  if (strProtocol2 == "http"  || strProtocol2 == "https"  ||
      strProtocol2 == "tcp"   || strProtocol2 == "udp"    ||
      strProtocol2 == "rtp"   || strProtocol2 == "sdp"    ||
      strProtocol2 == "mms"   || strProtocol2 == "mmst"   ||
      strProtocol2 == "mmsh"  || strProtocol2 == "rtsp"   ||
      strProtocol2 == "rtmp"  || strProtocol2 == "rtmpt"  ||
      strProtocol2 == "rtmpe" || strProtocol2 == "rtmpte" ||
      strProtocol2 == "rtmps")
    return true;

  return false;
}

bool URLUtils::IsInZIP(const string& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "zip" && url.GetFileName() != "";
}

bool URLUtils::IsISO9660(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "iso9660:");
}

bool URLUtils::IsLibraryFolder(const string& strFile)
{
  CURL url(strFile);
  return StringUtils::EqualsNoCase(url.GetProtocol(), "library");
}

bool URLUtils::IsLiveTV(const string& strFile)
{
  string strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  if(IsTuxBox(strFile)
  || IsVTP(strFile)
  || IsHDHomeRun(strFile)
  || IsSlingbox(strFile)
  || IsHTSP(strFile)
  || StringUtils::StartsWithNoCase(strFile, "sap:")
  ||(StringUtils::EndsWithNoCase(strFileWithoutSlash, ".pvr") && !StringUtils::StartsWithNoCase(strFileWithoutSlash, "pvr://recordings")))
    return true;

  // CMythDirectory not implemented (TODO)
  /*
  if (IsMythTV(strFile) && CMythDirectory::IsLiveTV(strFile))
    return true;
  */

  return false;
}

bool URLUtils::IsMultiPath(const string& strPath)
{
  return StringUtils::StartsWithNoCase(strPath, "multipath:");
}

bool URLUtils::IsMusicDb(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "musicdb:");
}

bool URLUtils::IsMythTV(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "myth:");
}

bool URLUtils::IsNfs(const string& strFile)
{
  string strFile2(strFile);

  // Stack directory not implemented (TODO)
  /*
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  */

  return StringUtils::StartsWithNoCase(strFile2, "nfs:");
}

bool URLUtils::IsOnDVD(const string& strFile)
{
#ifdef TARGET_WINDOWS
  if (strFile.size() >= 2 && strFile.substr(1,1) == ":")
    return (GetDriveType(strFile.substr(0, 3).c_str()) == DRIVE_CDROM);
#endif

  if (StringUtils::StartsWith(strFile, "dvd:"))
    return true;

  if (StringUtils::StartsWith(strFile, "udf:"))
    return true;

  if (StringUtils::StartsWith(strFile, "iso9660:"))
    return true;

  if (StringUtils::StartsWith(strFile, "cdda:"))
    return true;

  return false;
}

bool URLUtils::IsOnLAN(const string& strPath)
{
  // Directory objects not implemented (TODO)
  /*
  if(IsMultiPath(strPath))
    return IsOnLAN(CMultiPathDirectory::GetFirstPath(strPath));

  if(IsStack(strPath))
    return IsOnLAN(CStackDirectory::GetFirstStackedFile(strPath));

  if(IsSpecial(strPath))
    return IsOnLAN(CSpecialProtocol::TranslatePath(strPath));
  */
  if(IsDAAP(strPath))
    return true;

  if(IsPlugin(strPath))
    return false;

  if(IsTuxBox(strPath))
    return true;

  if(IsUPnP(strPath))
    return true;

  CURL url(strPath);
  if (ProtocolHasParentInHostname(url.GetProtocol()))
    return IsOnLAN(url.GetHostName());

  if(!IsRemote(strPath))
    return false;

  string host = url.GetHostName();

  return IsHostOnLAN(host);
}

bool URLUtils::IsPlugin(const string& strFile)
{
  CURL url(strFile);
  return StringUtils::EqualsNoCase(url.GetProtocol(), "plugin");
}

bool URLUtils::IsPVRRecording(const string& strFile)
{
  string strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  return StringUtils::EndsWithNoCase(strFileWithoutSlash, ".pvr") &&
         StringUtils::StartsWithNoCase(strFile, "pvr://recordings");
}

bool URLUtils::IsRAR(const string& strFile)
{
  string strExtension = GetExtension(strFile);

  if (StringUtils::EqualsNoCase(strExtension, ".001") && !StringUtils::EndsWithNoCase(strFile, ".ts.001"))
    return true;

  if (StringUtils::EqualsNoCase(strExtension, ".cbr"))
    return true;

  if (StringUtils::EqualsNoCase(strExtension, ".rar"))
    return true;

  return false;
}

bool URLUtils::IsRemote(const string& strFile)
{
  if (IsCDDA(strFile) || IsISO9660(strFile))
    return false;

  // Directory objects not implemented (TODO)
  /*
  if (IsSpecial(strFile))
    return IsRemote(CSpecialProtocol::TranslatePath(strFile));

  if(IsStack(strFile))
    return IsRemote(CStackDirectory::GetFirstStackedFile(strFile));

  if(IsMultiPath(strFile))
  { // virtual paths need to be checked separately
    vector<string> paths;
    if (CMultiPathDirectory::GetPaths(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }
  */

  CURL url(strFile);
  if(ProtocolHasParentInHostname(url.GetProtocol()))
    return IsRemote(url.GetHostName());

  if (!url.IsLocal())
    return true;

  return false;
}

bool URLUtils::IsScript(const string& strFile)
{
  CURL url(strFile);
  return StringUtils::EqualsNoCase(url.GetProtocol(), "script");
}

bool URLUtils::IsSlingbox(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "sling:");
}

bool URLUtils::IsSmb(const string& strFile)
{
  string strFile2(strFile);

  // Stack Directory not implemented (TODO)
  /*
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  */

  return StringUtils::StartsWithNoCase(strFile2, "smb:");
}

bool URLUtils::IsSourcesPath(const string& strPath)
{
  CURL url(strPath);
  return StringUtils::EqualsNoCase(url.GetProtocol(), "sources");
}

bool URLUtils::IsSpecial(const string& strFile)
{
  string strFile2(strFile);

  // CStackDirectory not implemented (TODO)
  /*
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  */

  return StringUtils::StartsWithNoCase(strFile2, "special:");
}

bool URLUtils::IsStack(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "stack:");
}

bool URLUtils::IsTuxBox(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "tuxbox:");
}

bool URLUtils::IsUPnP(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "upnp:");
}

bool URLUtils::IsURL(const string& strFile)
{
  return strFile.find("://") != string::npos;
}

bool URLUtils::IsVideoDb(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "videodb:");
}

bool URLUtils::IsVTP(const string& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "vtp:");
}

bool URLUtils::IsZIP(const string& strFile) // also checks for comic books!
{
  return HasExtension(strFile, ".zip|.cbz");
}

bool URLUtils::ProtocolHasEncodedFilename(const string& prot)
{
  string prot2 = TranslateProtocol(prot);

  // For now assume only (quasi) http internet streams use URL encoding
  return prot2 == "http"  ||
         prot2 == "https";
}

bool URLUtils::ProtocolHasEncodedHostname(const string& prot)
{
  return ProtocolHasParentInHostname(prot) ||
         StringUtils::EqualsNoCase(prot, "musicsearch") ||
         StringUtils::EqualsNoCase(prot, "image");
}

bool URLUtils::ProtocolHasParentInHostname(const string& prot)
{
  return StringUtils::EqualsNoCase(prot, "zip") ||
         StringUtils::EqualsNoCase(prot, "rar") ||
         StringUtils::EqualsNoCase(prot, "apk") ||
         StringUtils::EqualsNoCase(prot, "bluray") ||
         StringUtils::EqualsNoCase(prot, "udf");
}

bool URLUtils::UpdateUrlEncoding(string &strFilename)
{
  if (strFilename.empty())
    return false;

  CURL url(strFilename);
  // if this is a stack:// URL we need to work with its filename
  if (IsStack(strFilename))
  {
    vector<string> files;
    // stack:// not implemented (TODO)
    /*
    if (!CStackDirectory::GetPaths(strFilename, files))
      return false;
    */

    for (vector<string>::iterator file = files.begin(); file != files.end(); file++)
    {
      string filePath = *file;
      UpdateUrlEncoding(filePath);
      *file = filePath;
    }

    string stackPath;
    // stack:// not implemented (TODO)
    /*
    if (!CStackDirectory::ConstructStackPath(files, stackPath))
      return false;
    */

    url.Parse(stackPath);
  }
  // if the protocol has an encoded hostname we need to work with its hostname
  else if (ProtocolHasEncodedHostname(url.GetProtocol()))
  {
    string hostname = url.GetHostName();
    UpdateUrlEncoding(hostname);
    url.SetHostName(hostname);
  }
  else
    return false;

  string newFilename = url.Get();
  if (newFilename == strFilename)
    return false;

  strFilename = newFilename;
  return true;
}

void URLUtils::GetCommonPath(string& strParent, const string& strPath)
{
  // find the common path of parent and path
  unsigned int j = 1;
  while (j <= min(strParent.size(), strPath.size()) && strnicmp(strParent.c_str(), strPath.c_str(), j) == 0)
    j++;
  strParent.erase(j - 1);
  // they should at least share a / at the end, though for things such as path/cd1 and path/cd2 there won't be
  if (!HasSlashAtEnd(strParent))
  {
    strParent = GetDirectory(strParent);
    AddSlashAtEnd(strParent);
  }
}

void URLUtils::Split(const string& fileNameAndPath, string& path, string& fileName)
{
  path.clear();
  fileName.clear();

  size_t i = fileNameAndPath.length() - 1;
  while (i != 0)
  {
    char ch = fileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1))
      break;
    i--;
  }

  if (i != 0)
  {
    // Take left including the directory separator
    path = fileNameAndPath.substr(0, i + 1);
    // Everything to the right of the directory separator
    fileName = fileNameAndPath.substr(i + 1);
  }
  else
  {
    fileName = fileNameAndPath;
  }
}

string URLUtils::resolvePath(const string &path)
{
  if (path.empty())
    return path;

  size_t posSlash = path.find('/');
  size_t posBackslash = path.find('\\');
  string delim = posSlash < posBackslash ? "/" : "\\";
  vector<string> parts;
  StringUtils::Split(path, delim, parts);
  vector<string> realParts;

  for (vector<string>::const_iterator part = parts.begin(); part != parts.end(); part++)
  {
    if (part->empty() || part->compare(".") == 0)
      continue;

    // go one level back up
    if (part->compare("..") == 0)
    {
      if (!realParts.empty())
        realParts.pop_back();
      continue;
    }

    realParts.push_back(*part);
  }

  string realPath;
  int i = 0;
  // re-add any / or \ at the beginning
  while (path.at(i) == delim.at(0))
  {
    realPath += delim;
    i++;
  }
  // put together the path
  realPath += StringUtils::Join(realParts, delim);
  // re-add any / or \ at the end
  if (path.at(path.size() - 1) == delim.at(0) && realPath.at(realPath.size() - 1) != delim.at(0))
    realPath += delim;

  return realPath;
}

