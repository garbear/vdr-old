/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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
#include "devices/linux/DVBDevice.h"
#include "utils/SynchronousAbort.h"
#include "utils/Tools.h"

#include <assert.h>

using namespace PLATFORM;
using namespace VDR::SATELLITE;
using namespace std;

namespace VDR
{

cScanner::cScanner(cDevice* device, const cScanConfig& setup)
 : m_device(device),
   m_setup(setup),
   m_abortableJob(NULL)
{
}

bool cScanner::GetFrontendType(eDvbType dvbType, fe_type& frontendType)
{
  switch (dvbType)
  {
  case DVB_TERR:  frontendType = FE_OFDM; break;
  case DVB_CABLE: frontendType = FE_QAM;  break;
  case DVB_SAT:   frontendType = FE_QPSK; break;
  case DVB_ATSC:  frontendType = FE_ATSC; break; // frontendType not actually used if scanType == DVB_ATSC
  default:
    dsyslog("Invalid scan type!!! %d", dvbType);
    return false;
  }
  return true;
}

void* cScanner::Process()
{
  {
    CLockObject lock(m_mutex);
    if (m_abortableJob)
      return NULL; // Scan already in progress
  }

  fe_type frontendType;
  if (!GetFrontendType(m_setup.dvbType, frontendType))
    return NULL;

  eChannelList channelList;
  switch (m_setup.dvbType)
  {
  case DVB_TERR:
  case DVB_CABLE:
  case DVB_ATSC:
    if (!CountryUtils::GetChannelList(m_setup.countryIndex, frontendType, m_setup.atscModulation, channelList))
      return NULL;
    break;
  case DVB_SAT:
    break; // Uses satellite ID instead of channel list
  }

  // Calculate scan space
  cScanLimits* scanLimits;
  switch (m_setup.dvbType)
  {
  case DVB_TERR:  scanLimits = new cScanLimitsTerrestrial();                     break;
  case DVB_CABLE: scanLimits = new cScanLimitsCable(m_setup.dvbcSymbolRate);     break;
  case DVB_SAT:   scanLimits = new cScanLimitsSatellite(m_setup.satelliteIndex); break;
  case DVB_ATSC:  scanLimits = new cScanLimitsATSC(m_setup.atscModulation);      break;
  }

  // Calculate frontend capabilities
  cDvbDevice* device = dynamic_cast<cDvbDevice*>(m_device);
  if (!device)
    return NULL;
  cFrontendCapabilities caps(device->m_dvbTuner.GetCapabilities(), m_setup.dvbType == DVB_TERR ? m_setup.dvbtInversion : m_setup.dvbcInversion);

  cScanTask* task;
  switch (m_setup.dvbType)
  {
  case DVB_TERR:  task = new cScanTaskTerrestrial(m_device, caps, channelList);          break;
  case DVB_CABLE: task = new cScanTaskCable(m_device, caps, channelList);                break;
  case DVB_SAT:   task = new cScanTaskSatellite(m_device, caps, m_setup.satelliteIndex); break;
  case DVB_ATSC:  task = new cScanTaskATSC(m_device, caps, channelList);                 break;
  default: return NULL;
  }

  {
    CLockObject lock(m_mutex);
    m_abortableJob = scanLimits;
  }

  scanLimits->ForEach(task);

  {
    CLockObject lock(m_mutex);
    m_abortableJob = NULL;
  }

  delete task;
  delete scanLimits;

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
