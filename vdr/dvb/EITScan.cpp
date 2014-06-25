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
#include "devices/Transfer.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "dvb/filters/PSIP_MGT.h"
#include "epg/Event.h"
#include "epg/Schedules.h"
#include "settings/Settings.h"
#include "utils/log/Log.h"

#include <algorithm>
#include <stdlib.h>
#include <unistd.h> // for sleep()

using namespace PLATFORM;
using namespace std;

namespace VDR
{

#define SCAN_TRANSPONDER_TIMEOUT_MS (20 * 1000)

// --- cScanList -------------------------------------------------------------

void cScanList::AddTransponder(const ChannelPtr& channel)
{
  if (channel->Source() != SOURCE_TYPE_NONE && channel->TransponderFrequencyMHz() != 0)
  {
    for (vector<cScanData>::const_iterator it = m_list.begin(); it != m_list.end(); ++it)
    {
      if (it->GetChannel()->Source() == channel->Source() &&
          ISTRANSPONDER(it->GetChannel()->TransponderFrequencyMHz(), channel->TransponderFrequencyMHz()))
        return;
    }
    m_list.push_back(cScanData(channel));
  }
}

// --- cEITScanner -----------------------------------------------------------

cEITScanner EITScanner;

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

    SaveEPGData();

    CreateScanList();

    vector<cScanData>::iterator it;
    for (it = m_scanList.m_list.begin(); it != m_scanList.m_list.end(); ++it)
    {
      ScanTransponder(*it);
      if (IsStopped())
        break;
    }

    cChannelManager::Get().Save();

    SaveEPGData();

    // This assumes we finished before m_nextFullScan timed out
    uint32_t durationSec = (cSettings::Get().m_iEPGScanTimeout * 60) -
                           (m_nextFullScan.TimeLeft() / 1000);

    dsyslog("EIT scan finished in %umin %usec", durationSec / 60, durationSec % 60);
  }

  m_scanList.Clear();

  return NULL;
}

void cEITScanner::ForceScan(void)
{
  m_nextFullScan.Init(0);
}

void cEITScanner::CreateScanList(void)
{
  m_scanList.Clear();

  for (ChannelVector::const_iterator it = m_transponderList.begin(); it != m_transponderList.end(); ++it)
    m_scanList.AddTransponder(*it);

  m_transponderList.clear();

  const ChannelVector& channels = cChannelManager::Get().GetCurrent();
  for (ChannelVector::const_iterator it = channels.begin(); it != channels.end(); it++)
    m_scanList.AddTransponder(*it);
}

bool cEITScanner::Scan(const DevicePtr& device, cScanData& scanData)
{
  assert(device);

  ChannelPtr channel = scanData.GetChannel();
  assert(channel);
  try
  {
    if (channel->GetCaId(0) &&
        channel->GetCaId(0) != device->CardIndex() &&
        channel->GetCaId(0) < CA_ENCRYPTED_MIN)
    {
      throw "Failed to scan channel: Channel cannot be decrypted";
    }

    if (!device->Channel()->ProvidesTransponder(*channel))
      throw "Failed to scan channel: Channel is not provided by device";

    if (device->Receiver()->Priority() >= 0)
      throw "Failed to scan channel: Some weird priority thing left over from VDR";

    if (!device->Channel()->MaySwitchTransponder(*channel) && !device->Channel()->ProvidesTransponderExclusively(*channel))
      throw "Failed to scan channel: Not allowed to switch channels";

    if (!device->Channel()->SwitchChannel(channel))
      throw "Failed to scan channel: Failed to switch channels";

    dsyslog("EIT scan: device %d source %s tp %5d MHz", device->CardIndex(), channel->Source().ToString().c_str(), channel->TransponderFrequencyMHz());

    EventVector events;

    cPsipMgt mgt(device.get());
    if (!mgt.GetPSIPData(events))
      throw "Failed to scan channel: Tuner failed to get lock";

    if (events.empty())
      throw "Scanned channel, but no events discovered!";

    // Finally, success! Add channels to EPG schedule
    for (EventVector::const_iterator it = events.begin(); it != events.end(); ++it)
      cSchedulesLock::AddEvent(channel->GetChannelID(), *it);
  }
  catch (const char* errorMsg)
  {
    dsyslog("%s", errorMsg);
    return false;
  }

  return true;
}

void cEITScanner::SaveEPGData(void)
{
  cSchedulesLock SchedulesLock(true);
  cSchedules *s = SchedulesLock.Get();
  if (s)
    s->Save();
}

bool cEITScanner::ScanTransponder(cScanData& scanData)
{
  // Look for a device whose channel provides EIT information
  for (DeviceVector::const_iterator it = cDeviceManager::Get().Iterator(); cDeviceManager::Get().IteratorHasNext(it); cDeviceManager::Get().IteratorNext(it))
  {
    if (*it && (*it)->Channel()->ProvidesEIT())
    {
      if (Scan(*it, scanData))
        return true;
    }
  }
  return false;
}

}
