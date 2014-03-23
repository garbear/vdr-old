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
#include "filesystem/SpecialProtocol.h"
#include "xbmc/libXBMC_addon.h"

namespace VDR
{

extern ADDON::CHelper_libXBMC_addon *XBMC;

cVFSFile::cVFSFile()
 : m_file(XBMC)
{
}

bool cVFSFile::Open(const std::string &url, unsigned int flags /* = 0 */)
{
  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(url);
  return m_file.Open(strTranslatedPath, flags);
}

bool cVFSFile::OpenForWrite(const std::string &url, bool bOverWrite /* = false */)
{
  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(url);
  return m_file.OpenForWrite(strTranslatedPath, bOverWrite);
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

  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(url);
  return XBMC->FileExists(strTranslatedPath.c_str(), true);
}

int cVFSFile::Stat(const std::string &url, struct __stat64 *buffer)
{
  if (!XBMC)
    return IFile::Stat(url, buffer);

  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(url);
  return XBMC->StatFile(strTranslatedPath.c_str(), buffer);
}

bool cVFSFile::Delete(const std::string &url)
{
  if (!XBMC)
    return false;

  std::string strTranslatedPath = CSpecialProtocol::TranslatePath(url);
  return XBMC->DeleteFile(strTranslatedPath.c_str());
}

bool cVFSFile::Rename(const std::string &url, const std::string &urlnew)
{
  if (!XBMC)
    return false;

  std::string strTranslatedPath    = CSpecialProtocol::TranslatePath(url);
  std::string strTranslatedPathNew = CSpecialProtocol::TranslatePath(urlnew);
  return XBMC->RenameFile(strTranslatedPath.c_str(), strTranslatedPathNew.c_str());
}

}
