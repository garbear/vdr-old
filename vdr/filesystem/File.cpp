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

#include "File.h"
#include "VFSFile.h"

#include <auto_ptr.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  do { delete (p); (p)=NULL; } while (0)
#endif

cFile::cFile()
 : m_pFileImpl(NULL),
   m_flags(0)
{
}

cFile::~cFile()
{
  SAFE_DELETE(m_pFileImpl);
}

IFile *cFile::CreateLoader(const std::string &url)
{
  bool isXBMC = true;
  if (isXBMC)
    return new cVFSFile();
  return NULL;
}

bool cFile::Open(const std::string &url, unsigned int flags /* = 0 */)
{
  m_flags = flags;
  if ((flags & READ_NO_CACHE) == 0)
    m_flags |= READ_CACHED;

  if (m_flags & READ_CACHED)
  {
    // for internet stream, if it contains multiple stream, file cache need handle it specially.
    //m_pFileImpl = new cFileCache((m_flags & READ_MULTI_STREAM) !=0 );
    return m_pFileImpl && m_pFileImpl->Open(url);
  }

  m_pFileImpl = CreateLoader(url);
  if (!m_pFileImpl)
    return false;

  try
  {
    if (!m_pFileImpl->Open(url, m_flags))
    {
      SAFE_DELETE(m_pFileImpl);
      return false;
    }
  }
  catch (...)
  {
    SAFE_DELETE(m_pFileImpl);
    return false;
  }

  if (m_pFileImpl->GetChunkSize() && !(m_flags & READ_CHUNKED))
  {
    //m_pBuffer = new CFileStreamBuffer(0);
    //m_pBuffer->Attach(m_pFile);
  }

  if (m_flags & READ_BITRATE)
  {
    // Bitrate stats not used
  }

  return true;
}

bool cFile::OpenForWrite(const std::string &url, bool bOverWrite /* = false */)
{
  m_pFileImpl = CreateLoader(url);
  if (!m_pFileImpl)
    return false;

  try
  {
    if (!m_pFileImpl->OpenForWrite(url, bOverWrite))
    {
      SAFE_DELETE(m_pFileImpl);
      return false;
    }
  }
  catch (...)
  {
    SAFE_DELETE(m_pFileImpl);
    return false;
  }

  return true;
}

int64_t cFile::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_pFileImpl)
    return 0;
  return m_pFileImpl->Read(lpBuf, uiBufSize);
}

bool cFile::ReadLine(std::string &strLine)
{
  strLine.clear();
  if (!m_pFileImpl)
    return false;
  return m_pFileImpl->ReadLine(strLine);
}

int64_t cFile::Write(const void *lpBuf, int64_t uiBufSize)
{
  if (!m_pFileImpl)
    return 0;
  return m_pFileImpl->Write(lpBuf, uiBufSize);
}

void cFile::Flush()
{
  if (!m_pFileImpl)
    return;
  return m_pFileImpl->Flush();
}

int64_t cFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET */)
{
  if (!m_pFileImpl)
    return -1;
  return m_pFileImpl->Seek(iFilePosition, iWhence);
}

int64_t cFile::GetPosition()
{
  if (!m_pFileImpl)
    return -1;
  return m_pFileImpl->GetPosition();
}

int64_t cFile::GetLength()
{
  if (!m_pFileImpl)
    return 0;
  return m_pFileImpl->GetLength();
}

int64_t cFile::Truncate(uint64_t size)
{
  if (!m_pFileImpl)
    return -1;
  return m_pFileImpl->Truncate(size);
}

void cFile::Close()
{
  if (!m_pFileImpl)
    return;
  m_pFileImpl->Close();
}

std::string cFile::GetContent()
{
  if (!m_pFileImpl)
    return "none/none";
  return m_pFileImpl->GetContent();
}

unsigned int cFile::GetChunkSize()
{
  if (!m_pFileImpl)
    return 0;
  return m_pFileImpl->GetChunkSize();
}

bool cFile::Exists(const std::string &url)
{
  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return false;
  return pFile->Exists(url);
}

int cFile::Stat(const std::string &url, struct __stat64 *buffer)
{
  if (buffer)
    memset(buffer, 0, sizeof(struct __stat64));

  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return -1;
  return pFile->Exists(url);
}

bool cFile::Delete(const std::string &url)
{
  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return false;
  return pFile->Delete(url);
}

bool cFile::Rename(const std::string &url, const std::string &urlnew)
{
  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return false;
  return pFile->Rename(url, urlnew);
}

bool cFile::SetHidden(const std::string &url, bool hidden)
{
  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return false;
  return pFile->SetHidden(url, false);
}

bool cFile::OnSameFileSystem(const std::string &strFile1, const std::string &strFile2)
{
  struct __stat64 statStruct;
  cFile file;
  file.Stat(strFile1, &statStruct);
  dev_t dev1 = statStruct.st_dev;
  if (dev1)
  {
    file.Stat(strFile2, &statStruct);
    return dev1 == statStruct.st_dev;
  }
  return false;
}
