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
#include "SectionSyncer.h"
#include "devices/Device.h"
#include "devices/Remux.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "utils/log/Log.h"
#include <algorithm>

using namespace std;

#define FILTER_DEBUGGING(x...) dsyslog(x)
//#define FILTER_DEBUGGING(x...) {}

namespace VDR
{

bool operator==(const cScanReceiver::filter_properties& lhs, const cScanReceiver::filter_properties& rhs)
{
  return rhs.pid  == lhs.pid &&
         rhs.tid  == lhs.tid &&
         rhs.mask == lhs.mask;
}

// --- cScanReceiver -----------------------------------------------------------

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, const filter_properties& filter) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
  m_filters.push_back(filter);
  m_sectionSyncers.insert(make_pair(filter.pid, new cSectionSyncer));
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, const std::vector<filter_properties>& filters) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
  for (std::vector<filter_properties>::const_iterator it = filters.begin(); it != filters.end(); ++it)
  {
    m_sectionSyncers.insert(make_pair(it->pid, new cSectionSyncer));
    m_filters.push_back(*it);
  }
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, size_t nbFilters, const filter_properties* filters) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
  for (size_t ptr = 0; ptr < nbFilters; ++ptr)
  {
    m_sectionSyncers.insert(make_pair(filters[ptr].pid, new cSectionSyncer));
    m_filters.push_back(filters[ptr]);
  }
}

bool cScanReceiver::Attach(void)
{
  bool retval = true;
  PLATFORM::CLockObject lock(m_mutex);

  if (m_attached)
    return true;

  m_attached = true;
  for (std::vector<filter_properties>::const_iterator it = m_filters.begin(); it != m_filters.end(); ++it)
    m_attached &= m_device->Receiver()->AttachStreamingReceiver(this, it->pid, it->tid, it->mask);

  if (!m_attached)
    esyslog("failed to attach %s filter", m_name.c_str());
  else
    FILTER_DEBUGGING("%s filter attached", m_name.c_str());

  return m_attached;
}

void cScanReceiver::Detach(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  RemoveFilters();
  for (std::map<uint16_t, cSectionSyncer*>::iterator it = m_sectionSyncers.begin(); it != m_sectionSyncers.end(); ++it)
    it->second->Reset();
  if (m_attached)
  {
    m_attached = false;

    for (std::vector<filter_properties>::const_iterator it = m_filters.begin(); it != m_filters.end(); ++it)
      m_device->Receiver()->DetachReceiverPid(this, it->pid);

    FILTER_DEBUGGING("%s filter detached", m_name.c_str());
  }
}

bool cScanReceiver::WaitForScan(uint32_t iTimeout /* = TRANSPONDER_TIMEOUT */)
{
  PLATFORM::CLockObject lock(m_scannedmutex);
  if (m_filters.empty())
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
  ReceivePacket(pid, data);
}

void cScanReceiver::AddFilter(const filter_properties& filter)
{
  PLATFORM::CLockObject lock(m_mutex);
  std::vector<filter_properties>::const_iterator it = std::find(m_filters.begin(), m_filters.end(), filter);
  if (it != m_filters.end())
    return;
  if (m_device->Receiver()->AttachStreamingReceiver(this, filter.pid, filter.tid, filter.mask))
  {
    m_sectionSyncers.insert(make_pair(filter.pid, new cSectionSyncer));
    m_filtersAdded.push_back(filter);
    m_filters.push_back(filter);
  }
}

void cScanReceiver::RemoveFilter(uint16_t pid)
{
  PLATFORM::CLockObject lock(m_mutex);

  std::vector<filter_properties>::iterator it      = m_filters.begin();
  std::vector<filter_properties>::iterator itAdded = m_filtersAdded.begin();

  for ( ; it != m_filters.end(); ++it)
    if (it->pid == pid)
      break;

  for ( ; itAdded != m_filtersAdded.end(); ++itAdded)
    if (itAdded->pid == pid)
      break;

  if (it != m_filters.end())
  {
    if (DynamicFilter(pid))
    {
      std::map<uint16_t, cSectionSyncer*>::iterator cit = m_sectionSyncers.find(pid);
      if (cit != m_sectionSyncers.end())
      {
        delete cit->second;
        m_sectionSyncers.erase(cit);
      }
      m_filtersAdded.erase(itAdded);
      m_filters.erase(it);
    }

    if (m_attached)
      m_device->Receiver()->DetachReceiverPid(this, pid);
  }
}

void cScanReceiver::RemoveFilters(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  std::vector<filter_properties> filtersRemoved;

  for (std::vector<filter_properties>::iterator it = m_filters.begin(); it != m_filters.end(); ++it)
  {
    m_device->Receiver()->DetachReceiverPid(this, it->pid);

    if (DynamicFilter(it->pid))
    {
      std::map<uint16_t, cSectionSyncer*>::iterator cit = m_sectionSyncers.find(it->pid);
      if (cit != m_sectionSyncers.end())
      {
        delete cit->second;
        m_sectionSyncers.erase(cit);
      }

      filtersRemoved.push_back(*it);
    }
  }

  for (std::vector<filter_properties>::const_iterator it = filtersRemoved.begin(); it != filtersRemoved.end(); ++it)
  {
    std::vector<filter_properties>::iterator it2 = std::find(m_filters.begin(), m_filters.end(), *it);
    assert(it2 != m_filters.end());
    m_filters.erase(it2);
  }
}

void cScanReceiver::SetScanned(void)
{
  {
    PLATFORM::CLockObject lock(m_scannedmutex);
    if (!m_scanned)
    {
      FILTER_DEBUGGING("%s scan completed", m_name.c_str());
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

bool cScanReceiver::DynamicFilter(uint16_t pid) const
{
  for (std::vector<filter_properties>::const_iterator it = m_filtersAdded.begin(); it != m_filtersAdded.end(); ++it)
    if (it->pid == pid)
      return true;
  return false;
}

bool cScanReceiver::Sync(uint16_t pid, uint8_t version, int sectionNumber, int endSectionNumber)
{
  std::map<uint16_t, cSectionSyncer*>::iterator it = m_sectionSyncers.find(pid);
  return it != m_sectionSyncers.end() ?
      it->second->Sync(version, sectionNumber, endSectionNumber) == cSectionSyncer::SYNC_STATUS_NEW_VERSION :
      false;
}

bool cScanReceiver::Synced(uint16_t pid) const
{
  std::map<uint16_t, cSectionSyncer*>::const_iterator it = m_sectionSyncers.find(pid);
  return it != m_sectionSyncers.end() ?
      it->second->Synced() :
      false;
}

}
