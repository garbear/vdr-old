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
#pragma once

#include "Types.h"
#include "URL.h"

#include <string>
#include <vector>

/*
 * URLUtils: tools to modify URLs
 */
class URLUtils
{
public:
  // Functional string methods

  static std::string AddFileToFolder(const std::string &strFolder, const std::string &strFile);
  static std::string CreateArchivePath(const std::string& strType, const std::string& strArchivePath, const std::string& strFilePathInArchive, const std::string& strPwd="");
  static std::string Decode(const std::string& strURLData);
  static std::string Encode(const std::string& strURLData);
  static std::string FixSlashesAndDups(const std::string& uri, const char slashCharacter = '/', const size_t startFrom = 0);
  static std::string GetDirectory(const std::string &uri);
  static std::string GetExtension(const std::string& uri); // Returns filename extension including period of filename
  static std::string GetFileName(const std::string& strFileNameAndPath); // Returns a filename given an url. Handles both / and \, and options in urls
  static std::string GetParentPath(const std::string& strPath);
  /*!
   * \brief Cleans up the given path by resolving "." and ".."
   * and returns it.
   *
   * This methods goes through the given path and removes any "."
   * (as it states "this directory") and resolves any ".." by
   * removing the previous directory from the path. This is done
   * for file paths and host names (in case of VFS paths).
   *
   * \param path Path to be cleaned up
   * \return Actual path without any "." or ".."
   */
  static std::string GetRealPath(const std::string &path);
  static std::string GetRedacted(const std::string& path);
  static std::string GetWithSlashAtEnd(const std::string& uri);
  static std::string ReplaceExtension(const std::string& uri, const std::string& newExtension);
  static std::vector<std::string> SplitPath(const std::string& strPath);
  static std::string SubstitutePath(const std::string& strPath, bool reverse = false);
  static std::string TranslateProtocol(const std::string& prot);
  static std::string ValidatePath(const std::string &path, bool bFixDoubleSlashes = false); // Return a validated path, with correct directory separators

  // Imperative string methods

  static void AddSlashAtEnd(std::string& uri);
  static void GetCommonPath(std::string& strPath, const std::string& strPath2);
  static void RemoveSlashAtEnd(std::string& uri);
  // Splits a full filename in path and file
  // ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  // Trailing slash will be preserved
  static void Split(const std::string& fileNameAndPath, std::string& path, std::string& fileName);

  // Predicate methods

  static bool Compare(const std::string& uri1, const std::string& uri2);
  static bool GetParentPath(const std::string& strPath, std::string& strParent);
  /*
   * \brief Check if filename have any of the listed extensions
   * \param strFileName Path or URL to check
   * \param strExtensions List of '.' prefixed lowercase extensions seperated with '|'
   * \return \e true if strFileName have any one of the extensions.
   * \note The check is case insensitive for strFileName, but requires
   *       strExtensions to be lowercase. Returns false when strFileName or
   *       strExtensions is empty.
   */
  static bool HasExtension(const std::string &uri, const std::string &extensions = "");
  static bool HasSlashAtEnd(const std::string& strFile, bool checkURL = false);
  static bool IsAddonsPath(const std::string& strFile);
  static bool IsAfp(const std::string& strFile);
  static bool IsAndroidApp(const std::string& strFile);
  static bool IsAPK(const std::string& strFile);
  static bool IsArchive(const std::string& strFile);
  static bool IsBluray(const std::string& strFile);
  static bool IsCDDA(const std::string& strFile);
  static bool IsDAAP(const std::string& strFile);
  static bool IsDAV(const std::string& strFile);
  static bool IsDOSPath(const std::string &path);
  static bool IsDVD(const std::string& strFile);
  static bool IsFileOnly(const std::string &url); // Return true if there are no directories in the url
  static bool IsFTP(const std::string& strFile);
  static bool IsFullPath(const std::string &url); // Return true if the url includes the full path
  static bool IsHD(const std::string& strFileName);
  static bool IsHDHomeRun(const std::string& strFile);
  static bool IsHostOnLAN(const std::string& hostName, bool offLineCheck = false);
  static bool IsHTSP(const std::string& strFile);
  static bool IsInAPK(const std::string& strFile);
  static bool IsInArchive(const std::string& strFile);
  static bool IsInPath(const std::string &uri, const std::string &baseURI); // Is baseURI In uri Path
  static bool IsInRAR(const std::string& strFile);
  static bool IsInternetStream(const CURL& url, bool bStrictCheck = false);
  static bool IsInZIP(const std::string& strFile);
  static bool IsISO9660(const std::string& strFile);
  static bool IsLibraryFolder(const std::string& strFile);
  static bool IsLiveTV(const std::string& strFile);
  static bool IsMultiPath(const std::string& strPath);
  static bool IsMusicDb(const std::string& strFile);
  static bool IsMythTV(const std::string& strFile);
  static bool IsNfs(const std::string& strFile);
  static bool IsOnDVD(const std::string& strFile);
  static bool IsOnLAN(const std::string& strFile);
  static bool IsPlugin(const std::string& strFile);
  static bool IsPVRRecording(const std::string& strFile);
  static bool IsRAR(const std::string& strFile);
  static bool IsRemote(const std::string& strFile);
  static bool IsScript(const std::string& strFile);
  static bool IsSlingbox(const std::string& strFile);
  static bool IsSmb(const std::string& strFile);
  static bool IsSourcesPath(const std::string& strFile);
  static bool IsSpecial(const std::string& strFile);
  static bool IsStack(const std::string& strFile);
  static bool IsTuxBox(const std::string& strFile);
  static bool IsUPnP(const std::string& strFile);
  static bool IsURL(const std::string& uri);
  static bool IsVideoDb(const std::string& strFile);
  static bool IsVTP(const std::string& strFile);
  static bool IsZIP(const std::string& strFile);
  static bool ProtocolHasEncodedFilename(const std::string& prot);
  static bool ProtocolHasEncodedHostname(const std::string& prot);
  static bool ProtocolHasParentInHostname(const std::string& prot);
  /*!
   * \brief Updates the URL encoded hostname of the given path
   *
   * This method must only be used to update paths encoded with
   * the old (Eden) URL encoding implementation to the new (Frodo)
   * URL encoding implementation (which does not URL encode -_.!().
   *
   * \param strFilename Path to update
   * \return True if the path has been updated/changed otherwise false
   */
  static bool UpdateUrlEncoding(std::string &strFilename);

private:
  static std::string resolvePath(const std::string &path);
};
