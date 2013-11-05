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

#include "IFile.h"
#include "../../xbmc/xbmc_vfs_utils.hpp"

class cVFSFile : public IFile
{
public:
  cVFSFile();
  virtual ~cVFSFile() { }

  virtual bool Open(const std::string &url, unsigned int flags = 0);
  virtual bool OpenForWrite(const std::string &url, bool bOverWrite = false);
  virtual int64_t Read(void *lpBuf, uint64_t uiBufSize);
  virtual bool ReadLine(std::string &strLine);
  virtual int64_t Write(const void *lpBuf, uint64_t uiBufSize);
  virtual void Flush();
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual int64_t Truncate(uint64_t size);
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual void Close();

  /*!
   * @brief Get the minimum size that can be read from the open file. For example,
   *        CDROM files in which access could be sector-based. It can also be
   *        used to indicate a file system is non-buffered but accepts any read
   *        size, in which case GetChunkSize() should return the value 1.
   * @return The chunk size
   */
  virtual unsigned int GetChunkSize();

  virtual bool Exists(const std::string &url);
  virtual int Stat(const std::string &url, struct __stat64 *buffer);
  virtual bool Delete(const std::string &url);
  virtual bool Rename(const std::string &url, const std::string &urlnew) { return false; }

private:
  VFSFile m_file;
};
