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

#include "Directory.h"
#include "VFSDirectory.h"

#include <auto_ptr.h>

using namespace std;

IDirectory *cDirectory::CreateLoader(const std::string &strPath)
{
  bool isXBMC = true;
  if (isXBMC)
    return new cVFSDirectory();
  return NULL;
}

bool cDirectory::GetDirectory(const string &strPath, cDirectoryListing &items, const string &strMask, int flags)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CreateLoader(strPath));
    if (!pDirectory.get())
      return false;
    pDirectory->SetFlags(flags);
    pDirectory->SetMask(strMask);
    bool result = pDirectory->GetDirectory(strPath, items);

    // Now filter for allowed files
    for (size_t i = 0; i < items.size(); i++)
    {
      cDirectoryFileLabel &label = items[i];
      bool isHidden = false;
      if (isHidden && !(flags & GET_HIDDEN))
      {
        items.erase(items.begin() + i);
        i--;
      }

      return result;
    }
  }
  catch (...)
  {
    // Log
  }
  return false;
}

IDirectory *cDirectory::GetDirectory(const string &strPath, IDirectoryCallback *callback, const string &strMask, int flags)
{
  IDirectory *pDirectory = CreateLoader(strPath);
  if (!pDirectory)
    return NULL;
  pDirectory->SetFlags(flags);
  pDirectory->SetMask(strMask);
  pDirectory->GetDirectoryAsync(strPath, callback);
  return pDirectory;
}

bool cDirectory::Create(const string &strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CreateLoader(strPath));
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

bool cDirectory::Exists(const string &strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CreateLoader(strPath));
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

bool cDirectory::Remove(const string &strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CreateLoader(strPath));
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

bool cDirectory::Rename(const string &strPath, const string &strNewPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CreateLoader(strPath));
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
