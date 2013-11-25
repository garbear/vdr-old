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

using namespace std;

cHDFile::cHDFile()
 : m_mode((std::ios_base::openmode)0)
{
}

cHDFile::~cHDFile()
{
  Close();
}

bool cHDFile::Open(const string &url, unsigned int flags /* = 0 */)
{
  Close();

  m_flags = flags;

  m_mode = ios::in;
  m_file.open(url.c_str(), m_mode);

  return m_file.is_open();
}

bool cHDFile::OpenForWrite(const string &url, bool bOverWrite /* = false */)
{
  Close();

  m_mode = ios::out;
  m_file.open(url.c_str(), m_mode);

  return m_file.is_open();
}

int64_t cHDFile::Read(void *lpBuf, uint64_t uiBufSize)
{
  if (m_file.is_open() && (m_mode & ios::in))
  {
    int64_t start = m_file.tellg();

    // TODO: Respect m_flags
    m_file.read(reinterpret_cast<char*>(lpBuf), uiBufSize);

    if (m_file.eof())
      return m_file.tellg() - start;
    else if (!m_file.fail())
      return uiBufSize;
  }
  return 0;
}

bool cHDFile::ReadLine(string &strLine)
{
  if (m_file.is_open() && (m_mode & ios::in))
  {
    //if (m_file.openmode & ios::binary)
    //  return false; // Shouldn't happen
    m_file >> strLine;
    return !strLine.empty();
  }
  return false;
}

int64_t cHDFile::Write(const void* lpBuf, uint64_t uiBufSize)
{
  if (m_file.is_open() && (m_mode & ios::out))
  {
    m_file.write(reinterpret_cast<const char*>(lpBuf), uiBufSize);
    return !m_file.fail();
  }
  return 0;
}

void cHDFile::Flush()
{
  if (m_file.is_open())
    m_file.flush();
}

int64_t cHDFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET */)
{
  if (m_file.is_open())
  {
    ios::seekdir whence;
    switch (iWhence)
    {
    case SEEK_CUR: whence = ios_base::cur; break;
    case SEEK_END: whence = ios_base::end; break;
    default:
    case SEEK_SET: whence = ios_base::beg; break;
    }

    if (m_mode & ios::in)
    {
      m_file.seekg(iFilePosition, whence);
      return m_file.tellg();
    }
    else if (m_mode & ios::out)
    {
      m_file.seekp(iFilePosition, whence);
      return m_file.tellp();
    }
  }

  return 0;
}

int cHDFile::Truncate(int64_t size)
{
  return -1;
}

int64_t cHDFile::GetPosition()
{
  if (m_file.is_open())
  {
    if (m_mode & ios::in)
      return m_file.tellg();
    else if (m_mode & ios::out)
      return m_file.tellp();
  }
  return -1;
}

int64_t cHDFile::GetLength()
{
  int64_t length = 0;

  if (m_file.is_open())
  {
    int pos = GetPosition();
    if (pos >= 0)
    {
      if (m_mode & ios::in)
      {
        m_file.seekg(0, ios_base::end);
        length = m_file.tellg();
        m_file.seekg(pos);
      }
      else if (m_mode & ios::out)
      {
        m_file.seekp(0, ios_base::end);
        length = m_file.tellp();
        m_file.seekp(pos);
      }
    }
  }

  return length;
}

void cHDFile::Close()
{
  if (m_file.is_open())
    m_file.close();
  m_mode = (std::ios_base::openmode)0;
  m_flags = 0;
}

bool cHDFile::Exists(const string &url)
{
  return false;
}

int cHDFile::Stat(const string &url, struct __stat64 *buffer)
{
  return -1;
}

bool cHDFile::Delete(const string &url)
{
  return false;
}

bool cHDFile::Rename(const string &url, const string &urlnew)
{
  return false;
}

bool cHDFile::SetHidden(const string &url, bool hidden)
{
  return false;
}
