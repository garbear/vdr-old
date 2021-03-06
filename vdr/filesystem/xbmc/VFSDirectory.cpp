/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "VFSDirectory.h"
#include "xbmc/libXBMC_addon.h"
#include "xbmc/xbmc_content_types.h"
#include "xbmc/xbmc_file_utils.hpp"
#include "filesystem/SpecialProtocol.h"
#include "utils/log/Log.h"

#include <vector>

using namespace std;

namespace VDR
{

extern ADDON::CHelper_libXBMC_addon *XBMC;

cVFSDirectory::cVFSDirectory()
 : m_XBMC(XBMC)
{
}

bool cVFSDirectory::GetDirectory(const string &strPath, DirectoryListing &items)
{
  CONTENT_ADDON_FILELIST *directory = NULL;
  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(strPath);

  if (m_XBMC->GetDirectory(strTranslatedPath.c_str(), &directory) && directory)
  {
    AddonFileItemList list(*directory);
    m_XBMC->FreeDirectory(directory);

    for (vector<AddonFileItem>::const_iterator it = list.m_fileItems.begin(); it != list.m_fileItems.end(); ++it)
    {
      string path = it->GetPropertyString("path");
      items.push_back(CDirectoryFileLabel(path));
    }
    return true;
  }
  return false;
}

bool cVFSDirectory::Create(const std::string &strPath)
{
  if (!m_XBMC)
    return IDirectory::Create(strPath);

  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(strPath);
  return m_XBMC->CreateDirectory(strTranslatedPath.c_str());
}

bool cVFSDirectory::Exists(const std::string &strPath)
{
  if (!m_XBMC)
    return IDirectory::Exists(strPath);

  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(strPath);
  return m_XBMC->DirectoryExists(strTranslatedPath.c_str());
}

bool cVFSDirectory::Remove(const std::string &strPath)
{
  if (!m_XBMC)
    return IDirectory::Remove(strPath);

  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(strPath);
  return m_XBMC->RemoveDirectory(strTranslatedPath.c_str());
}

bool cVFSDirectory::Rename(const std::string &strPath, const std::string &strNewPath)
{
  if (!m_XBMC)
    return IDirectory::Rename(strPath, strNewPath);

  //XXX
  esyslog("NOT IMPLEMENTED: %s: %s -> %s", __PRETTY_FUNCTION__, strPath.c_str(), strNewPath.c_str());
  return false; // Not implemented in libXBMC API.
}

}
