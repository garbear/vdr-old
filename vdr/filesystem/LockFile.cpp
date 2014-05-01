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

#include "LockFile.h"
#include "Directory.h"
#include "platform/threads/mutex.h"
#include "utils/DateTime.h"
#include "utils/log/Log.h"
#include "utils/Tools.h" // for AddDirectory()

#include <cstdlib>

namespace VDR
{

#define LOCKFILENAME       ".lock-vdr"
#define LOCKFILESTALETIME  600 // seconds before considering a lock file "stale"

cLockFile::cLockFile(const std::string& strDirectory)
{
  if (CDirectory::CanWrite(strDirectory))
    m_strFilename = AddDirectory(strDirectory, LOCKFILENAME);
}

cLockFile::~cLockFile()
{
  Unlock();
}

bool cLockFile::Lock(int WaitSeconds)
{
  if (!m_file.IsOpen() && !m_strFilename.empty())
  {
    CDateTime Timeout = CDateTime::GetUTCDateTime() + CDateTimeSpan(0, 0, 0, WaitSeconds);
    do
    {
      m_file.OpenForWrite(m_strFilename, true);
      if (!m_file.IsOpen())
      {
        if (CFile::Exists(m_strFilename))
        {
          struct stat64 fs;
          if (CFile::Stat(m_strFilename, &fs) == 0)
          {
            if (std::abs((long)(time(NULL) - fs.st_mtime)) > LOCKFILESTALETIME)
            {
              esyslog("ERROR: removing stale lock file '%s'", m_strFilename.c_str());
              if (!CFile::Delete(m_strFilename))
              {
                LOG_ERROR_STR(m_strFilename.c_str());
                break;
              }
              continue;
            }
          }
          else
          {
            LOG_ERROR_STR(m_strFilename.c_str());
            break;
          }
        }
        else
        {
          LOG_ERROR_STR(m_strFilename.c_str());
          break;
        }
        if (WaitSeconds)
          PLATFORM::CEvent::Sleep(1000);
      }
    } while (!m_file.IsOpen() && CDateTime::GetUTCDateTime() < Timeout);
  }

  return m_file.IsOpen();
}

void cLockFile::Unlock(void)
{
  if (m_file.IsOpen())
  {
    m_file.Close();
    CFile::Delete(m_strFilename);
  }
}

}
