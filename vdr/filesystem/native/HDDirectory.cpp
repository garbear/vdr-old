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

#include "HDDirectory.h"
#include "filesystem/File.h"
#include "utils/url/URLUtils.h"

#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <vector>

using namespace std;

CHDDirectory::CHDDirectory()
{
}

CHDDirectory::~CHDDirectory()
{
}

bool CHDDirectory::GetDirectory(const string &strPath, DirectoryListing &items)
{
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(strPath.c_str())) != NULL)
  {
    // Read all the files and directories within directory
    while ((ent = readdir(dir)) != NULL)
    {
      string directoryEntry = ent->d_name;
      bool bHidden = false;

#if !defined(TARGET_WINDOWS)
      if (!directoryEntry.empty() && directoryEntry[0] == '.')
        bHidden = true;
#endif

      items.push_back(CDirectoryFileLabel(directoryEntry, bHidden));
    }
    closedir (dir);
    return true;
  }

  // Could not open directory
  return false;
}

bool CHDDirectory::Create(const std::string &strPath)
{
  // TODO: mkdir -p
  int status;
  status = mkdir(strPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  return status == 0;
}

bool CHDDirectory::Exists(const std::string &strPath)
{
  struct __stat64 data;
  if (CFile::Stat(strPath, &data) == 0)
    return S_ISDIR(data.st_mode);
  return false;
}

bool CHDDirectory::Remove(const std::string &strPath)
{
  // TODO: Windows might want to use http://stackoverflow.com/questions/734717/how-to-delete-a-folder-in-c
  int status;
  status = rmdir(strPath.c_str());
  return status == 0;
}

bool CHDDirectory::Rename(const std::string &strPath, const std::string &strNewPath)
{
  int status;
  status = rename(strPath.c_str(), strNewPath.c_str());
  return status == 0;
}
