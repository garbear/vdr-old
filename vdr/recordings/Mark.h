#pragma once

#include "utils/Tools.h"
#include "platform/threads/mutex.h"

#define DEFAULTFRAMESPERSECOND 25.0

extern double MarkFramesPerSecond;
extern PLATFORM::CMutex MutexMarkFramesPerSecond;

class cMark : public cListObject {
  friend class cMarks; // for sorting
private:
  double framesPerSecond;
  int position;
  cString comment;
public:
  cMark(int Position = 0, const char *Comment = NULL, double FramesPerSecond = DEFAULTFRAMESPERSECOND);
  virtual ~cMark();
  int Position(void) const { return position; }
  const char *Comment(void) const { return comment; }
  void SetPosition(int Position) { position = Position; }
  void SetComment(const char *Comment) { comment = Comment; }
  cString ToText(void);
  bool Parse(const char *s);
  bool Save(FILE *f);
  };
