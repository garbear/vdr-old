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

#include "ResumeFile.h"
#include "Config.h"
#include "recordings/Recordings.h"
#include "settings/Settings.h"
#include "utils/log/Log.h"

#define RESUMEFILESUFFIX  "/resume%s%s"

namespace VDR
{

cResumeFile::cResumeFile(const std::string& strFileName, bool IsPesRecording)
{
  m_bIsPesRecording = IsPesRecording;
  const char *Suffix = m_bIsPesRecording ? RESUMEFILESUFFIX ".vdr" : RESUMEFILESUFFIX;
  m_strFileName = strFileName;
  m_strFileName.append(StringUtils::Format(Suffix, cSettings::Get().m_iResumeID ? "." : "", cSettings::Get().m_iResumeID ? StringUtils::Format("%d", cSettings::Get().m_iResumeID).c_str() : ""));
}

cResumeFile::~cResumeFile()
{
}

int cResumeFile::Read(void)
{
  int resume = -1;
#if 0
  if (!m_strFileName.empty())
  {
    CFile file;
    if (file.Open(m_strFileName))
    {
      if (m_bIsPesRecording)
      {
        if (!file.Read(&resume, sizeof(resume)))
        {
          LOG_ERROR_STR(m_strFileName.c_str());
          resume = -1;
        }
      }
      else
      {
        std::string line;
        while (file.ReadLine(line))
          if (*line.c_str() == 'I')
            resume = atoi(skipspace(line.c_str()));
      }
    }
    else
    {
      LOG_ERROR_STR(m_strFileName.c_str());
    }
  }
#endif

  return resume;
}

bool cResumeFile::Save(int Index)
{
#if 0
  if (!m_strFileName.empty())
  {
    CFile file;
    if (file.OpenForWrite(m_strFileName, true))
    {
      if (m_bIsPesRecording)
      {
        file.Write(&Index, sizeof(Index));
      }
      else
      {
        char buf[9];
        snprintf(buf, 8, "I %d\n", Index);
        file.Write(buf, sizeof(buf));
      }
      Recordings.ResetResume(m_strFileName);
      return true;
    }
    else
    {
      LOG_ERROR_STR(m_strFileName.c_str());
    }
  }
#endif
  return false;
}

void cResumeFile::Delete(void)
{
  if (!m_strFileName.empty())
  {
    if (CFile::Delete(m_strFileName))
      Recordings.ResetResume(m_strFileName);
    else if (CFile::Exists(m_strFileName))
      LOG_ERROR_STR(m_strFileName.c_str());
  }
}

}
