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
#pragma once

#include "filesystem/IDirectory.h"

#include <string>

namespace VDR
{
class CHDDirectory : public IDirectory
{
public:
  CHDDirectory();
  virtual ~CHDDirectory();

  virtual bool GetDirectory(const std::string &strPath, DirectoryListing &items);
  //virtual void GetDirectoryAsync(const std::string &strPath, IDirectoryCallback *callback);
  //virtual void CancelDirectory();
  virtual bool Create(const std::string &strPath);
  virtual bool Exists(const std::string &strPath);
  virtual bool Remove(const std::string &strPath);
  virtual bool Rename(const std::string &strPath, const std::string &strNewPath);
  virtual bool DiskSpace(const std::string &strPath, disk_space_t& space);

protected:
  /*!
   * \brief Return a new CDirectoryFetchJob (allows subclasses to override)
   *
   * The default job is to simply run GetDirectory() in another thread.
   *
   * \param strPath The path to fetch
   * \param callback The callback to invoke on success or failure
   * \return The newly-allocated job. Deallocation is the responsibility of IDirectory
   */
  //virtual CDirectoryFetchJob *GetJob(const std::string &strPath, IDirectoryCallback *callback);
};
}
