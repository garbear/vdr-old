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

#include "utils/SynchronousAbort.h"

#include <gtest/gtest.h>

#include <platform/threads/threads.h>

using namespace PLATFORM;

namespace VDR
{

#define JIFFY_MS  100 // A small amount of time

// --- cAbortableJob -----------------------------------------------------------

class cAbortableJob : public cSynchronousAbort, public CThread
{
public:
  /*!
   * \brief Create a job that lasts the specified number of jiffies
   */
  cAbortableJob(unsigned int jiffies) : m_jiffies(jiffies) { }
  virtual ~cAbortableJob() { }

  virtual void* Process()
  {
    usleep(m_jiffies * JIFFY_MS * 1000);
    Finished();
    return NULL;
  }

private:
  unsigned int m_jiffies;
};

// --- cAbortJob --------------------------------------------------------------

class cAbortJob : public CThread
{
public:
  /*!
   * \brief Wait for specified jiffies then abort the job
   */
  cAbortJob(cAbortableJob* abortableJob, unsigned int jiffies) : m_abortableJob(abortableJob), m_jiffies(jiffies) { }
  virtual ~cAbortJob() { }

  virtual void* Process()
  {
    usleep(m_jiffies * JIFFY_MS * 1000);
    m_abortableJob->Abort(false);
    return NULL;
  }

private:
  cAbortableJob* m_abortableJob;
  unsigned int m_jiffies;
};


TEST(SynchronousAbort, WaitForAbort)
{
  // Create a thread lasting 3 jiffies
  cAbortableJob abortableJob(3);

  // Test initial state: not aborted
  EXPECT_TRUE(abortableJob.CreateThread(true));
  EXPECT_FALSE(abortableJob.IsAborting());
  EXPECT_FALSE(abortableJob.IsAborted());

  // Wait 2 jiffies asynchronously then abort thread
  cAbortJob abortJob(&abortableJob, 2);
  EXPECT_TRUE(abortJob.CreateThread(true));

  // Thread is still running after 1 jiffy
  EXPECT_FALSE(abortableJob.WaitForAbort(JIFFY_MS));
  EXPECT_FALSE(abortableJob.IsAborting());
  EXPECT_FALSE(abortableJob.IsAborted());

  // Thread should be aborted after another jiffy
  EXPECT_TRUE(abortableJob.WaitForAbort());
  EXPECT_TRUE(abortableJob.IsAborting());
  EXPECT_FALSE(abortableJob.IsAborted());

  // Wait 1 jiffy for thread to finish
  usleep(JIFFY_MS * 1000);

  // Thread must be aborted by now
  usleep(JIFFY_MS * 1000);
  EXPECT_TRUE(abortableJob.IsAborted());

  // Should return true immediately as job is already aborted
  EXPECT_TRUE(abortableJob.WaitForAbort());
}

TEST(SynchronousAbort, Abort)
{
  // Create a thread lasting 3 jiffies
  cAbortableJob abortableJob(3);

  // Test initial state: not aborted
  EXPECT_TRUE(abortableJob.CreateThread(true));
  EXPECT_FALSE(abortableJob.IsAborting());
  EXPECT_FALSE(abortableJob.IsAborted());

  // Abort the job, but don't wait for it to finish
  abortableJob.Abort(false);
  EXPECT_TRUE(abortableJob.IsAborting());
  EXPECT_FALSE(abortableJob.IsAborted());

  // Now wait for it to finish
  abortableJob.Abort(true);
  EXPECT_TRUE(abortableJob.IsAborted());
}

}
