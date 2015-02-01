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
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/Remux.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "utils/log/Log.h"
#include <libsi/util.h>
#include <algorithm>

using namespace std;
using namespace PLATFORM;

#define FILTER_DEBUGGING(x...) dsyslog(x)
//#define FILTER_DEBUGGING(x...) {}

/** maximum open pids for this filter */
#define SCAN_MAX_OPEN_FILTERS (5)

namespace VDR
{

bool operator<(const cScanReceiver::filter_properties& lhs, const cScanReceiver::filter_properties& rhs)
{
  if (rhs.pid  < lhs.pid)  return true;
  if (lhs.pid  < rhs.pid)  return false;
  if (rhs.tid  < lhs.tid)  return true;
  if (lhs.tid  < rhs.tid)  return false;
  if (rhs.mask < lhs.mask) return true;
  if (lhs.mask < rhs.mask) return false;
  return false;
}

cScanReceiver::cScanFilterStatus::cScanFilterStatus(const filter_properties& filter, cScanReceiver* receiver, bool dynamic):
    m_filter(filter),
    m_syncer(new cSectionSyncer),
    m_state(SCAN_STATE_WAITING),
    m_dynamic(dynamic),
    m_attached(false),
    m_receiver(receiver)
{
}

cScanReceiver::cScanFilterStatus::~cScanFilterStatus(void)
{
  Detach();
  delete m_syncer;
}

cScanReceiver::FILTER_SCAN_STATE cScanReceiver::cScanFilterStatus::State(void) const
{
  CLockObject lock(m_mutex);
  return m_state;
}

void cScanReceiver::cScanFilterStatus::SetState(FILTER_SCAN_STATE state)
{
  CLockObject lock(m_mutex);
  m_state = state;
}

bool cScanReceiver::cScanFilterStatus::Attached(void) const
{
  CLockObject lock(m_mutex);
  return m_attached;
}

bool cScanReceiver::cScanFilterStatus::Attach(void)
{
  CLockObject lock(m_mutex);
  if (!m_attached)
  {
    m_attached = m_receiver->m_device->Receiver()->AttachStreamingReceiver(m_receiver, m_filter.pid, m_filter.tid, m_filter.mask);
    if (m_attached)
      m_state = SCAN_STATE_OPEN;
  }
  return m_attached;
}

void cScanReceiver::cScanFilterStatus::Detach(void)
{
  CLockObject lock(m_mutex);
  if (m_attached)
  {
    m_attached = false;
    m_receiver->m_device->Receiver()->DetachStreamingReceiver(m_receiver, m_filter.pid, m_filter.tid, m_filter.mask, false);
    m_syncer->Reset();
  }
}

void cScanReceiver::cScanFilterStatus::ResetSectionSyncer(void)
{
  CLockObject lock(m_mutex);
  m_syncer->Reset();
}

bool cScanReceiver::cScanFilterStatus::Sync(uint8_t version, int sectionNumber, int endSectionNumber)
{
  CLockObject lock(m_mutex);
  return m_syncer->Sync(version, sectionNumber, endSectionNumber) == cSectionSyncer::SYNC_STATUS_NEW_VERSION;
}

bool cScanReceiver::cScanFilterStatus::Synced(void) const
{
  CLockObject lock(m_mutex);
  return m_syncer->Synced();
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
  m_filtersNew.insert(make_pair(filter, new cScanFilterStatus(filter, this, false)));
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, const std::vector<filter_properties>& filters) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
  for (std::vector<filter_properties>::const_iterator it = filters.begin(); it != filters.end(); ++it)
    m_filtersNew.insert(make_pair(*it, new cScanFilterStatus(*it, this, false)));
}

cScanReceiver::cScanReceiver(cDevice* device, const std::string& name, size_t nbFilters, const filter_properties* filters) :
    m_device(device),
    m_locked(false),
    m_scanned(false),
    m_attached(false),
    m_name(name)
{
  for (size_t ptr = 0; ptr < nbFilters; ++ptr)
    m_filtersNew.insert(make_pair(filters[ptr], new cScanFilterStatus(filters[ptr], this, false)));
}

cScanReceiver::~cScanReceiver(void)
{
  Detach();
  for (std::map<filter_properties, cScanFilterStatus*>::iterator cit = m_filtersNew.begin(); cit != m_filtersNew.end(); ++cit)
    delete cit->second;
}

bool cScanReceiver::Attach(void)
{
  bool retval = true;
  CLockObject lock(m_mutex);

  if (m_attached)
    return true;

  m_attached = true;
  for (std::map<filter_properties, cScanFilterStatus*>::const_iterator it = m_filtersNew.begin(); m_attached && it != m_filtersNew.end(); ++it)
    m_attached &= it->second->Attach();

  if (!m_attached)
    esyslog("failed to attach %s filter", m_name.c_str());
  else
  {
    FILTER_DEBUGGING("%s filter attached", m_name.c_str());
    m_scanned = false;
  }

  return m_attached;
}

void cScanReceiver::Detach(bool wait /* = false */)
{
  CLockObject lock(m_mutex);
  if (m_attached)
  {
    m_attached = false;
    RemoveFilters();
    FILTER_DEBUGGING("%s filter detached", m_name.c_str());
  }
}

