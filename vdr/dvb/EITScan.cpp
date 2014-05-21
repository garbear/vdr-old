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

#include "EITScan.h"
#include "channels/ChannelManager.h"
#include "devices/DeviceManager.h"
#include "epg/Schedules.h"
#include "settings/Settings.h"
#include "utils/log/Log.h"

#include <algorithm>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h> // for sleep()

using namespace PLATFORM;
using namespace std;

namespace VDR
{

#define SCAN_TRANSPONDER_TIMEOUT_MS (20 * 1000)

cEITScanner::cEITScanner(void)
 : m_nextFullScan(0),
   m_bScanFinished(false)
{
}

cEITScanner& cEITScanner::Get()
{
  static cEITScanner instance;
  return instance;
}

void cEITScanner::ForceScan(void)
{
  m_nextFullScan.Init(0);
}

void* cEITScanner::Process(void)
{
  {
    cSchedulesLock lock(true);
    cSchedules* schedules = lock.Get();
    if (schedules)
      schedules->Read();
  }

  m_nextFullScan.Init(0);

  while (!IsStopped())
  {
    // TODO: Wait on an event instead of VDR's timeout strategy
    while (m_nextFullScan.TimeLeft())
    {
      sleep(5); // 5 seconds
      if (IsStopped())
        return NULL;
    }

    m_nextFullScan.Init(cSettings::Get().m_iEPGScanTimeout * 1000 * 60);
    dsyslog("EIT scan started, next full scan in %d minutes", cSettings::Get().m_iEPGScanTimeout);

    // Save EPG data
    {
      cSchedulesLock SchedulesLock(true);
      cSchedules *s = SchedulesLock.Get();
      if (s)
        s->Save();
    }

    // Create scan list
    const ChannelVector& channels = cChannelManager::Get().GetCurrent();
    for (ChannelVector::const_iterator it = channels.begin(); it != channels.end(); it++)
      AddTransponder(*it);

    ChannelVector::iterator it;
    for (it = m_scanList.begin(); it != m_scanList.end(); ++it)
    {
      cDeviceManager::Get().ScanTransponder(*it);
      if (IsStopped())
        break;
    }

    cChannelManager::Get().Save();

    // Save EPG data
    {
      cSchedulesLock SchedulesLock(true);
      cSchedules *s = SchedulesLock.Get();
      if (s)
        s->Save();
    }

    // This assumes we finished before m_nextFullScan timed out
    uint32_t durationSec = (cSettings::Get().m_iEPGScanTimeout * 60) -
                           (m_nextFullScan.TimeLeft() / 1000);

    dsyslog("EIT scan finished in %umin %usec", durationSec / 60, durationSec % 60);
  }

  m_scanList.clear();

  return NULL;
}

void cEITScanner::AddTransponder(const ChannelPtr& transponder)
{
  assert(transponder->Source() != SOURCE_TYPE_NONE);
  assert(transponder->FrequencyMHzWithPolarization() != 0);

  bool bFound = false;

  for (ChannelVector::const_iterator it = m_scanList.begin(); !bFound && it != m_scanList.end(); ++it)
  {
    if (transponder->Source() == (*it)->Source() &&
        ISTRANSPONDER(transponder->FrequencyMHzWithPolarization(), (*it)->FrequencyMHzWithPolarization()))
    {
      bFound = true;
    }
  }

  if (!bFound)
    m_scanList.push_back(transponder);
}

}
