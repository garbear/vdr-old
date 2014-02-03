#include "Mark.h"
#include "platform/threads/mutex.h"

double MarkFramesPerSecond = DEFAULTFRAMESPERSECOND;
PLATFORM::CMutex MutexMarkFramesPerSecond;

cMark::cMark(int iPosition, const std::string& strComment /* = "" */, double dFramesPerSecond /* = DEFAULTFRAMESPERSECOND */)
{
  m_iPosition        = iPosition;
  m_strComment       = strComment;
  m_dFramesPerSecond = dFramesPerSecond;
}

cMark::~cMark(void)
{
}

std::string cMark::Serialise(void)
{
  return *cString::sprintf("%s%s%s\n", IndexToHMSF(m_iPosition, true, m_dFramesPerSecond).c_str(), !m_strComment.empty() ? " " : "", !m_strComment.empty() ? m_strComment.c_str() : "");
}

bool cMark::Deserialise(const std::string& strData)
{
  m_strComment.clear();
  m_dFramesPerSecond = MarkFramesPerSecond;
  m_iPosition = HMSFToIndex(strData.c_str(), m_dFramesPerSecond);
  const char *p = strchr(strData.c_str(), ' ');
  if (p)
  {
    p = skipspace(p);
    if (*p)
      m_strComment = p;
  }
  return true;
}

bool cMark::Save(FILE *f)
{
  return fprintf(f, "%s", Serialise().c_str()) > 0;
}

int cMark::Position(void) const
{
  return m_iPosition;
}

std::string cMark::Comment(void) const
{
  return m_strComment;
}

void cMark::SetPosition(int Position)
{
  m_iPosition = Position;
}

void cMark::SetComment(const std::string& strComment)
{
  m_strComment = strComment;
}

std::string cMark::IndexToHMSF(int Index, bool WithFrame, double FramesPerSecond)
{
  const char *Sign = "";
  if (Index < 0)
  {
    Index = -Index;
    Sign = "-";
  }
  double Seconds;
  int f = int(
      modf((Index + 0.5) / FramesPerSecond, &Seconds) * FramesPerSecond + 1);
  int s = int(Seconds);
  int m = s / 60 % 60;
  int h = s / 3600;
  s %= 60;
  return *cString::sprintf(WithFrame ? "%s%d:%02d:%02d.%02d" : "%s%d:%02d:%02d", Sign, h, m, s, f);
}


int cMark::HMSFToIndex(const char *HMSF, double FramesPerSecond)
{
  int h, m, s, f = 1;
  int n = sscanf(HMSF, "%d:%d:%d.%d", &h, &m, &s, &f);
  if (n == 1)
    return h - 1; // plain frame number
  if (n >= 3)
    return int(round((h * 3600 + m * 60 + s) * FramesPerSecond)) + f - 1;
  return 0;
}
