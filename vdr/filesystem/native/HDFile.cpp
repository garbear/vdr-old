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
#include "utils/url/URL.h"

using namespace std;

CHDFile::CHDFile()
 : m_mode((std::ios_base::openmode)0),
   m_flags(0)
{
}

CHDFile::~CHDFile()
{
  Close();
}

bool CHDFile::Open(const string &url, unsigned int flags /* = 0 */)
{
  Close();

  m_flags = flags;

  m_mode = ios::in;
  m_file.open(url.c_str(), m_mode);

  return m_file.is_open();
}

bool CHDFile::OpenForWrite(const string &url, bool bOverWrite /* = false */)
{
  Close();

  if (!bOverWrite && Exists(url))
    return false;

  m_mode = ios::out;
  m_file.open(url.c_str(), m_mode);

  return m_file.is_open();
}

int64_t CHDFile::Read(void *lpBuf, uint64_t uiBufSize)
{
  if (lpBuf && uiBufSize && m_file.is_open() && (m_mode & ios::in))
  {
    int64_t start = m_file.tellg();

    // TODO: Respect m_flags
    m_file.read(reinterpret_cast<char*>(lpBuf), uiBufSize);

    if (m_file.eof())
    {
      m_file.clear();
      // Can't use m_file.tellg(), EOF causes tellg() to return -1
      int64_t size = Seek(0, SEEK_END);
      return size > 0 ? size - start : 0;
    }
    else if (!m_file.fail())
      return uiBufSize;
  }
  return 0;
}

bool CHDFile::ReadLine(string &strLine)
{
  if (m_file.is_open() && (m_mode & ios::in))
    return std::getline(m_file, strLine);

  return false;
}

int64_t CHDFile::Write(const void* lpBuf, uint64_t uiBufSize)
{
  if (lpBuf && uiBufSize && m_file.is_open() && (m_mode & ios::out))
  {
    m_file.write(reinterpret_cast<const char*>(lpBuf), uiBufSize);
    return !m_file.fail() ? uiBufSize : 0;
  }
  return 0;
}

void CHDFile::Flush()
{
  if (m_file.is_open())
    m_file.flush();
}

int64_t CHDFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET */)
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

int CHDFile::Truncate(int64_t size)
{
  return -1;
}

int64_t CHDFile::GetPosition()
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

int64_t CHDFile::GetLength()
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

void CHDFile::Close()
{
  if (m_file.is_open())
    m_file.close();
  m_mode = (std::ios_base::openmode)0;
  m_flags = 0;
}

bool CHDFile::Exists(const string &url)
{
  ifstream file(url.c_str());
  return (bool)file;
}

int CHDFile::Stat(const string &url, struct __stat64 *buffer)
{
  return -1;
}

bool CHDFile::Delete(const string &url)
{
  string strFile(GetLocal(url));

#ifdef TARGET_WINDOWS
  //return ::DeleteFileW(CWIN32Util::ConvertPathToWin32Form(strFile).c_str()) ? true : false; // TODO
#else
  return remove(strFile.c_str()) == 0;
#endif
}

bool CHDFile::Rename(const string &url, const string &urlnew)
{
#ifdef TARGET_WINDOWS
  //TODO
#else
  return rename(url.c_str(), urlnew.c_str()) == 0;
#endif
}

bool CHDFile::SetHidden(const string &url, bool hidden)
{
  return false;
}

std::string CHDFile::GetLocal(const CURL &url)
{
  std::string path(url.GetFileName());

  if (url.GetProtocol() == "file")
  {
    // file://drive[:]/path
    // file:///drive:/path
    std::string host(url.GetHostName());

    if (!host.empty())
    {
      if (*(host.end() - 1) == ':')
        path = host + "/" + path;
      else
        path = host + ":/" + path;
    }
  }

  // Linux does not use alias or shortcut methods (TODO for other OSes)
  /*
  if (IsAliasShortcut(path))
    TranslateAliasShortcut(path);
  */

  return path;
}
