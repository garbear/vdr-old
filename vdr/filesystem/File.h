/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2002 Frodo
 *      Portions Copyright (C) by the authors of ffmpeg and xvid
 *      Portions Copyright (C) 2002-2013 Team XBMC
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

// File flags from File.h of XBMC project

/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
#define READ_TRUNCATED    0x01
/* indicate that that caller support read in the minimum defined chunk size, this disables internal cache then */
#define READ_CHUNKED      0x02
/* use cache to access this file */
#define READ_CACHED       0x04
/* open without caching. regardless to file type. */
#define READ_NO_CACHE     0x08
/* calcuate bitrate for file while reading */
#define READ_BITRATE      0x10
/* indicate the caller will seek between multiple streams in the file frequently */
#define READ_MULTI_STREAM 0x20

class cFile
{
public:
  cFile();
  virtual ~cFile();

  virtual bool Open(const std::string &url, unsigned int flags = 0);
  virtual bool OpenForWrite(const std::string &url, bool bOverWrite = false);
  virtual int64_t Read(void *lpBuf, int64_t uiBufSize);
  virtual bool ReadLine(std::string &strLine);
  virtual int64_t Write(const void *lpBuf, int64_t uiBufSize);
  virtual void Flush();
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual int64_t Truncate(uint64_t size);
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual void Close();

  virtual std::string GetContent();

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
  virtual bool Rename(const std::string &url, const std::string &urlnew);
  virtual bool SetHidden(const std::string &url, bool hidden);

  //virtual int IoControl(EIoControl request, void* param) { return -1; }

private:
  static IFile *CreateLoader(const std::string &url);

  IFile        *m_pFileImpl;
  unsigned int  m_flags;
  //CFileStreamBuffer* m_pBuffer;
};
