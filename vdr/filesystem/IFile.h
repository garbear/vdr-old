/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2002 Frodo
 *      Portions Copyright (C) by the authors of ffmpeg and xvid
 *      Portions Copyright (C) 2002-2013 Team XBMC
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

#include "../../platform/os.h"

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>

// File flags from File.h of XBMC project

// Indicate that caller can handle truncated reads, where function returns before entire buffer has been filled
#define READ_TRUNCATED     0x01
// Indicate that that caller support read in the minimum defined chunk size, this disables internal cache then
#define READ_CHUNKED       0x02
// Use cache to access this file
#define READ_CACHED        0x04
// Open without caching. regardless to file type.
#define READ_NO_CACHE      0x08
// Calcuate bitrate for file while reading
#define READ_BITRATE       0x10
// Indicate the caller will seek between multiple streams in the file frequently
#define READ_MULTI_STREAM  0x20

/*
struct SNativeIoControl
{
  unsigned long int   request;
  void*               param;
};

struct SCacheStatus
{
  uint64_t forward;  // number of bytes cached forward of current position
  unsigned maxrate;  // maximum number of bytes per second cache is allowed to fill
  unsigned currate;  // average read rate from source file since last position change
  bool     full;     // is the cache full
};

enum EIoControl {
  IOCTRL_NATIVE        = 1, // SNativeIoControl structure, containing what should be passed to native ioctrl
  IOCTRL_SEEK_POSSIBLE = 2, // return 0 if known not to work, 1 if it should work
  IOCTRL_CACHE_STATUS  = 3, // SCacheStatus structure
  IOCTRL_CACHE_SETRATE = 4, // unsigned int with speed limit for caching in bytes per second
  IOCTRL_SET_CACHE     = 8, // CFileCache
};
*/

class IFile
{
public:
  virtual ~IFile() { }

  virtual bool Open(const std::string &url, unsigned int flags = 0) = 0;
  virtual bool OpenForWrite(const std::string &url, bool bOverWrite = false) { return false; }
  virtual int64_t Read(void *lpBuf, uint64_t uiBufSize) = 0;
  virtual bool ReadLine(std::string &strLine) { return false; }
  virtual int64_t Write(const void *lpBuf, uint64_t uiBufSize) = 0;
  virtual void Flush() { }
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) = 0;
  virtual int64_t Truncate(uint64_t size) { return -1; }
  virtual int64_t GetPosition() = 0;
  virtual int64_t GetLength() = 0;
  virtual void Close() = 0;

  virtual std::string GetContent() { return "application/octet-stream"; }
  virtual std::string GetContentCharset() { return ""; }

  /*!
   * @brief Get the minimum size that can be read from the open file. For example,
   *        CDROM files in which access could be sector-based. It can also be
   *        used to indicate a file system is non-buffered but accepts any read
   *        size, in which case GetChunkSize() should return the value 1.
   * @return The chunk size
   */
  virtual unsigned int GetChunkSize() { return 0; }

  virtual bool Exists(const std::string &url) = 0;
  virtual int Stat(const std::string &url, struct __stat64 *buffer)
  {
    if (buffer)
      memset(buffer, 0, sizeof(struct __stat64));
    return -1;
  }
  virtual bool Delete(const std::string &url) { return false; }
  virtual bool Rename(const std::string &url, const std::string &urlnew) { return false; }
  virtual bool SetHidden(const std::string &url, bool hidden) { return false; }

  //virtual int IoControl(EIoControl request, void* param) { return -1; }
};
