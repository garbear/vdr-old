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

#include "DeviceScanSubsystem.h"
#include "DeviceChannelSubsystem.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "dvb/filters/PAT.h"
#include "dvb/filters/PSIP_MGT.h"
#include "dvb/filters/PSIP_VCT.h"
#include "dvb/filters/SDT.h"
#include "epg/Schedules.h"
#include "lib/platform/util/timeutils.h"

#include <algorithm>

using namespace PLATFORM;
using namespace std;

// TODO: This can be shorted by fetching PMT tables in parallel
#define TRANSPONDER_TIMEOUT 5000

namespace VDR
{

/*
 * cSectionScanner
 */
cSectionScanner::cSectionScanner(cDevice* device, iFilterCallback* callback)
 : m_device(device),
   m_callback(callback),
   m_bSuccess(true)
{
}

bool cSectionScanner::WaitForExit(unsigned int timeoutMs)
{
  StopThread(timeoutMs);
  return m_bSuccess;
}

/*
 * cChannelPropsScanner
 */
void* cChannelPropsScanner::Process(void)
{
  m_bSuccess = false;

  cPat pat(m_device);
  m_bSuccess = pat.ScanChannels(m_callback);

  cChannelManager::Get().NotifyObservers();

  return NULL;
}

/*
 * cChannelNamesScanner
 */
void* cChannelNamesScanner::Process(void)
{
  m_bSuccess = false;

  // TODO: Use SDT for non-ATSC tuners
  cPsipVct vct(m_device);
  m_bSuccess = vct.ScanChannels(m_callback);

  if (m_bSuccess) {
    cSdt sdt(m_device);
    sdt.ScanChannels();
  }
  cChannelManager::Get().NotifyObservers();

  return NULL;
}

/*
 * cEventScanner
 */
void* cEventScanner::Process(void)
{
  m_bSuccess = false;

  cPsipMgt mgt(m_device);
  m_bSuccess = mgt.ScanPSIPData(m_callback);

  return NULL;
}

/*
 * cDeviceScanSubsystem
 */
cDeviceScanSubsystem::cDeviceScanSubsystem(cDevice* device)
 : cDeviceSubsystem(device),
   m_channelPropsScanner(device, this),
   m_channelNamesScanner(device, this),
   m_eventScanner(device, this)
{
}

void cDeviceScanSubsystem::StartScan()
{
  m_channelPropsScanner.CreateThread(true);
  m_channelNamesScanner.CreateThread(true);
  m_eventScanner.CreateThread(true);
}

bool cDeviceScanSubsystem::WaitForTransponderScan()
{
  const int64_t startMs = GetTimeMs();
  const int64_t timeoutMs = TRANSPONDER_TIMEOUT;

  if (m_channelPropsScanner.WaitForExit(timeoutMs))
  {
    const int64_t duration = GetTimeMs() - startMs;
    const int64_t timeLeft = timeoutMs - duration;
    return m_channelNamesScanner.WaitForExit(std::max(timeLeft, (int64_t)1));
  }

  return false;
}

bool cDeviceScanSubsystem::WaitForEPGScan(unsigned int timeoutMs)
{
  return m_eventScanner.WaitForExit(timeoutMs);
}

void cDeviceScanSubsystem::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageChannelLock:
  {
    StartScan();
    break;
  }
  default:
    break;
  }
}

void cDeviceScanSubsystem::OnChannelPropsScanned(const ChannelPtr& channel)
{
  cChannelManager::Get().MergeChannelProps(channel);
}

void cDeviceScanSubsystem::OnChannelNamesScanned(const ChannelPtr& channel)
{
  cChannelManager::Get().MergeChannelNamesAndModulation(channel);
}

void cDeviceScanSubsystem::OnEventScanned(const cChannelID& channelId, const EventPtr& event)
{
  cSchedulesLock::AddEvent(channelId, event);
}

}
