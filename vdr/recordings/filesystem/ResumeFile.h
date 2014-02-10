#pragma once

#include <string>

class cResumeFile
{
public:
  cResumeFile(const char *FileName, bool IsPesRecording);
  ~cResumeFile();
  int Read(void);
  bool Save(int Index);
  void Delete(void);
private:
  std::string m_strFileName;
  bool        m_bIsPesRecording;
};
