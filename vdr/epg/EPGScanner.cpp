/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "EPGScanner.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "devices/TunerHandle.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "epg/ScheduleManager.h"
#include "utils/log/Log.h"


#include <assert.h>
#include <memory> // TODO: Remove me
#include <vector>

using namespace PLATFORM;
using namespace std;

#define EPG_SCAN_INTERVAL_MS (3000000)

namespace VDR
{

cEPGScanner& cEPGScanner::Get(void)
{
  static cEPGScanner _instance;
  return _instance;
}

cEPGScanner::cEPGScanner(void) :
    m_scanTriggered(false)
{
  CreateThread(true);
}

bool cEPGScanner::Start(void)
{
  {
    CLockObject lock(m_mutex);
    m_scanTriggered = true;
    m_condition.Signal();
  }

  if (!IsRunning())
  {
    CreateThread(true);
    return true;
  }

  return false;
}

void cEPGScanner::Stop(bool bWait)
{
  StopThread(-1);
  {
    CLockObject lock(m_mutex);
    m_scanTriggered = true;
    m_condition.Signal();
  }
  if (bWait)
    StopThread(0);
}

void* cEPGScanner::Process()
{
  std::list<cTransponder> transponders;
  ChannelPtr dummyChannel = ChannelPtr(new cChannel());
  DevicePtr device;
  while (!IsStopped())
  {
    {
      CLockObject lock(m_mutex);
      m_condition.Wait(m_mutex, m_scanTriggered, EPG_SCAN_INTERVAL_MS);
      if (IsStopped() || !m_scanTriggered)
        continue;
    }

    // TODO get a free tuner, not the first one
    device = cDeviceManager::Get().GetDevice(0);
    transponders = cChannelManager::Get().GetCurrentTransponders();
    if (transponders.empty() || !device)
    {
      Sleep(1000);
      continue;
    }

    for (std::list<cTransponder>::iterator it = transponders.begin(); it != transponders.end() && !IsStopped(); ++it)
    {
      dummyChannel->SetTransponder(*it);
      TunerHandlePtr newHandle = device->Acquire(dummyChannel, TUNING_TYPE_EPG_SCAN, this);
      if (newHandle)
      {
        device->Scan()->AttachReceivers();
        device->Scan()->WaitForEPGScan();
        cScheduleManager::Get().NotifyObservers();
        newHandle->Release();
      }
      else
      {
        /** continue again in a second when the tuner is busy */
        while (!IsStopped() && !device->CanTune(TUNING_TYPE_EPG_SCAN))
          Sleep(1000);
      }
    }
  }

  return NULL;
}

void cEPGScanner::LockAcquired(void)
{
}

void cEPGScanner::LockLost(void)
{
  cDeviceManager::Get().GetDevice(0)->Scan()->DetachReceivers();
}

void cEPGScanner::LostPriority(void)
{
  Stop(false);
}

}
