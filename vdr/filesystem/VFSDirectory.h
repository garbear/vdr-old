/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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
#pragma once

#include "IDirectory.h"
#include "../../xbmc/libXBMC_addon.h"

#include <string>

extern ADDON::CHelper_libXBMC_addon *XBMC;

class cVFSDirectory : public IDirectory
{
public:

  /*!
   * @brief Construct a new directory
   * @param XBMC The libXBMC helper instance. Must remain valid during the lifetime of this class.
   */
  cVFSDirectory();
  virtual ~cVFSDirectory() { }

  /*!
   * \brief Get the contents of a directory
   * \param strPath Directory to read
   * \param items The directory entries retrieved by GetDirectory()
   * \return true on success
   */
  virtual bool GetDirectory(const std::string &strPath, cDirectoryListing &items);

  /*!
  * \brief Create the directory
  * \param strPath Directory to create
  * \return true if directory is created or already exists
  */
  virtual bool Create(const std::string &strPath);

  /*!
  * \brief Check if the directory exists
  * \param strPath Directory to check
  * \return true if directory exists
  */
  virtual bool Exists(const std::string &strPath);

  /*!
  * \brief Removes the directory
  * \param strPath Directory to remove
  * \return true on success
  */
  virtual bool Remove(const std::string &strPath);

  /*!
  * \brief Renames the directory
  * \param strPath Directory to rename
  * \param strNewPath Target directory name
  * \return true on success
  */
  virtual bool Rename(const std::string &strPath, const std::string &strNewPath);

  void SetMask(const std::string &strMask);
  void SetFlags(int flags);

private:
  ADDON::CHelper_libXBMC_addon* m_XBMC;
};
