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
#pragma once

#include "File.h"

#include <stddef.h>
#include <sys/types.h>

namespace VDR
{

class CVideoFile
{
public:
  CVideoFile(void);
  virtual ~CVideoFile();

  bool Open(const char *FileName, int Flags, mode_t Mode = DEFFILEMODE);
  void Close(void);
  void SetReadAhead(size_t ra);
  off_t Seek(off_t Offset, int Whence);
  ssize_t Read(void *Data, size_t Size);
  ssize_t Write(const void *Data, size_t Size);

  static CVideoFile *Create(const char *FileName, int Flags, mode_t Mode = DEFFILEMODE);

private:
  int FadviseDrop(off_t Offset, off_t Len);

  CFile  m_file;
  off_t  m_curpos;
  off_t  m_cachedstart;
  off_t  m_cachedend;
  off_t  m_begin;
  off_t  m_lastpos;
  off_t  m_ahead;
  size_t m_readahead;
  size_t m_written;
  size_t m_totwritten;
};

}
