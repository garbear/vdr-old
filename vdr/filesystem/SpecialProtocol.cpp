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

#include <limits.h>
#include <assert.h>

using namespace std;

#define VDR_ROOT        "vdr"
#define HOME_ROOT       "home"
#define TEMP_ROOT       "temp"
#define XBMCHOME_ROOT   "xbmc-home"
#define XBMCTEMP_ROOT   "xbmc-temp"

#define ADDON_PROFILE   "special://profile/addon_data/service.vdr/"

map<string, string> CSpecialProtocol::m_pathMap;

std::string CSpecialProtocol::GetExecutablePath()
{
  std::string strExecutablePath;

#ifdef TARGET_WINDOWS

  static const size_t bufSize = MAX_PATH * 2;
  wchar_t* buf = new wchar_t[bufSize];
  buf[0] = 0;
  ::GetModuleFileNameW(0, buf, bufSize);
  buf[bufSize-1] = 0;
  g_charsetConverter.wToUTF8(buf, strExecutablePath);
  delete[] buf;

#elif defined(TARGET_DARWIN)

  char     given_path[2*MAXPATHLEN];
  uint32_t path_size =2*MAXPATHLEN;

  GetDarwinExecutablePath(given_path, &path_size);
  strExecutablePath = given_path;

#elif defined(TARGET_FREEBSD)

  char buf[PATH_MAX];
  size_t buflen;
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = getpid();

  buflen = sizeof(buf) - 1;
  if (sysctl(mib, 4, buf, &buflen, NULL, 0) < 0)
    strExecutablePath.clear(); // TODO: This seems wrong
  else
    strExecutablePath = buf;

#else

  /* Get our PID and build the name of the link in /proc */
  pid_t pid = getpid();
  char linkname[64]; /* /proc/<pid>/exe */
  snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);

  /* Now read the symbolic link */
  char buf[PATH_MAX + 1];
  buf[0] = 0;

  int ret = readlink(linkname, buf, sizeof(buf) - 1);
  if (ret != -1)
    buf[ret] = 0;

  strExecutablePath = buf;

#endif

  return strExecutablePath;
}

std::string CSpecialProtocol::GetHomePath(const std::string strEnvVariable /* = VDR_HOME_ENV_VARIABLE */)
{
  //std::string strPath = CEnvironment::getenv(strEnvVariable); // TODO
  std::string strPath;

  // TODO
  /*
#ifdef TARGET_WINDOWS
  if (strPath.find("..") != std::string::npos)
  {
    //expand potential relative path to full path
    CStdStringW strPathW;
    g_charsetConverter.utf8ToW(strPath, strPathW, false);
    CWIN32Util::AddExtraLongPathPrefix(strPathW);
    const unsigned int bufSize = GetFullPathNameW(strPathW, 0, NULL, NULL);
    if (bufSize != 0)
    {
      wchar_t * buf = new wchar_t[bufSize];
      if (GetFullPathNameW(strPathW, bufSize, buf, NULL) <= bufSize-1)
      {
        std::wstring expandedPathW(buf);
        CWIN32Util::RemoveExtraLongPathPrefix(expandedPathW);
        g_charsetConverter.wToUTF8(expandedPathW, strPath);
      }

      delete [] buf;
    }
  }
#endif
  */

  if (strPath.empty())
  {
    // TODO
    /*
#if defined(TARGET_DARWIN)
    int      result = -1;
    char     given_path[2*MAXPATHLEN];
    uint32_t path_size =2*MAXPATHLEN;

    result = GetDarwinExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // Move backwards to last /.
      for (int n=strlen(given_path)-1; given_path[n] != '/'; n--)
        given_path[n] = '\0';

      #if defined(TARGET_DARWIN_IOS)
        strcat(given_path, "/VDRData/VDRHome/");
      #else
        // Assume local path inside application bundle.
        strcat(given_path, "../Resources/VDR/");
      #endif

      // Convert to real path.
      char real_path[2*MAXPATHLEN];
      if (realpath(given_path, real_path) != NULL)
        return real_path;
    }
#endif
    */

    std::string strHomePath = GetExecutablePath();
    size_t last_sep = strHomePath.find_last_of(PATH_SEPARATOR_CHAR);
    if (last_sep != std::string::npos)
      strPath = strHomePath.substr(0, last_sep);
    else
      strPath = strHomePath;

    // CMake uses a build directory
    if (StringUtils::CompareNoCase(URLUtils::GetFileName(strPath), "build") == 0)
      strPath = URLUtils::GetDirectory(strPath);
  }

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  /* Change strPath accordingly when target is VDR_HOME and when INSTALL_PATH
   * and BIN_INSTALL_PATH differ
   */
  std::string installPath = INSTALL_PATH;
  std::string binInstallPath = BIN_INSTALL_PATH;
  if (!strEnvVariable.compare("VDR_HOME") && installPath.compare(binInstallPath))
  {
    int pos = strPath.length() - binInstallPath.length();
    std::string tmp = strPath;
    tmp.erase(0, pos);
    if (!tmp.compare(binInstallPath))
    {
      strPath.erase(pos, strPath.length());
      strPath.append(installPath);
    }
  }
#endif

  return strPath;
}

bool CSpecialProtocol::SetFileBasePath()
{
  string vdrPath = GetHomePath();
  if (vdrPath.empty())
  {
    esyslog("Failed to find the base path");
    return false;
  }

  /* Set vdr path */
  SetVDRPath(vdrPath);

#ifdef TARGET_XBMC
  SetHomePath(ADDON_PROFILE);
#else
  SetHomePath(vdrPath); // TODO
#endif

  string tempPath = "special://temp"; // TODO: GetTempPath()
  if (tempPath.empty())
  {
    esyslog("Failed to determine the temp path");
    return false;
  }
  //SetTempPath(tempPath);

  SetXBMCHomePath("special://home");
  SetXBMCTempPath("special://temp");

  return true;
}

void CSpecialProtocol::SetVDRPath(const string &dir)
{
  SetPath(VDR_ROOT, dir);
}

void CSpecialProtocol::SetHomePath(const string &dir)
{
  SetPath(HOME_ROOT, dir);
}

void CSpecialProtocol::SetTempPath(const string &dir)
{
  SetPath(TEMP_ROOT, dir);
}

void CSpecialProtocol::SetXBMCHomePath(const string &dir)
{
  SetPath(XBMCHOME_ROOT, dir);
}

void CSpecialProtocol::SetXBMCTempPath(const string &dir)
{
  SetPath(XBMCTEMP_ROOT, dir);
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
  /*else*/ if (RootDir == VDR_ROOT || // TODO: this used to be an "else if"
           RootDir == HOME_ROOT ||
           RootDir == TEMP_ROOT ||
           RootDir == XBMCHOME_ROOT ||
           RootDir == XBMCTEMP_ROOT)
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
  isyslog("special://" VDR_ROOT "/ is mapped to: %s", GetPath(VDR_ROOT).c_str());
  isyslog("special://" HOME_ROOT "/ is mapped to: %s", GetPath(HOME_ROOT).c_str());
  isyslog("special://" TEMP_ROOT "/ is mapped to: %s", GetPath(TEMP_ROOT).c_str());
  isyslog("special://" XBMCHOME_ROOT "/ is mapped to: %s", GetPath(XBMCHOME_ROOT).c_str());
  isyslog("special://" XBMCTEMP_ROOT "/ is mapped to: %s", GetPath(XBMCTEMP_ROOT).c_str());
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
