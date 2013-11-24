/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
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

#include "HDFile.h"

#include <fcntl.h>

cHDFile::cHDFile()
 : m_fd(-1),
   m_curpos(0)
{

}

cHDFile::~cHDFile()
{
  Close();
}


bool cHDFile::Open(const std::string &url, unsigned int flags = 0)
{
  Close();
  int fileDes = open(url.c_str(), flags, DEFFILEMODE);
  if (fileDes >= 0)
  {
    m_fd = fileDes;
    return true;
  }
  return false;
}

bool cHDFile::OpenForWrite(const std::string &url, bool bOverWrite = false)
{
  return false;
}

int64_t cHDFile::Read(void *lpBuf, uint64_t uiBufSize)
{
  if (m_fd < 0)
    return 0;

  ssize_t bytesRead = 0;

  while (1)
  {
    ssize_t p = read(m_fd, lpBuf, uiBufSize);
    if (p >= 0 || errno == EINTR)
    {
      bytesRead =  p;
      break;
    }
  }

  if (bytesRead > 0)
    m_curpos += bytesRead;

  return bytesRead;
}

int cHDFile::Write(const void* lpBuf, uint64_t uiBufSize)
{
  return 0;
}

void cHDFile::Flush()
{
  return;
}

int64_t cHDFile::Seek(int64_t iFilePosition, int iWhence = SEEK_SET)
{
  if (m_fd < 0)
    return 0;

  if (iWhence == SEEK_SET && iFilePosition == m_curpos)
     return m_curpos;

  m_curpos = lseek(m_fd, iFilePosition, iWhence);
  return m_curpos;
}

int cHDFile::Truncate(int64_t size)
{
  return -1;
}

int64_t cHDFile::GetPosition()
{
  if (m_fd < 0)
    return -1;

  return m_curpos;
}

int64_t cHDFile::GetLength()
{
  if (m_fd < 0)
    return 0;

  return 0;
}

void cHDFile::Close()
{
  if (m_fd >= 0)
    close(m_fd);
  m_fd = -1;
}

bool cHDFile::Exists(const std::string &url)
{
  return false;
}

int cHDFile::Stat(const std::string &url, struct __stat64 *buffer)
{
  return -1;
}

bool cHDFile::Delete(const std::string &url)
{
  return false;
}

bool cHDFile::Rename(const std::string &url, const std::string &urlnew)
{
  return false;
}

bool cHDFile::SetHidden(const std::string &url, bool hidden)
{
  return false;
}
