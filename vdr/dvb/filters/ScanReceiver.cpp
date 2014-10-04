/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "ScanReceiver.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "utils/log/Log.h"

using namespace std;

namespace VDR
{

cScanReceiver::cScanReceiver(cDevice* device, uint16_t pid) :
    m_device(device),
    m_locked(false),
    m_scanned(false)
{
  m_pids.push_back(pid);
}

cScanReceiver::cScanReceiver(cDevice* device, const std::vector<uint16_t>& pids) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_pids(pids)
{
}

cScanReceiver::cScanReceiver(cDevice* device, size_t nbPids, const uint16_t* pids) :
    m_device(device),
    m_locked(false),
    m_scanned(false)
{
  for (size_t ptr = 0; ptr < nbPids; ++ptr)
    m_pids.push_back(pids[ptr]);
}

bool cScanReceiver::Attach(void)
{
  bool retval = true;
  for (std::vector<uint16_t>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
    retval &= m_device->Receiver()->AttachReceiver(this, *it);
  return retval;
}

bool cScanReceiver::WaitForScan(uint32_t iTimeout /* = TRANSPONDER_TIMEOUT */)
{
  PLATFORM::CLockObject lock(m_mutex);
  return m_scannedEvent.Wait(m_mutex, m_scanned, iTimeout);
}

void cScanReceiver::LockAcquired(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  m_locked  = true;
  m_scanned = false;
}

void cScanReceiver::LockLost(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  m_locked = false;
  m_scanned = false;
}

}
