#pragma once

#include "File.h"

#include <sys/types.h>

namespace VDR
{

class CVideoFile
{
public:
  CVideoFile(void);
  virtual ~CVideoFile();

  bool Open(const char *FileName, int Flags, mode_t Mode = DEFFILEMODE);
  void Close(void);
  void SetReadAhead(size_t ra);
  off_t Seek(off_t Offset, int Whence);
  ssize_t Read(void *Data, size_t Size);
  ssize_t Write(const void *Data, size_t Size);

  static CVideoFile *Create(const char *FileName, int Flags, mode_t Mode = DEFFILEMODE);

private:
  int FadviseDrop(off_t Offset, off_t Len);

  CFile  m_file;
  off_t  m_curpos;
  off_t  m_cachedstart;
  off_t  m_cachedend;
  off_t  m_begin;
  off_t  m_lastpos;
  off_t  m_ahead;
  size_t m_readahead;
  size_t m_written;
  size_t m_totwritten;
};

}
