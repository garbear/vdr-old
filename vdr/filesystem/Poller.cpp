/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "Poller.h"
#include "utils/Tools.h"

cPoller::cPoller(int fileHandle, bool bOut)
 : m_numFileHandles(0)
{
  Add(fileHandle, bOut);
}

bool cPoller::Add(int fileHandle, bool bOut)
{
  if (fileHandle >= 0)
  {
    for (int i = 0; i < m_numFileHandles; i++)
    {
      if (m_pfd[i].fd == fileHandle && m_pfd[i].events == (bOut ? POLLOUT : POLLIN))
        return true;
    }
    if (m_numFileHandles < MaxPollFiles)
    {
      m_pfd[m_numFileHandles].fd = fileHandle;
      m_pfd[m_numFileHandles].events = bOut ? POLLOUT : POLLIN;
      m_pfd[m_numFileHandles].revents = 0;
      m_numFileHandles++;
      return true;
    }
    esyslog("ERROR: too many file handles in cPoller");
  }
  return false;
}

bool cPoller::Poll(int timeoutMs)
{
  if (m_numFileHandles)
  {
    if (poll(m_pfd, m_numFileHandles, timeoutMs) != 0)
    {
      // returns true even in case of an error, to let the caller
      // access the file and thus see the error code
      return true;
    }
  }
  return false;
}
