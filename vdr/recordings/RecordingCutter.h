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

#include "platform/threads/threads.h"

#include <string>

namespace VDR
{
class cCuttingThread;

class cCutter {
private:
  static PLATFORM::CMutex mutex;
  static std::string originalVersionName;
  static std::string editedVersionName;
  static cCuttingThread *cuttingThread;
  static bool error;
  static bool ended;
public:
  static bool Start(const char *FileName);
  static void Stop(void);
  static bool Active(const char *FileName = NULL);
         ///< Returns true if the cutter is currently active.
         ///< If a FileName is given, true is only returned if either the
         ///< original or the edited file name is equal to FileName.
  static bool Error(void);
  static bool Ended(void);
  };

bool CutRecording(const char *FileName);
}