bool cScanReceiver::WaitForScan(uint32_t iTimeout /* = TRANSPONDER_TIMEOUT */)
{
  CLockObject lock(m_scannedmutex);
  if (m_filtersNew.empty() || !m_attached)
    return true;
  return m_scannedEvent.Wait(m_scannedmutex, m_scanned, iTimeout);
}

void cScanReceiver::LockAcquired(void)
{
  CLockObject lock(m_mutex);
  m_locked  = true;
  m_scanned = false;
}

void cScanReceiver::LockLost(void)
{
  CLockObject lock(m_mutex);
  m_locked = false;
  m_scanned = false;
  Detach();
}

void cScanReceiver::Receive(const uint16_t pid, const uint8_t* data, const size_t len, ts_crc_check_t& crcvalid)
{
  if (crcvalid == TS_CRC_NOT_CHECKED)
    crcvalid = SI::CRC32::isValid((const char *)data, len) ? TS_CRC_CHECKED_VALID : TS_CRC_CHECKED_INVALID;
  if (crcvalid == TS_CRC_CHECKED_INVALID)
    return;

  ReceivePacket(pid, data);
}

void cScanReceiver::AddFilter(const filter_properties& filter)
{
  CLockObject lock(m_mutex);
  if (m_filtersNew.find(filter) != m_filtersNew.end())
    return;
  cScanFilterStatus* scan = new cScanFilterStatus(filter, this, true);
  if (scan)
  {
    m_filtersNew.insert(make_pair(filter, scan));
    if (NbOpenPids() < SCAN_MAX_OPEN_FILTERS)
      scan->Attach();
  }
}

void cScanReceiver::RemoveFilter(const filter_properties& filter)
{
  CLockObject lock(m_mutex);
  std::map<filter_properties, cScanFilterStatus*>::iterator it = m_filtersNew.find(filter);
  if (it != m_filtersNew.end())
  {
    it->second->Detach();
    if (it->second->Dynamic())
      m_filtersNew.erase(it);
  }
}

void cScanReceiver::RemoveFilters(void)
{
  CLockObject lock(m_mutex);
  std::vector<filter_properties> filtersRemoved;

  for (std::map<filter_properties, cScanFilterStatus*>::iterator it = m_filtersNew.begin(); it != m_filtersNew.end();)
  {
    if (it->second->Dynamic())
    {
      delete it->second;
      m_filtersNew.erase(it++);
    }
    else
    {
      it->second->Detach();
      ++it;
    }
  }

  m_attached = false;
}

void cScanReceiver::SetScanned(void)
{
  {
    CLockObject lock(m_scannedmutex);
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
  CLockObject lock(m_scannedmutex);
  m_scanned = false;
}

bool cScanReceiver::Scanned(void) const
{
  CLockObject lock(m_scannedmutex);
  return m_attached && m_scanned;
}

bool cScanReceiver::DynamicFilter(const filter_properties& filter) const
{
  std::map<filter_properties, cScanFilterStatus*>::const_iterator it = m_filtersNew.find(filter);
  if (it != m_filtersNew.end())
    return it->second->Dynamic();
  return false;
}

bool cScanReceiver::Sync(uint16_t pid, uint8_t tid, uint8_t version, int sectionNumber, int endSectionNumber)
{
  CLockObject lock(m_mutex);
  for (std::map<filter_properties, cScanFilterStatus*>::iterator it = m_filtersNew.begin(); it != m_filtersNew.end(); ++it)
  {
    if (it->first.pid == pid && it->first.tid == tid)
      return it->second->Sync(version, sectionNumber, endSectionNumber);
  }
  return false;
}

bool cScanReceiver::Synced(uint16_t pid) const
{
  bool bSynced(false);
  CLockObject lock(m_mutex);
  for (std::map<filter_properties, cScanFilterStatus*>::const_iterator it = m_filtersNew.begin(); it != m_filtersNew.end(); ++it)
  {
    if (it->first.pid == pid)
      bSynced &= it->second->Synced();
  }
  return bSynced;
}

void cScanReceiver::FilterScanned(const filter_properties& filter)
{
  CLockObject lock(m_mutex);
  cScanFilterStatus* nextFilter;
  bool hasNextFilter(false);
  size_t activePids(0);

  std::map<filter_properties, cScanFilterStatus*>::iterator it = m_filtersNew.find(filter);
  if (it != m_filtersNew.end())
  {
    it->second->Detach();
    it->second->SetState(SCAN_STATE_DONE);
  }

  for (std::map<filter_properties, cScanFilterStatus*>::const_iterator it = m_filtersNew.begin(); it != m_filtersNew.end(); ++it)
  {
    switch (it->second->State())
    {
      case SCAN_STATE_OPEN:
        ++activePids;
        break;
      case SCAN_STATE_WAITING:
        if (!hasNextFilter)
        {
          nextFilter = it->second;
          hasNextFilter = true;
        }
        break;
      default:
        break;
    }
  }

  if (activePids < SCAN_MAX_OPEN_FILTERS && hasNextFilter)
    nextFilter->Attach(); // TODO check for failures
}

size_t cScanReceiver::NbOpenPids(void) const
{
  size_t retval(0);
  CLockObject lock(m_mutex);
  for (std::map<filter_properties, cScanFilterStatus*>::const_iterator it = m_filtersNew.begin(); it != m_filtersNew.end(); ++it)
    if (it->second->Attached())
      ++retval;
  return retval;
}

}
