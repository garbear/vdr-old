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

#include "File.h"
#include "native/HDFile.h"
#include "SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/url/URL.h"

#ifdef TARGET_XBMC
#include "xbmc/VFSFile.h"
#endif

#include <auto_ptr.h>

using namespace std;

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  do { delete (p); (p) = NULL; } while (0)
#endif

CFile::CFile()
 : m_pFileImpl(NULL),
   m_flags(0),
   m_bOpenForWrite(false)
{
}

CFile::~CFile()
{
  Close();
}

IFile *CFile::CreateLoader(const string &path)
{
#if TARGET_XBMC
  return new cVFSFile();
#else
  string translatedPath = CSpecialProtocol::TranslatePath(path);
  CURL url(translatedPath);
  string protocol = url.GetProtocol();
  StringUtils::ToLower(protocol); // TODO: have GetProtocol() canonicalize to lowercase

  if (protocol == "file" || protocol.empty())
    return new CHDFile();
  return NULL;
#endif
}

bool CFile::Open(const string &url, unsigned int flags /* = 0 */)
{
  Close();
  // TODO: Evaluate if we should import SpecialProtocolFile from XBMC so that
  // we can remove calls to TranslatePath() at the beginning of most functions
  m_flags = flags;

  /*
  if ((flags & READ_NO_CACHE) == 0)
    m_flags |= READ_CACHED;

  if (m_flags & READ_CACHED)
  {
    // for internet stream, if it contains multiple stream, file cache need handle it specially.
    //m_pFileImpl = new CFileCache((m_flags & READ_MULTI_STREAM) !=0 );
    return m_pFileImpl && m_pFileImpl->Open(url);
  }
  */

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

bool CFile::OpenForWrite(const string &url, bool bOverWrite /* = false */)
{
  Close();

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

  m_bOpenForWrite = true;

  return true;
}

int64_t CFile::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_pFileImpl)
    return 0;
  return m_pFileImpl->Read(lpBuf, uiBufSize);
}

bool CFile::ReadLine(string &strLine)
{
  strLine.clear();
  if (!m_pFileImpl)
    return false;
  return m_pFileImpl->ReadLine(strLine);
}

bool CFile::LoadFile(const string &url, vector<uint8_t> &outputBuffer)
{
  static const unsigned int MAX_FILE_SIZE = 0x7FFFFFFF; // TODO
  static const unsigned int MIN_CHUNK_SIZE = 64 * 1024U;
  static const unsigned int MAX_CHUNK_SIZEI = 2048 * 1024U;

  outputBuffer.clear();
  if (!Open(url, READ_TRUNCATED))
    return false;

  /*
   * GetLength() will typically return values that fall into three cases:
   * 1. The real filesize. This is the typical case.
   * 2. Zero. This is the case for some http:// streams for example.
   * 3. Some value smaller than the real filesize. This is the case for an expanding file.
   *
   * In order to handle all three cases, we read the file in chunks, relying on Read()
   * returning 0 at EOF.  To minimize (re)allocation of the buffer, the chunksize in
   * cases 1 and 3 is set to one byte larger than the value returned by GetLength().
   * The chunksize in case 2 is set to the lowest value larger than MIN_CHUNK_SIZE aligned
   * to GetChunkSize().
   *
   * We fill the buffer entirely before reallocation.  Thus, reallocation never occurs in case 1
   * as the buffer is larger than the file, so we hit EOF before we hit the end of buffer.
   *
   * To minimize reallocation, we double the chunksize each read while chunksize is lower
   * than MAX_CHUNK_SIZEI.
   */
  int64_t filesize = GetLength();
  if (filesize > MAX_FILE_SIZE)
    return false; // File is too large for this function // TODO

  unsigned int chunksize = (filesize > 0) ? (unsigned int)(filesize + 1) : GetChunkSize(GetChunkSize(), MIN_CHUNK_SIZE);
  unsigned int total_read = 0;
  while (true)
  {
    if (total_read == outputBuffer.size())
    {
      // (re)alloc
      if (outputBuffer.size() >= MAX_FILE_SIZE)
      {
        outputBuffer.clear();
        return false;
      }

      outputBuffer.resize(outputBuffer.size() + chunksize);
      if (chunksize < MAX_CHUNK_SIZEI)
        chunksize *= 2;
    }
    unsigned int read = Read(outputBuffer.data() + total_read, outputBuffer.size() - total_read);
    total_read += read;
    if (!read)
      break;
  }

  outputBuffer.resize(total_read);

  return true;
}

int64_t CFile::Write(const void *lpBuf, int64_t uiBufSize)
{
  if (!m_pFileImpl || !m_bOpenForWrite)
    return 0;
  return m_pFileImpl->Write(lpBuf, uiBufSize);
}

void CFile::Flush()
{
  if (!m_pFileImpl)
    return;
  return m_pFileImpl->Flush();
}

int64_t CFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET */)
{
  if (!m_pFileImpl)
    return -1;
  return m_pFileImpl->Seek(iFilePosition, iWhence);
}

int64_t CFile::GetPosition()
{
  if (!m_pFileImpl)
    return -1;
  return m_pFileImpl->GetPosition();
}

int64_t CFile::GetLength()
{
  if (!m_pFileImpl)
    return 0;
  return m_pFileImpl->GetLength();
}

int64_t CFile::Truncate(uint64_t size)
{
  if (!m_pFileImpl)
    return -1;
  return m_pFileImpl->Truncate(size);
}

void CFile::Close()
{
  if (!m_pFileImpl)
    return;
  m_pFileImpl->Close();
  SAFE_DELETE(m_pFileImpl);
  m_bOpenForWrite = false;
}

string CFile::GetContentMimeType()
{
  if (!m_pFileImpl)
    return MIME_TYPE_NONE;
  return m_pFileImpl->GetContent();
}

string CFile::GetContentCharset()
{
  if (!m_pFileImpl)
    return "";
  return m_pFileImpl->GetContentCharset();
}

unsigned int CFile::GetChunkSize()
{
  if (!m_pFileImpl)
    return 0;
  return m_pFileImpl->GetChunkSize();
}

bool CFile::Exists(const string &url)
{
  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return false;
  return pFile->Exists(url);
}

int CFile::Stat(const string &url, struct __stat64 *buffer)
{
  if (buffer)
    memset(buffer, 0, sizeof(struct __stat64));

  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return -1;
  return pFile->Stat(url, buffer);
}

bool CFile::Delete(const string &url)
{
  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return false;
  return pFile->Delete(url);
}

bool CFile::Rename(const string &url, const string &urlnew)
{
  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return false;
  return pFile->Rename(url, urlnew);
}

bool CFile::SetHidden(const string &url, bool hidden)
{
  std::auto_ptr<IFile> pFile(CreateLoader(url));
  if (!pFile.get())
    return false;
  return pFile->SetHidden(url, false);
}

bool CFile::OnSameFileSystem(const string &strFile1, const string &strFile2)
{
  struct __stat64 statStruct;
  CFile file;
  file.Stat(strFile1, &statStruct);
  dev_t dev1 = statStruct.st_dev;
  if (dev1)
  {
    file.Stat(strFile2, &statStruct);
    return dev1 == statStruct.st_dev;
  }
  return false;
}
