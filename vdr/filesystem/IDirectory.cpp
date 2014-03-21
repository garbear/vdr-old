/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "IDirectory.h"
#include "DirectoryFetchJob.h"

#include <string>

using namespace std;

namespace VDR
{

class AsynchronousFetch : public CDirectoryFetchJob
{
public:
  AsynchronousFetch(IDirectory *dir, const string &strPath, IDirectoryCallback *callback) : CDirectoryFetchJob(dir, strPath, callback) { }

protected:
  virtual bool DoFetch(IDirectory *dir, const std::string &strPath, DirectoryListing &items)
  {
    if (!dir)
      return false;
    return dir->GetDirectory(strPath, items);
  }
};

CDirectoryFetchJob *IDirectory::GetJob(const std::string &strPath, IDirectoryCallback *callback)
{
  return new AsynchronousFetch(this, strPath, callback);
}

IDirectory::~IDirectory()
{
  delete m_fetchJob;
}

void IDirectory::GetDirectoryAsync(const string &strPath, IDirectoryCallback *callback)
{
  if (m_fetchJob)
    delete m_fetchJob;
  m_fetchJob = GetJob(strPath, callback);
}

unsigned int IDirectory::GetProgress() const
{
  if (!m_fetchJob)
    return 0;
  return m_fetchJob->GetProgress();
}

void IDirectory::CancelDirectory()
{
  if (!m_fetchJob)
    return;
  m_fetchJob->Abort(-1);
}

}
