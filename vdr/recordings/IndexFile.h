#pragma once

#include "filesystem/File.h"
#include "utils/Tools.h"
#include "ResumeFile.h"
#include "platform/threads/threads.h"

struct tIndexTs;
class cIndexFileGenerator;

class cIndexFile {
private:
  CFile m_file;
  cString fileName;
  int size, last;
  tIndexTs *index;
  bool isPesRecording;
  cResumeFile resumeFile;
  cIndexFileGenerator *indexFileGenerator;
  PLATFORM::CMutex mutex;
  void ConvertFromPes(tIndexTs *IndexTs, int Count);
  void ConvertToPes(tIndexTs *IndexTs, int Count);
  bool CatchUp(int Index = -1);
public:
  cIndexFile(const char *FileName, bool Record, bool IsPesRecording = false, bool PauseLive = false);
  ~cIndexFile();
  bool Ok(void) { return index != NULL; }
  bool Write(bool Independent, uint16_t FileNumber, off_t FileOffset);
  bool Get(int Index, uint16_t *FileNumber, off_t *FileOffset, bool *Independent = NULL, int *Length = NULL);
  int GetNextIFrame(int Index, bool Forward, uint16_t *FileNumber = NULL, off_t *FileOffset = NULL, int *Length = NULL);
  int GetClosestIFrame(int Index);
       ///< Returns the index of the I-frame that is closest to the given Index (or Index itself,
       ///< if it already points to an I-frame). Index may be any value, even outside the current
       ///< range of frame indexes.
       ///< If there is no actual index data available, 0 is returned.
  int Get(uint16_t FileNumber, off_t FileOffset);
  int Last(void) { CatchUp(); return last; }
       ///< Returns the index of the last entry in this file, or -1 if the file is empty.
  int GetResume(void) { return resumeFile.Read(); }
  bool StoreResume(int Index) { return resumeFile.Save(Index); }
  bool IsStillRecording(void);
  void Delete(void);
  static int GetLength(const char *FileName, bool IsPesRecording = false);
       ///< Calculates the recording length (number of frames) without actually reading the index file.
       ///< Returns -1 in case of error.
  static cString IndexFileName(const char *FileName, bool IsPesRecording);
  };

bool GenerateIndex(const char *FileName);

