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
#include "devices/Remux.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "utils/log/Log.h"
#include <algorithm>

using namespace std;

namespace VDR
{

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, uint16_t pid) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
  m_pids.insert(pid);
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, const std::vector<uint16_t>& pids) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
  for (std::vector<uint16_t>::const_iterator it = pids.begin(); it != pids.end(); ++it)
    m_pids.insert(*it);
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, size_t nbPids, const uint16_t* pids) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
  for (size_t ptr = 0; ptr < nbPids; ++ptr)
    m_pids.insert(pids[ptr]);
}

bool cScanReceiver::Attach(void)
{
  bool retval = true;
  PLATFORM::CLockObject lock(m_mutex);

  if (m_attached)
    return true;

  m_attached = true;
  for (std::set<uint16_t>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
    m_attached &= m_device->Receiver()->AttachReceiver(this, *it);

  dsyslog(m_attached ? "%s filter attached" : "failed to attach %s filter", m_name.c_str());

  return m_attached;
}

void cScanReceiver::Detach(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  RemovePids();
  if (m_attached)
  {
    m_attached = false;
    dsyslog("%s filter detached", m_name.c_str());
  }
}

bool cScanReceiver::WaitForScan(uint32_t iTimeout /* = TRANSPONDER_TIMEOUT */)
{
  PLATFORM::CLockObject lock(m_scannedmutex);
  if (m_pids.empty())
    return true;
  return m_scannedEvent.Wait(m_scannedmutex, m_scanned, iTimeout);
}

void cScanReceiver::LockAcquired(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  m_locked  = true;
  m_scanned = false;
}

void cScanReceiver::LockLost(void)
{
  {
    PLATFORM::CLockObject lock(m_mutex);
    m_locked = false;
    m_scanned = false;
  }
  Detach();
}

void cScanReceiver::Receive(const uint16_t pid, const uint8_t* data, const size_t len)
{
  ReceivePacket(pid, data + 1);
}

void cScanReceiver::AddPid(uint16_t pid)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (m_pids.find(pid) != m_pids.end())
    return;
  if (m_device->Receiver()->AttachReceiver(this, pid))
  {
    m_pidsAdded.insert(pid);
    m_pids.insert(pid);
  }
}

void cScanReceiver::RemovePid(uint16_t pid)
{
  PLATFORM::CLockObject lock(m_mutex);
  std::set<uint16_t>::iterator it = m_pids.find(pid);
  if (it != m_pids.end())
  {
    if (DynamicPid(pid))
    {
      m_pidsAdded.erase(pid);
      m_pids.erase(it);
    }

    if (m_attached)
      m_device->Receiver()->DetachReceiverPid(this, pid);
  }
}

void cScanReceiver::RemovePids(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  std::vector<uint16_t> pidsRemoved;

  for (std::set<uint16_t>::iterator it = m_pids.begin(); it != m_pids.end(); ++it)
  {
    m_device->Receiver()->DetachReceiverPid(this, *it);

    if (DynamicPid(*it))
      pidsRemoved.push_back(*it);
  }

  for (std::vector<uint16_t>::const_iterator it = pidsRemoved.begin(); it != pidsRemoved.end(); ++it)
    m_pids.erase(*it);
}

void cScanReceiver::SetScanned(void)
{
  {
    PLATFORM::CLockObject lock(m_scannedmutex);
    if (!m_scanned)
    {
      dsyslog("%s scan completed", m_name.c_str());
      m_scanned = true;
    }
    m_scannedEvent.Broadcast();
  }
  m_device->Scan()->SetScanned(this);
}

void cScanReceiver::ResetScanned(void)
{
  PLATFORM::CLockObject lock(m_scannedmutex);
  m_scanned = false;
}

bool cScanReceiver::Scanned(void) const
{
  PLATFORM::CLockObject lock(m_scannedmutex);
  return m_scanned;
}

bool cScanReceiver::DynamicPid(uint16_t pid) const
{
  return m_pidsAdded.find(pid) != m_pidsAdded.end();
}

}
