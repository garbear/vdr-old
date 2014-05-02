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

#include "Mark.h"
#include "Config.h"

#include <string>
#include <time.h>

namespace VDR
{
class cMarks
{
public:
  bool Load(const std::string& strRecordingFileName, double FramesPerSecond = DEFAULTFRAMESPERSECOND, bool IsPesRecording = false);
  bool Update(void);
  bool Save(void);
  void Align(void);
  void Sort(void);
  void Add(int Position);
  size_t Size(void) const { return m_marks.size(); }
  bool Empty(void) const { return m_marks.empty(); }
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
private:
  std::string         recordingFileName;
  std::string         fileName;
  double              framesPerSecond;
  bool                isPesRecording;
  time_t              nextUpdate;
  time_t              lastFileTime;
  time_t              lastChange;
  std::vector<cMark*> m_marks;
};
}
