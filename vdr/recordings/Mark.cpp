#include "Mark.h"
#include "platform/threads/mutex.h"

double MarkFramesPerSecond = DEFAULTFRAMESPERSECOND;
PLATFORM::CMutex MutexMarkFramesPerSecond;

cString IndexToHMSF(int Index, bool WithFrame, double FramesPerSecond)
{
  const char *Sign = "";
  if (Index < 0) {
     Index = -Index;
     Sign = "-";
     }
  double Seconds;
  int f = int(modf((Index + 0.5) / FramesPerSecond, &Seconds) * FramesPerSecond + 1);
  int s = int(Seconds);
  int m = s / 60 % 60;
  int h = s / 3600;
  s %= 60;
  return cString::sprintf(WithFrame ? "%s%d:%02d:%02d.%02d" : "%s%d:%02d:%02d", Sign, h, m, s, f);
}

int HMSFToIndex(const char *HMSF, double FramesPerSecond)
{
  int h, m, s, f = 1;
  int n = sscanf(HMSF, "%d:%d:%d.%d", &h, &m, &s, &f);
  if (n == 1)
     return h - 1; // plain frame number
  if (n >= 3)
     return int(round((h * 3600 + m * 60 + s) * FramesPerSecond)) + f - 1;
  return 0;
}

cMark::cMark(int Position, const char *Comment, double FramesPerSecond)
{
  position = Position;
  comment = Comment;
  framesPerSecond = FramesPerSecond;
}

cMark::~cMark()
{
}

cString cMark::ToText(void)
{
  return cString::sprintf("%s%s%s\n", *IndexToHMSF(position, true, framesPerSecond), Comment() ? " " : "", Comment() ? Comment() : "");
}

bool cMark::Parse(const char *s)
{
  comment = NULL;
  framesPerSecond = MarkFramesPerSecond;
  position = HMSFToIndex(s, framesPerSecond);
  const char *p = strchr(s, ' ');
  if (p) {
     p = skipspace(p);
     if (*p)
        comment = strdup(p);
     }
  return true;
}

bool cMark::Save(FILE *f)
{
  return fprintf(f, "%s", *ToText()) > 0;
}
