/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "recordings/RecordingConfig.h" // for DEFAULTFRAMESPERSECOND
#include "lib/platform/threads/mutex.h"
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
