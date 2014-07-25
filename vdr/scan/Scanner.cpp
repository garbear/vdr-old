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
#include "FrontendCapabilities.h"
#include "SatelliteUtils.h"
#include "ScanLimits.h"
#include "ScanTask.h"
#include "channels/ChannelManager.h"
#include "devices/linux/DVBDevice.h"
#include "utils/log/Log.h"
#include "utils/SynchronousAbort.h"
#include "utils/Tools.h"

#include <assert.h>

using namespace PLATFORM;
using namespace VDR::SATELLITE;
using namespace std;

namespace VDR
{

cScanner::cScanner(void)
 : m_abortableJob(NULL),
   m_percentage(0.0f),
   m_frequencyHz(0)
{
}

bool cScanner::GetFrontendType(TRANSPONDER_TYPE dvbType, fe_type& frontendType)
{
  switch (dvbType)
  {
  case TRANSPONDER_TERRESTRIAL:  frontendType = FE_OFDM; break;
  case TRANSPONDER_CABLE: frontendType = FE_QAM;  break;
  case TRANSPONDER_SATELLITE:   frontendType = FE_QPSK; break;
  case TRANSPONDER_ATSC:  frontendType = FE_ATSC; break; // frontendType not actually used if scanType == TRANSPONDER_ATSC
  default:
    dsyslog("Invalid scan type!!! %d", dvbType);
    return false;
  }
  return true;
}

bool cScanner::Start(const cScanConfig& setup)
{
  assert(setup.device.get() != NULL);

  if (!IsRunning())
  {
    m_setup = setup;
    CreateThread();
    return true;
  }

  return false;
}

void* cScanner::Process()
{
  try
  {
    fe_type_t frontendType;
    if (!GetFrontendType(m_setup.dvbType, frontendType))
      throw false;

    eChannelList channelList;
    switch (m_setup.dvbType)
    {
    case TRANSPONDER_TERRESTRIAL:
    case TRANSPONDER_CABLE:
    case TRANSPONDER_ATSC:
      if (!CountryUtils::GetChannelList(m_setup.countryIndex, frontendType, m_setup.atscModulation, channelList))
        throw false;
      break;
    case TRANSPONDER_SATELLITE:
      break; // Uses satellite ID instead of channel list
    }

    // Calculate scan space
    cScanLimits* scanLimits;
    switch (m_setup.dvbType)
    {
    case TRANSPONDER_ATSC:       scanLimits = new cScanLimitsATSC(m_setup.atscModulation);      break;
    case TRANSPONDER_CABLE:      scanLimits = new cScanLimitsCable(m_setup.dvbcSymbolRate);     break;
    case TRANSPONDER_SATELLITE:  scanLimits = new cScanLimitsSatellite(m_setup.satelliteIndex); break;
    case TRANSPONDER_TERRESTRIAL:scanLimits = new cScanLimitsTerrestrial();                     break;
    }

    // Calculate frontend capabilities
    //cDvbDevice* device = dynamic_cast<cDvbDevice*>(m_setup.device);
    cDvbDevice* device = static_cast<cDvbDevice*>(m_setup.device.get());
    if (!device)
      throw false;

    cFrontendCapabilities caps(device->m_dvbTuner.GetCapabilities(), m_setup.dvbType == TRANSPONDER_TERRESTRIAL ? m_setup.dvbtInversion : m_setup.dvbcInversion);

    cScanTask* task;
    switch (m_setup.dvbType)
    {
    case TRANSPONDER_ATSC:        task = new cScanTaskATSC(device, caps, channelList);                 break;
    case TRANSPONDER_CABLE:       task = new cScanTaskCable(device, caps, channelList);                break;
    case TRANSPONDER_SATELLITE:   task = new cScanTaskSatellite(device, caps, m_setup.satelliteIndex); break;
    case TRANSPONDER_TERRESTRIAL: task = new cScanTaskTerrestrial(device, caps, channelList);          break;
    default: throw false;
    }

    {
      CLockObject lock(m_mutex);
      m_abortableJob = scanLimits;
    }

    scanLimits->ForEach(task, m_setup, this);

    {
      CLockObject lock(m_mutex);
      m_abortableJob = NULL;
    }

    delete task;
    delete scanLimits;

    cChannelManager::Get().Save();
  }
  catch (bool bSucess)
  {
    return NULL;
  }

  return NULL;
}

void cScanner::Stop(bool bWait /* = false */)
{
  cSynchronousAbort* abortableJob;
  {
    CLockObject lock(m_mutex);
    abortableJob = m_abortableJob;
  }
  if (abortableJob)
    abortableJob->Abort(bWait);
}

}
