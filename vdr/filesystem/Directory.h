/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "IDirectory.h"

#include <string>
#include <vector>

class cDirectory
{
public:
  cDirectory() { }
  ~cDirectory() { }

  static bool GetDirectory(const std::string &strPath,
                           cDirectoryListing &items,
                           const std::string &strMask = "",
                           int flags = DEFAULTS);
  // Need to manually delete IDirectory object (TODO: fix this)
  static IDirectory *GetDirectory(const std::string &strPath,
                           IDirectoryCallback *callback,
                           const std::string &strMask = "",
                           int flags = DEFAULTS);

  static bool Create(const std::string &strPath);
  static bool Exists(const std::string &strPath);
  static bool Remove(const std::string &strPath);
  static bool Rename(const std::string &strPath, const std::string &strNewPath);

  static bool CalculateDiskSpace(const std::string &strPath, unsigned int &size, unsigned int &used, unsigned int &free);
  static bool CanWrite(const std::string &strPath);

private:
  static IDirectory *CreateLoader(const std::string &strPath);
};
