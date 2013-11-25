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

#include "VFSFile.h"

#include "xbmc/libXBMC_addon.h"

extern ADDON::CHelper_libXBMC_addon *XBMC;

cVFSFile::cVFSFile()
 : m_file(XBMC)
{
}

bool cVFSFile::Open(const std::string &url, unsigned int flags /* = 0 */)
{
  return m_file.Open(url, flags);
}

bool cVFSFile::OpenForWrite(const std::string &url, bool bOverWrite /* = false */)
{
  return m_file.OpenForWrite(url, bOverWrite);
}

int64_t cVFSFile::Read(void *lpBuf, uint64_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}

bool cVFSFile::ReadLine(std::string &strLine)
{
  return m_file.ReadLine(strLine);
}

int64_t cVFSFile::Write(const void *lpBuf, uint64_t uiBufSize)
{
  return m_file.Write(lpBuf, uiBufSize);
}

void cVFSFile::Flush()
{
  return m_file.Flush();
}

int64_t cVFSFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET */)
{
  return m_file.Seek(iFilePosition, iWhence);
}

int64_t cVFSFile::GetPosition()
{
  return m_file.GetPosition();
}

int64_t cVFSFile::GetLength()
{
  return m_file.GetLength();
}

int64_t cVFSFile::Truncate(uint64_t size)
{
  return m_file.Truncate(size);
}

void cVFSFile::Close()
{
  m_file.Close();
}

unsigned int cVFSFile::GetChunkSize()
{
  return m_file.GetChunkSize();
}

bool cVFSFile::Exists(const std::string &url)
{
  if (!XBMC)
    return false;
  return XBMC->FileExists(url.c_str(), true);
}

int cVFSFile::Stat(const std::string &url, struct __stat64 *buffer)
{
  if (!XBMC)
    return IFile::Stat(url, buffer);
  return XBMC->StatFile(url.c_str(), buffer);
}

bool cVFSFile::Delete(const std::string &url)
{
  if (!XBMC)
    return false;
  return XBMC->DeleteFile(url.c_str());
}
