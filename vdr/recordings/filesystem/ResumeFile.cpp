#include "ResumeFile.h"
#include "../Recordings.h"
#include "Config.h"
#include "utils/Tools.h"

#define RESUMEFILESUFFIX  "/resume%s%s"

cResumeFile::cResumeFile(const std::string& strFileName, bool IsPesRecording)
{
  m_bIsPesRecording = IsPesRecording;
  const char *Suffix = m_bIsPesRecording ? RESUMEFILESUFFIX ".vdr" : RESUMEFILESUFFIX;
  m_strFileName = strFileName;
  m_strFileName.append(StringUtils::Format(Suffix, g_setup.ResumeID ? "." : "", g_setup.ResumeID ? *itoa(g_setup.ResumeID) : ""));
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
