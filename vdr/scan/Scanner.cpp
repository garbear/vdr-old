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

#include "Scanner.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/linux/DVBDevice.h" // TODO: Remove me
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "dvb/filters/PAT.h"
#include "dvb/filters/PSIP_VCT.h"
#include "epg/EPGScanner.h"
#include "lib/platform/util/timeutils.h"
#include "transponders/TransponderFactory.h"
#include "utils/log/Log.h"

#include <assert.h>
#include <memory> // TODO: Remove me
#include <vector>

using namespace PLATFORM;
using namespace std;

namespace VDR
{

cScanner::cScanner(void)
 : m_frequencyHz(0),
   m_number(0),
   m_percentage(0.0f)
{
}

bool cScanner::Start(const cScanConfig& setup)
{
  assert(setup.device.get() != NULL);

  if (!IsRunning())
  {
    m_setup = setup;
    CreateThread(true);
    return true;
  }

  return false;
}

void cScanner::Stop(bool bWait)
{
  StopThread(bWait ? 0 : -1);
}

void* cScanner::Process()
{
  m_percentage = 0.0f;

  cTransponderFactory* transponders = NULL;

  shared_ptr<cDvbDevice> dvbDevice = dynamic_pointer_cast<cDvbDevice>(m_setup.device);
  if (!dvbDevice)
    return NULL;
  const fe_caps_t caps = dvbDevice->m_dvbTuner.Capabilities();

  if (m_setup.device->Channel()->ProvidesSource(TRANSPONDER_ATSC))
  {
    dsyslog("Scanning ATSC frequencies");
    transponders = new cAtscTransponderFactory(caps, m_setup.atscModulation);
  }
  else if (m_setup.device->Channel()->ProvidesSource(TRANSPONDER_CABLE))
  {
    dsyslog("Scanning DVB-C frequencies");
    transponders = new cCableTransponderFactory(caps, m_setup.dvbcSymbolRate);
  }
  else if (m_setup.device->Channel()->ProvidesSource(TRANSPONDER_SATELLITE))
  {
    dsyslog("Scanning DVB-S frequencies");
    transponders = new cSatelliteTransponderFactory(caps, m_setup.satelliteIndex);
  }
  else if (m_setup.device->Channel()->ProvidesSource(TRANSPONDER_TERRESTRIAL))
  {
    dsyslog("Scanning DVB-T frequencies");
    transponders = new cTerrestrialTransponderFactory(caps);
  }

  if (!transponders)
    return NULL;

  const int64_t startMs = GetTimeMs();
  ChannelPtr channel = ChannelPtr(new cChannel);

  while (!IsStopped() && transponders->HasNext())
  {
    cTransponder transponder = transponders->GetNext();

    m_frequencyHz = transponder.FrequencyHz();
    m_number      = transponder.ChannelNumber();

    channel->SetTransponder(transponder);

    TunerHandlePtr newHandle = dvbDevice->Acquire(channel, TUNING_TYPE_CHANNEL_SCAN, this);
    if (newHandle)
    {
      bool bSuccess = m_setup.device->Scan()->WaitForTransponderScan();
      cChannelManager::Get().NotifyObservers();
      if (bSuccess)
        bSuccess = m_setup.device->Scan()->WaitForEPGScan();
      newHandle->Release();
      dsyslog("%s %d MHz", bSuccess ? "Successfully scanned" : "Failed to scan", transponder.FrequencyMHz());
    }
    else
    {
      if (!dvbDevice->CanTune(TUNING_TYPE_CHANNEL_SCAN))
      {
        isyslog("Scan aborted: another subscription is using the tuner");
        break;
      }
      else
      {
        dsyslog("%s %d MHz", "Failed to scan", transponder.FrequencyMHz());
      }
    }

    m_percentage += (1.0f / transponders->TransponderCount()) * 100.0f;
  }

  m_percentage = 100.0f;

  const int64_t durationSec = (GetTimeMs() - startMs) / 1000;
  isyslog("Channel scan took %d min %d sec", durationSec / 60, durationSec % 60);
  cEPGScanner::Get().Start();

  return NULL;
}

void cScanner::LockAcquired(void)
{
}

void cScanner::LockLost(void)
{
}

void cScanner::LostPriority(void)
{
  isyslog("Scan aborted: another subscription is using the tuner");
  StopThread(-1);
}

}
