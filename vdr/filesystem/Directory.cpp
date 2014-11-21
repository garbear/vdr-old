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

#include "Directory.h"
#include "native/HDDirectory.h"
#include "SpecialProtocol.h"
#include "utils/StringUtils.h"

#ifdef TARGET_XBMC
#include "xbmc/VFSDirectory.h"
#endif

#include <memory>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

using namespace std;

namespace VDR
{

IDirectory *CDirectory::CreateLoader(const std::string &path, bool bForceLocal /* = false */)
{
  if (!bForceLocal)
  {
#if TARGET_XBMC
  return new cVFSDirectory();
#endif
  }

  string translatedPath = CSpecialProtocol::TranslatePath(path);
  CURL url(translatedPath);
  string protocol = url.GetProtocol();
  StringUtils::ToLower(protocol); // TODO: See File.cpp

  if (protocol == "file" || protocol.empty())
    return new CHDDirectory();

  return NULL;
}

bool CDirectory::GetDirectory(const string &strPath, DirectoryListing &items, const string &strMask, int flags, bool bForceLocal /* = false */)
{
  try
  {
    std::shared_ptr<IDirectory> pDirectory(CreateLoader(strPath, bForceLocal));
    if (!pDirectory.get())
      return false;
    pDirectory->SetFlags(flags);
    pDirectory->SetMask(strMask);
    bool result = pDirectory->GetDirectory(strPath, items);

    // Now filter for allowed files
    for (size_t i = 0; i < items.size(); i++)
    {
      CDirectoryFileLabel &label = items[i];
      bool isHidden = false;
      if (isHidden && !(flags & GET_HIDDEN))
      {
        items.erase(items.begin() + i);
        i--;
      }
    }

    items.PATH = strPath;
    return result;
  }
  catch (...)
  {
    // Log
  }
  return false;
}

IDirectory *CDirectory::GetDirectory(const string &strPath, IDirectoryCallback *callback, const string &strMask, int flags)
{
  IDirectory *pDirectory = CreateLoader(strPath);
  if (!pDirectory)
    return NULL;
  pDirectory->SetFlags(flags);
  pDirectory->SetMask(strMask);
  pDirectory->GetDirectoryAsync(strPath, callback);
  return pDirectory;
}

bool CDirectory::Create(const string &strPath)
{
  try
  {
    std::shared_ptr<IDirectory> pDirectory(CreateLoader(strPath));
    if (!pDirectory.get())
      return false;
    return pDirectory->Create(strPath);
  }
  catch (...)
  {
    // Log
  }
  return false;
}

bool CDirectory::Exists(const string &strPath)
{
  try
  {
    std::shared_ptr<IDirectory> pDirectory(CreateLoader(strPath));
    if (!pDirectory.get())
      return false;
    return pDirectory->Exists(strPath);
  }
  catch (...)
  {
    // Log
  }
  return false;
}

bool CDirectory::Remove(const string &strPath)
{
  try
  {
    std::shared_ptr<IDirectory> pDirectory(CreateLoader(strPath));
    if (!pDirectory.get())
      return false;
    return pDirectory->Remove(strPath);
  }
  catch (...)
  {
    // Log
  }
  return false;
}

bool CDirectory::Rename(const string &strPath, const string &strNewPath)
{
  try
  {
    std::shared_ptr<IDirectory> pDirectory(CreateLoader(strPath));
    if (!pDirectory.get())
      return false;
    return pDirectory->Rename(strPath, strNewPath);
  }
  catch (...)
  {
    // Log
  }
  return false;
}

bool CDirectory::CalculateDiskSpace(const string &strPath, disk_space_t& space)
{
  try
  {
    std::shared_ptr<IDirectory> pDirectory(CreateLoader(strPath));
    if (!pDirectory.get())
      return false;
    return pDirectory->DiskSpace(strPath, space);
  }
  catch (...)
  {
    // Log
  }
  return false;
}

bool CDirectory::CanWrite(const string &strPath)
{
  struct stat64 ds;
  if (stat64(strPath.c_str(), &ds) == 0)
  {
    if (S_ISDIR(ds.st_mode))
    {
      if (access(strPath.c_str(), R_OK | W_OK | X_OK) == 0)
        return true;
      return false; // Can't access directory
    }
    return false; // Not a directory
  }
  return false;
}

}
