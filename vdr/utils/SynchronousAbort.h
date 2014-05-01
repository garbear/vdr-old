/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include <platform/threads/mutex.h>

namespace VDR
{
/*!
 * \brief Base class for jobs requiring a blocking Abort() call
 */
class cSynchronousAbort
{
public:
  cSynchronousAbort() : m_bAborting(false), m_bAborted(false) { }
  virtual ~cSynchronousAbort() { Finished(); }

  /*!
   * \brief Abort the job
   * \param bWait - If true, Abort() blocks until Finished() is called
   */
  void Abort(bool bWait);
  bool IsAborting() const { return m_bAborting; }
  bool IsAborted() const { return m_bAborted; }

  /*!
   * \brief Block until Abort() is called (doesn't wait for job to finish)
   * \param iTimeoutMs - Time to block in ms, or 0 for infinite
   * \return True if Abort() is called, false if timeout elapses
   */
  bool WaitForAbort(uint32_t iTimeoutMs = 0);

protected:
  /*!
   * \brief Call from subclass when job is finished
   */
  void Finished();

private:
  // Synchronicity is achieved using dual events
  PLATFORM::CEvent m_abortEvent; // Event is fired when job should be aborted
  PLATFORM::CEvent m_finishedEvent; // Event is fired when job is finished
  volatile bool    m_bAborting; // Abort() has been called
  volatile bool    m_bAborted; // Job was aborted and Finish() has been called by subclass
};
}
