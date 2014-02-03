#include "ResumeFile.h"
#include "Recordings.h"
#include "Config.h"
#include "utils/Tools.h"

#define RESUMEFILESUFFIX  "/resume%s%s"

cResumeFile::cResumeFile(const char *FileName, bool IsPesRecording)
{
  isPesRecording = IsPesRecording;
  const char *Suffix = isPesRecording ? RESUMEFILESUFFIX ".vdr" : RESUMEFILESUFFIX;
  fileName = MALLOC(char, strlen(FileName) + strlen(Suffix) + 1);
  if (fileName) {
     strcpy(fileName, FileName);
     sprintf(fileName + strlen(fileName), Suffix, g_setup.ResumeID ? "." : "", g_setup.ResumeID ? *itoa(g_setup.ResumeID) : "");
     }
  else
     esyslog("ERROR: can't allocate memory for resume file name");
}

cResumeFile::~cResumeFile()
{
  free(fileName);
}

int cResumeFile::Read(void)
{
  int resume = -1;
  if (fileName)
  {
    CFile file;
    if (file.Open(fileName))
    {
      if (isPesRecording)
      {
        if (!file.Read(&resume, sizeof(resume)))
        {
          LOG_ERROR_STR(fileName);
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
      LOG_ERROR_STR(fileName);
    }
  }

  return resume;
}

bool cResumeFile::Save(int Index)
{
  if (fileName) {
    CFile file;
    if (file.OpenForWrite(fileName, true))
    {
      if (isPesRecording)
      {
        file.Write(&Index, sizeof(Index));
      }
      else
      {
        char buf[9];
        snprintf(buf, 8, "I %d\n", Index);
        file.Write(buf, sizeof(buf));
      }
      Recordings.ResetResume(fileName);
      return true;
    }
    else
    {
      LOG_ERROR_STR(fileName);
    }
  }
  return false;
}

void cResumeFile::Delete(void)
{
  if (fileName)
  {
    if (CFile::Delete(fileName))
      Recordings.ResetResume(fileName);
    else if (CFile::Exists(fileName))
      LOG_ERROR_STR(fileName);
  }
}
