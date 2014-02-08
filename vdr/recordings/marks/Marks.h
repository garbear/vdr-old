#pragma once

#include "Mark.h"
#include "Config.h"

class cMarks : public cConfig<cMark> {
private:
  cString recordingFileName;
  cString fileName;
  double framesPerSecond;
  bool isPesRecording;
  time_t nextUpdate;
  time_t lastFileTime;
  time_t lastChange;
public:
  bool Load(const char *RecordingFileName, double FramesPerSecond = DEFAULTFRAMESPERSECOND, bool IsPesRecording = false);
  bool Update(void);
  bool Save(void);
  void Align(void);
  void Sort(void);
  void Add(int Position);
  cMark *Get(int Position);
  cMark *GetPrev(int Position);
  cMark *GetNext(int Position);
  cMark *GetNextBegin(cMark *EndMark = NULL);
       ///< Returns the next "begin" mark after EndMark, skipping any marks at the
       ///< same position as EndMark. If EndMark is NULL, the first actual "begin"
       ///< will be returned (if any).
  cMark *GetNextEnd(cMark *BeginMark);
       ///< Returns the next "end" mark after BeginMark, skipping any marks at the
       ///< same position as BeginMark.
  int GetNumSequences(void);
       ///< Returns the actual number of sequences to be cut from the recording.
       ///< If there is only one actual "begin" mark, and it is positioned at index
       ///< 0 (the beginning of the recording), and there is no "end" mark, the
       ///< return value is 0, which means that the result is the same as the original
       ///< recording.
  };
