/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include <sys/poll.h>

namespace VDR
{
class cPoller
{
public:
  cPoller(int fileHandle = -1, bool bOut = false, bool bPriorityOnly = false);

  bool Add(int fileHandle, bool bOut, bool bPriorityOnly = false);

  bool Poll(int timeoutMs = 0);

private:
  enum { MaxPollFiles = 16 };

  pollfd m_pfd[MaxPollFiles];
  int    m_numFileHandles;
};
}
