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
#pragma once

#include "Types.h"
#include "utils/url/URL.h"
#include "Config.h"

#include <map>
#include <string>

namespace VDR
{

/*
 * Static class for path translation from our special:// URIs
 *
 * The paths are as follows:
 *
 * special://vdr/         - the main VDR folder (i.e. where the app resides)
 * special://home/        - a writable version of the main VDR folder
 *                            Linux: ~/.vdr/
 *                            OS X:  ~/Library/Application Support/VDR/
 *                            Win32: ~/Application Data/VDR/
 *                            XBMC add-on: special://profile/addon_data/service.vdr/
 * special://temp/        - the temporary directory
 *                            Linux: ~/tmp/vdr<username>
 *                            OS X:  ~/
 *                            Win32: ~/Application Data/VDR/cache
 *
 * When running as an add-on, all other special protocol URIs are forwarded to
 * XBMC via its VFS api. To allow access to paths eclipsed by the above, the
 * following can be used:
 *
 * special://xbmc-home/    - the folder that XBMC maps to special://home/
 * special://xbmc-temp/    - the folder that XBMC maps to special://temp/
 */
class CURL;
class CSpecialProtocol
{
public:
  static bool SetFileBasePath();
  static void SetVDRPath(const std::string &path);
  static void SetHomePath(const std::string &path);
  static void SetTempPath(const std::string &path);

  static bool ComparePath(const std::string &path1, const std::string &path2);
  static void LogPaths();

  static std::string TranslatePath(const std::string &path);
  static std::string TranslatePath(const CURL &url);
  static std::string TranslatePathConvertCase(const std::string& path);

private:
  static std::string GetExecutablePath();
  static std::string GetHomePath(const std::string strEnvVariable = VDR_HOME_ENV_VARIABLE);
  static std::string GetTemporaryPath();

  static void SetPath(const std::string &key, const std::string &path);
  static std::string GetPath(const std::string &key);

  static std::map<std::string, std::string> m_pathMap;
};

#ifdef TARGET_WINDOWS
#define PATH_SEPARATOR_CHAR '\\'
#define PATH_SEPARATOR_STRING "\\"
#else
#define PATH_SEPARATOR_CHAR '/'
#define PATH_SEPARATOR_STRING "/"
#endif

}
