#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string>

class CVideoFile;

class cFileName
{
public:
  cFileName(const std::string& strFileName, bool Record, bool Blocking = false, bool IsPesRecording = false);
  ~cFileName();
  std::string Name(void) { return m_strFileName + m_strFileOffset; }
  uint16_t Number(void) { return m_iFileNumber; }
  bool GetLastPatPmtVersions(int &PatVersion, int &PmtVersion);
  CVideoFile *Open(void);
  void Close(void);
  CVideoFile *SetOffset(int Number, off_t Offset = 0); // yes, Number is int for easier internal calculating
  CVideoFile *NextFile(void);

private:
  CVideoFile* m_file;
  uint16_t    m_iFileNumber;
  std::string m_strFileName;
  std::string m_strFileOffset;
  bool        m_bRecord;
  bool        m_bBlocking;
  bool        m_bIsPesRecording;
};
