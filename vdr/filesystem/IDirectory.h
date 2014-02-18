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

#include <string>
#include <vector>

// TODO
class CDirectoryFileLabel
{
public:
  CDirectoryFileLabel(const std::string &strName, bool bHidden = false)
   : m_strName(strName),
     m_bHidden(bHidden)
  {
  }

  const std::string &Name() const { return m_strName; }
  bool Hidden() const { return m_bHidden; }

private:
  std::string m_strName;
  bool        m_bHidden;
};

//typedef std::vector<CDirectoryFileLabel> DirectoryListing;
/*
 * \brief DirectoryListing is a vector of file labels with an additional
 *        property, PATH, that specifies the path of the directory being listed.
 */
class DirectoryListing : public std::vector<CDirectoryFileLabel>
{
public:
  std::string PATH;
};

class IDirectoryCallback
{
public:
  virtual void OnDirectoryFetchComplete(bool bSuccess, const DirectoryListing &items) = 0;

  virtual ~IDirectoryCallback() { }
};

class CDirectoryFetchJob;

/*!
 * \brief Available directory flags
 *
 * The defaults are to allow file directories, no prompting, retrieve file
 * information, hide hidden files, and utilise the directory cache.
 *
 */
enum DIR_FLAG
{
  DEFAULTS      = 0,
  NO_FILE_INFO  = (1 << 0), // Don't read additional file info (stat for example)
  GET_HIDDEN    = (1 << 1), // Get hidden files
  READ_CACHE    = (1 << 2), // Force reading from the directory cache (if available)
};

/*!
 * \brief Interface to the directory on a file system
 */
class IDirectory
{
public:
  IDirectory() : m_flags(DEFAULTS), m_fetchJob(NULL) { }
  virtual ~IDirectory();

  /*!
   * \brief Get the contents of a directory (synchronous)
   * \param strPath Directory to read
   * \param items The directory entries retrieved by GetDirectory()
   * \return true on success
   */
  virtual bool GetDirectory(const std::string &strPath, DirectoryListing &items) = 0;

  /*!
   * \brief Get the contents of a directory (asynchronous)
   *
   * This function defaults to calling the GetDirectory() above in another thread.
   *
   * \param strPath Directory to read
   * \param callback The callback to invoke off-thread when directory fetch is
   *        complete. Must remain valid over the lifetime of this object.
   */
  virtual void GetDirectoryAsync(const std::string &strPath, IDirectoryCallback *callback);

  /*!
   * \brief Retrieve the progress of the current directory fetch (if possible)
   * \return the progress as a percentage in the range 0..100
   */
  unsigned int GetProgress() const;

  /*!
   * \brief Cancel the current directory fetch (if possible)
   */
  virtual void CancelDirectory();

  /*!
  * \brief Create the directory
  * \param strPath Directory to create
  * \return true if directory is created or already exists
  */
  virtual bool Create(const std::string &strPath) { return false; }

  /*!
  * \brief Check if the directory exists
  * \param strPath Directory to check
  * \return true if directory exists
  */
  virtual bool Exists(const std::string &strPath) { return false; }

  /*!
   * Get disk space info for this directory
   * @param strPath The path to get the info for
   * @param size Current size
   * @param used Current bytes in use
   * @param free Free bytes on this disk
   * @return True when fetched successfully
   */
  virtual bool DiskSpace(const std::string &strPath, unsigned int &size, unsigned int &used, unsigned int &free) { return false; }

  /*!
  * \brief Removes the directory
  * \param strPath Directory to remove
  * \return true on success
  */
  virtual bool Remove(const std::string &strPath) { return false; }

  /*!
  * \brief Renames the directory
  * \param strPath Directory to rename
  * \param strNewPath Target directory name
  * \return true on success
  */
  virtual bool Rename(const std::string &strPath, const std::string &strNewPath) { return false; }

  int GetFlags() const { return m_flags; }
  void SetFlags(int flags) { m_flags = flags; }

  const std::string &GetMask() const { return m_strFileMask; }
  void SetMask(const std::string &strMask) { m_strFileMask = strMask; }

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
  virtual CDirectoryFetchJob *GetJob(const std::string &strPath, IDirectoryCallback *callback);

private:
  // Directory flags - see DIR_FLAG
  int m_flags;

  // Holds the file mask specified by SetMask()
  std::string m_strFileMask;

  CDirectoryFetchJob *m_fetchJob;
};
