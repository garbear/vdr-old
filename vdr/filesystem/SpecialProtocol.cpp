/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/url/URL.h"
#include "utils/url/URLUtils.h"
//#include "Util.h"
//#include "guilib/GraphicContext.h"
//#include "profiles/ProfilesManager.h"
//#include "settings/AdvancedSettings.h"
//#include "settings/Settings.h"
//#include "utils/log.h"

//#ifdef TARGET_POSIX
//#include <dirent.h>
//#endif

#include <assert.h>

using namespace std;

map<string, string> CSpecialProtocol::m_pathMap;

void CSpecialProtocol::SetVDRPath(const string &dir)
{
  SetPath("vdr", dir);
}

void CSpecialProtocol::SetHomePath(const string &dir)
{
  SetPath("home", dir);
}

void CSpecialProtocol::SetTempPath(const string &dir)
{
  SetPath("temp", dir);
}

void CSpecialProtocol::SetXBMCHomePath(const string &dir)
{
  SetPath("xbmchome", dir);
}

void CSpecialProtocol::SetXBMCTempPath(const string &dir)
{
  SetPath("xbmctemp", dir);
}

bool CSpecialProtocol::ComparePath(const string &path1, const string &path2)
{
  return TranslatePath(path1) == TranslatePath(path2);
}

string CSpecialProtocol::TranslatePath(const string &path)
{
  // This wrapper is necessary because calling TranslatePath(char*) fails to
  // do double-implicit construction (from char* to std::string, and then from
  // std::string to CURL)
  return TranslatePath(CURL(path));
}

string CSpecialProtocol::TranslatePath(const CURL &url)
{
  // check for special-protocol, if not, return
  if (!StringUtils::EqualsNoCase(url.GetProtocol(), "special"))
    return url.Get();

  string FullFileName = url.GetFileName();

  string RootDir;
  string FileName;
  string translatedPath;

  // Split up into the root and the rest of the filename
  size_t pos = FullFileName.find('/');
  if (pos != string::npos && pos > 1)
  {
    RootDir = FullFileName.substr(0, pos);

    if (pos < FullFileName.length())
      FileName = FullFileName.substr(pos + 1);
  }
  else
    RootDir = FullFileName;

  StringUtils::ToLower(RootDir);

  // Don't support any of these protocols (TODO)
  /*
  if (StringUtils::EqualsNoCase(RootDir, "subtitles"))
    translatedPath = URLUtils::AddFileToFolder(CSettings::Get().GetString("subtitles.custompath"), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "userdata"))
    translatedPath = URLUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "database"))
    translatedPath = URLUtils::AddFileToFolder(CProfilesManager::Get().GetDatabaseFolder(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "thumbnails"))
    translatedPath = URLUtils::AddFileToFolder(CProfilesManager::Get().GetThumbnailsFolder(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "recordings") || StringUtils::EqualsNoCase(RootDir, "cdrips"))
    translatedPath = URLUtils::AddFileToFolder(CSettings::Get().GetString("audiocds.recordingpath"), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "screenshots"))
    translatedPath = URLUtils::AddFileToFolder(CSettings::Get().GetString("debug.screenshotpath"), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "musicplaylists"))
    translatedPath = URLUtils::AddFileToFolder(CUtil::MusicPlaylistsLocation(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "videoplaylists"))
    translatedPath = URLUtils::AddFileToFolder(CUtil::VideoPlaylistsLocation(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "skin"))
    translatedPath = URLUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "logpath"))
    translatedPath = URLUtils::AddFileToFolder(g_advancedSettings.m_logFolder, FileName);
  */

  //

  // From here on, we have our "real" special paths
  /*else*/ if (RootDir == "vdr" || // TODO: this used to be an "else if"
           RootDir == "home" ||
           RootDir == "temp" ||
           RootDir == "xbmchome" ||
           RootDir == "xbmctemp")
  {
    string basePath = GetPath(RootDir);
    if (!basePath.empty())
      translatedPath = URLUtils::AddFileToFolder(basePath, FileName);
    else
      translatedPath.clear();
  }

  // Don't recurse in - this allows special://xbmchome to map to special://home
  // and invoke the use of a VFS file. Otherwise, the URL gets incorrectly
  // resolved to VDR's home folder.

  // Validate the final path, just in case
  return URLUtils::ValidatePath(translatedPath);
}

string CSpecialProtocol::TranslatePathConvertCase(const string& path)
{
  string translatedPath = TranslatePath(path);

#if defined(TARGET_POSIX) && false // TODO
  if (translatedPath.find("://") != string::npos)
    return translatedPath;

  // If the file exists with the requested name, simply return it
  struct stat stat_buf;
  if (stat(translatedPath.c_str(), &stat_buf) == 0)
    return translatedPath;

  string result;
  vector<string> tokens;
  StringUtils::Tokenize(translatedPath, tokens, "/");
  string file;
  DIR* dir;
  struct dirent* de;

  for (unsigned int i = 0; i < tokens.size(); i++)
  {
    file = result + "/";
    file += tokens[i];
    if (stat(file.c_str(), &stat_buf) == 0)
    {
      result += "/" + tokens[i];
    }
    else
    {
      dir = opendir(result.c_str());
      if (dir)
      {
        while ((de = readdir(dir)) != NULL)
        {
          // check if there's a file with same name but different case
          if (strcasecmp(de->d_name, tokens[i].c_str()) == 0)
          {
            result += "/";
            result += de->d_name;
            break;
          }
        }

        // if we did not find any file that somewhat matches, just
        // fallback but we know it's not gonna be a good ending
        if (de == NULL)
          result += "/" + tokens[i];

        closedir(dir);
      }
      else
      { // this is just fallback, we won't succeed anyway...
        result += "/" + tokens[i];
      }
    }
  }

  return result;
#else
  return translatedPath;
#endif
}

void CSpecialProtocol::LogPaths()
{
  /* TODO
  CLog::Log(LOGNOTICE, "special://xbmc/ is mapped to: %s", GetPath("xbmc").c_str());
  CLog::Log(LOGNOTICE, "special://xbmcbin/ is mapped to: %s", GetPath("xbmcbin").c_str());
  CLog::Log(LOGNOTICE, "special://masterprofile/ is mapped to: %s", GetPath("masterprofile").c_str());
  CLog::Log(LOGNOTICE, "special://home/ is mapped to: %s", GetPath("home").c_str());
  CLog::Log(LOGNOTICE, "special://temp/ is mapped to: %s", GetPath("temp").c_str());
  //CLog::Log(LOGNOTICE, "special://userhome/ is mapped to: %s", GetPath("userhome").c_str());
  if (!CUtil::GetFrameworksPath().empty())
    CLog::Log(LOGNOTICE, "special://frameworks/ is mapped to: %s", GetPath("frameworks").c_str());
  */
}

// private routines, to ensure we only set/get an appropriate path
void CSpecialProtocol::SetPath(const string &key, const string &path)
{
  m_pathMap[key] = path;
}

string CSpecialProtocol::GetPath(const string &key)
{
  map<string, string>::iterator it = m_pathMap.find(key);
  assert(it != m_pathMap.end());
  return it->second;
}
