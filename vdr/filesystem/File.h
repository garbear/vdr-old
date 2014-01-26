/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include <stdint.h>
#include <sys/types.h>
#include <vector>
#include <stdio.h>

#define MIME_TYPE_NONE     "none/none"

class CFile
{
public:
  CFile();
  ~CFile();

  bool Open(const std::string &url, unsigned int flags = 0);
  bool OpenForWrite(const std::string &url, bool bOverWrite = false);
  int64_t Read(void *lpBuf, int64_t uiBufSize);
  bool ReadLine(std::string &strLine);
  bool LoadFile(const std::string &url, std::vector<uint8_t> &outputBuffer);
  int64_t Write(const void *lpBuf, int64_t uiBufSize);
  void Flush();
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  int64_t Truncate(uint64_t size);
  int64_t GetPosition();
  int64_t GetLength();
  void Close();

  std::string GetContentMimeType();
  std::string GetContentCharset();

  /*!
   * \brief Get the minimum size that can be read from the open file. For example,
   *        CDROM files in which access could be sector-based. It can also be
   *        used to indicate a file system is non-buffered but accepts any read
   *        size, in which case GetChunkSize() should return the value 1.
   * \return The chunk size
   */
  unsigned int GetChunkSize();

  /*!
   * \brief Calculate a size that is aligned to chunk size, but always greater
   *        or equal to the file's chunk size
   */
  static int GetChunkSize(int chunk, int minimum)
  {
    return chunk ? chunk * ((minimum + chunk - 1) / chunk) : minimum;
  }

  static bool Exists(const std::string &url);
  static int Stat(const std::string &url, struct __stat64 *buffer);
  static bool Delete(const std::string &url);
  static bool Rename(const std::string &url, const std::string &urlnew);
  static bool SetHidden(const std::string &url, bool hidden);

  //int IoControl(EIoControl request, void* param) { return -1; }

  static bool OnSameFileSystem(const std::string &strFile1, const std::string &strFile2);

private:
  static IFile *CreateLoader(const std::string &path);

  IFile        *m_pFileImpl;
  unsigned int  m_flags;
  //CFileStreamBuffer* m_pBuffer;
};
