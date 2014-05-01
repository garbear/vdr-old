#pragma once

#include <string>

namespace VDR
{
class cResumeFile
{
public:
  cResumeFile(const std::string& strFileName, bool IsPesRecording);
  ~cResumeFile();
  int Read(void);
  bool Save(int Index);
  void Delete(void);
private:
  std::string m_strFileName;
  bool        m_bIsPesRecording;
};
}
