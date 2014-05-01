#pragma once

#include "recordings/RecordingConfig.h" // for DEFAULTFRAMESPERSECOND
#include "platform/threads/mutex.h"
#include "utils/List.h"

#include <string>

class TiXmlNode;

namespace VDR
{
extern double MarkFramesPerSecond;
extern PLATFORM::CMutex MutexMarkFramesPerSecond;

class cMark : public cListObject
{
  friend class cMarks; // for sorting

public:
  cMark(int iPosition = 0, const std::string& strComment = "", double dFramesPerSecond = DEFAULTFRAMESPERSECOND);
  virtual ~cMark(void);

  int Position(void) const;
  void SetPosition(int Position);

  std::string Comment(void) const;
  void SetComment(const std::string& strComment);

  bool Deserialise(const TiXmlNode *node);
  bool Serialise(TiXmlNode *node) const;

  /**
   * Converts the given index to a string, optionally containing the frame number.
   */
  static std::string IndexToHMSF(int Index, bool WithFrame = false, double FramesPerSecond = DEFAULTFRAMESPERSECOND);

  /**
   * Converts the given string (format: "hh:mm:ss.ff") to an index.
   */
  static int HMSFToIndex(const char *HMSF, double FramesPerSecond = DEFAULTFRAMESPERSECOND);

private:
  double      m_dFramesPerSecond;
  int         m_iPosition;
  std::string m_strComment;
};
}
