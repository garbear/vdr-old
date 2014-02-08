#pragma once

class cResumeFile
{
public:
  cResumeFile(const char *FileName, bool IsPesRecording);
  ~cResumeFile();
  int Read(void);
  bool Save(int Index);
  void Delete(void);
private:
  char *fileName;
  bool isPesRecording;
};
