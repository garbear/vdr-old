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

#include "DirectoryFetchJob.h"
#include "IDirectory.h"

using namespace PLATFORM;
using namespace std;

namespace VDR
{

CDirectoryFetchJob::CDirectoryFetchJob(IDirectory *dir, const string &strPath, IDirectoryCallback *callback)
 : m_dir(dir),
   m_strPath(strPath),
   m_progress(0),
   m_callback(callback)
{
  CreateThread(false);
}

void *CDirectoryFetchJob::Process()
{
  DirectoryListing items;
  bool bSuccess = DoFetch(m_dir, m_strPath, items);

  SetProgress(100);

  if (m_callback)
    m_callback->OnDirectoryFetchComplete(bSuccess, items);

  return NULL;
}

void CDirectoryFetchJob::Abort(int iWaitMs)
{
  StopThread(iWaitMs);
}

unsigned int CDirectoryFetchJob::GetProgress()
{
  CLockObject lock(m_progressMutex);
  return m_progress;
}

void CDirectoryFetchJob::SetProgress(unsigned int progress)
{
  CLockObject lock(m_progressMutex);
  m_progress = progress;
}

}
