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
  cPsiBuffer* buffer = cPsiBuffers::Get().Allocate(pid);
  if (buffer)
    m_pids.insert(make_pair(pid, buffer));
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, const std::vector<uint16_t>& pids) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false)
{
  cPsiBuffer* buffer;
  for (std::vector<uint16_t>::const_iterator it = pids.begin(); it != pids.end(); ++it)
  {
    buffer = cPsiBuffers::Get().Allocate(*it);
    if (buffer)
      m_pids.insert(make_pair(*it, buffer));
  }
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, size_t nbPids, const uint16_t* pids) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false)
{
  cPsiBuffer* buffer;
  for (size_t ptr = 0; ptr < nbPids; ++ptr)
  {
    buffer = cPsiBuffers::Get().Allocate(pids[ptr]);
    if (buffer)
      m_pids.insert(make_pair(pids[ptr], buffer));
  }
}

bool cScanReceiver::Attach(void)
{
  bool retval = true;
  PLATFORM::CLockObject lock(m_mutex);

  if (m_attached)
    return true;

  m_attached = true;
  for (std::map<uint16_t, cPsiBuffer*>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
    m_attached &= m_device->Receiver()->AttachReceiver(this, it->first);

  return m_attached;
}

void cScanReceiver::Detach(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  RemovePids();
  m_attached = false;
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

void cScanReceiver::Receive(const std::vector<uint8_t>& data)
{
  if (TsError(data.data()) || !TsHasPayload(data.data()))
    return;

  uint16_t pid = TsPid(data.data());
  std::map<uint16_t, cPsiBuffer*>::iterator it = m_pids.find(pid);
  if (it != m_pids.end())
  {
    const uint8_t* payload = it->second->AddTsData(data.data(), data.size());
    if (payload)
      ReceivePacket(pid, payload + 1);
  }
}

void cScanReceiver::AddPid(uint16_t pid)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (m_pids.find(pid) != m_pids.end())
    return;
  cPsiBuffer* buffer = cPsiBuffers::Get().Allocate(pid);
  if (buffer)
  {
    m_pids.insert(make_pair(pid, buffer));
    m_pidsAdded.insert(pid);
    m_attached &= m_device->Receiver()->AttachReceiver(this, pid);
  }
  else
  {
    m_attached = false;
  }
}

void cScanReceiver::RemovePid(uint16_t pid)
{
  PLATFORM::CLockObject lock(m_mutex);
  std::map<uint16_t, cPsiBuffer*>::iterator it = m_pids.find(pid);
  if (it != m_pids.end())
  {
    if (DynamicPid(pid))
    {
      cPsiBuffers::Get().Release(it->second);
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

  for (std::map<uint16_t, cPsiBuffer*>::iterator it = m_pids.begin(); it != m_pids.end(); ++it)
  {
    m_device->Receiver()->DetachReceiverPid(this, it->first);

    if (DynamicPid(it->first))
    {
      cPsiBuffers::Get().Release(it->second);
      pidsRemoved.push_back(it->first);
    }
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
