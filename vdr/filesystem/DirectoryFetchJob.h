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

#include "IDirectory.h"

#include "lib/platform/threads/threads.h"
#include "lib/platform/threads/mutex.h"

#include <string>

namespace VDR
{
class CDirectoryFetchJob : public PLATFORM::CThread
{
public:
  /*!
   * \brief Construct a new job and start the thread immediately
   *
   * The job can be aborted via Abort().
   */
  CDirectoryFetchJob(IDirectory *dir, const std::string &strPath, IDirectoryCallback *callback);

  /*!
   * \brief Main thread for directory fetch jobs
   *
   * Subclasses should override DoFetch(), which is called automatically by Process().
   */
  virtual void *Process();

  /*!
   * \brief Abort the directory fetch job
   * \param iWaitMs The amount of ms to wait. negative = don't wait, 0 = infinite
   */
  void Abort(int iWaitMs);

  /*!
   * \brief Get the progress of this job
   *
   * The progress of an unstarted job is always 0, and the progress of a
   * completed job is always 100 right before the callback is fired.
   *
   * \return The Progress as a percent
   */
  unsigned int GetProgress();

protected:
  /*!
   * \brief Override with the directory retrieval implementation.
   *
   * DoFetch() can call SetProgress() to record its progress. A directory fetch
   * job can check to see if Abort() has been called via IsStopped().
   *
   * \param items The vector of items to fill with the directory listing
   * \return True on success, false on failure or abort (items can be non-empty)
   */
  virtual bool DoFetch(IDirectory *dir, const std::string &strPath, DirectoryListing &items) = 0;

  /*!
   * \brief Set the progress of this job
   */
  void SetProgress(unsigned int progress);

private:
  IDirectory         *m_dir;
  std::string         m_strPath;
  unsigned int        m_progress;
  IDirectoryCallback *m_callback;
  PLATFORM::CMutex    m_progressMutex;
};
}
