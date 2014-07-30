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
#include "transponders/TransponderFactory.h"
#include "utils/log/Log.h"

#include <assert.h>
#include <memory> // TODO: Remove me
#include <vector>

using namespace PLATFORM;
using namespace std;

// TODO: This can be shorted by fetching PMT tables in parallel
#define TRANSPONDER_TIMEOUT 5000

namespace VDR
{

cScanner::cScanner(void)
 : m_frequencyHz(0),
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

void* cScanner::Process()
{
  m_percentage = 0.0f;

  cTransponderFactory* transponders = NULL;

  // TODO: Need to get caps from cDevice class
  const DevicePtr& device = m_setup.device;
  shared_ptr<cDvbDevice> dvbDevice = dynamic_pointer_cast<cDvbDevice>(m_setup.device);
  if (!dvbDevice)
    return NULL;
  const fe_caps_t caps = dvbDevice->m_dvbTuner.GetCapabilities();

  switch (m_setup.dvbType)
  {
  case TRANSPONDER_ATSC:
    transponders = new cAtscTransponderFactory(caps, m_setup.atscModulation);
    break;
  case TRANSPONDER_CABLE:
    transponders = new cCableTransponderFactory(caps, m_setup.dvbcSymbolRate);
    break;
  case TRANSPONDER_SATELLITE:
    transponders = new cSatelliteTransponderFactory(caps, m_setup.satelliteIndex);
    break;
  case TRANSPONDER_TERRESTRIAL:
    transponders = new cTerrestrialTransponderFactory(caps);
    break;
  }

  if (!transponders)
    return NULL;

  while (transponders->HasNext())
  {
    cTransponder transponder = transponders->GetNext();
    m_frequencyHz = transponder.FrequencyHz();

    if (device->Channel()->SwitchTransponder(transponder))
    {
      bool bSuccess = device->Scan()->WaitForChannelScan(TRANSPONDER_TIMEOUT);
      dsyslog("%s %d MHz", bSuccess ? "Successfully scanned" : "Failed to scan", transponder.FrequencyMHz());
    }

    m_percentage += 1.0f / transponders->TransponderCount();
  }

  m_percentage = 100.0f;

  return NULL;
}

}
