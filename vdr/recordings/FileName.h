#pragma once

#include <stdint.h>
#include <stdio.h>

class cUnbufferedFile;

class cFileName
{
public:
  cFileName(const char *FileName, bool Record, bool Blocking = false, bool IsPesRecording = false);
  ~cFileName();
  const char *Name(void) { return fileName; }
  uint16_t Number(void) { return fileNumber; }
  bool GetLastPatPmtVersions(int &PatVersion, int &PmtVersion);
  cUnbufferedFile *Open(void);
  void Close(void);
  cUnbufferedFile *SetOffset(int Number, off_t Offset = 0); // yes, Number is int for easier internal calculating
  cUnbufferedFile *NextFile(void);

private:
  cUnbufferedFile *file;
  uint16_t fileNumber;
  char *fileName, *pFileNumber;
  bool record;
  bool blocking;
  bool isPesRecording;
};
