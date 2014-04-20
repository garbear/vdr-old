/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "DeviceSectionFilterSubsystem.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceSectionFilterSubsystem.h"
#include "dvb/filters/FilterData.h"
#include "platform/threads/mutex.h"
#include "utils/Tools.h"

#include <algorithm>
#include <set>
#include <unistd.h>

namespace VDR
{

using namespace PLATFORM;
using namespace std;

// Wait 10 seconds between logging incomplete sections
#define LOG_INCOMPLETE_SECTIONS_DELAY_S  10

// --- cFilterHandleContainer --------------------------------------------------

void cFilterHandleContainer::InsertFilter(cFilter* filter, const vector<FilterHandlePtr>& filterHandles)
{
  // If any filter handles are orphaned by this, their reference count will drop
  // to zero, closing the handle
  m_container[filter] = filterHandles;

  UpdateCache();
}

void cFilterHandleContainer::RemoveFilter(cFilter* filter)
{
  map<cFilter*, vector<FilterHandlePtr> >::iterator it = m_container.find(filter);
  if (it != m_container.end())
  {
    m_container.erase(it);
    UpdateCache();
  }
}

void cFilterHandleContainer::Clear()
{
  m_container.clear();
  m_filters.clear();
  m_filterHandles.clear();
}

const FilterHandlePtr& cFilterHandleContainer::GetHandle(const cFilterData& data) const
{
  static FilterHandlePtr emptyPtr;

  for (vector<FilterHandlePtr>::const_iterator itHandle = m_filterHandles.begin();
      itHandle != m_filterHandles.end(); ++itHandle)
  {
    if ((*itHandle)->GetFilterData() == data)
      return *itHandle;
  }

  return emptyPtr;
}

void cFilterHandleContainer::UpdateCache(void)
{
  std::vector<cFilter*>     filters;
  std::set<FilterHandlePtr> uniqueFilterHandles;

  // Walk the map and insert filter handles into the set
  for (map<cFilter*, vector<FilterHandlePtr> >::const_iterator itFilter = m_container.begin();
      itFilter != m_container.end(); ++itFilter)
  {
    filters.push_back(itFilter->first);

    const vector<FilterHandlePtr> filterHandles = itFilter->second;
    for (vector<FilterHandlePtr>::const_iterator itHandle = filterHandles.begin();
        itHandle != filterHandles.end(); ++itHandle)
    {
      uniqueFilterHandles.insert(*itHandle);
    }
  }

  m_filters.clear();
  m_filterHandles.clear();

  m_filters.insert(m_filters.begin(), filters.begin(), filters.end());
  m_filterHandles.insert(m_filterHandles.begin(), uniqueFilterHandles.begin(), uniqueFilterHandles.end());
}

// --- cDeviceSectionFilterSubsystem -------------------------------------------

cDeviceSectionFilterSubsystem::cDeviceSectionFilterSubsystem(cDevice *device)
 : cDeviceSubsystem(device),
   m_eitFilter(device),
   m_patFilter(device),
   m_sdtFilter(device, &m_patFilter),
   m_nitFilter(device),
   m_iStatusCount(0),
   m_bEnabled(false)
{
}

int cDeviceSectionFilterSubsystem::GetSource(void)
{
  CLockObject lock(m_mutex);
  return IsRunning() ? m_channel->Source() : 0;
}

int cDeviceSectionFilterSubsystem::GetTransponder(void)
{
  CLockObject lock(m_mutex);
  return IsRunning() ? m_channel->TransponderFrequency() : 0;
}

ChannelPtr cDeviceSectionFilterSubsystem::GetChannel(void)
{
  CLockObject lock(m_mutex);
  return IsRunning() ? m_channel : cChannel::EmptyChannel;
}

void cDeviceSectionFilterSubsystem::SetChannel(const ChannelPtr& channel)
{
  CLockObject lock(m_mutex);
  m_channel = channel;
}

void cDeviceSectionFilterSubsystem::StartSectionHandler()
{
  CLockObject lock(m_mutex);
  if (!IsRunning())
  {
    m_eitFilter.Enable(true);
    m_patFilter.Enable(true);
    m_sdtFilter.Enable(true);
    m_nitFilter.Enable(true);
    CreateThread();
  }
}

void cDeviceSectionFilterSubsystem::StopSectionHandler()
{
  StopThread(3000);

  {
    CLockObject lock(m_mutex);
    m_filterHandles.Clear();
  }
}

void* cDeviceSectionFilterSubsystem::Process(void)
{
  // Buffer for section data
  std::vector<uint8_t> data;

  m_nextLogTimeout.Set(LOG_INCOMPLETE_SECTIONS_DELAY_S * 1000);

  while (!IsStopped())
  {
    vector<FilterHandlePtr> filterHandles;
    {
      CLockObject lock(m_mutex);
      filterHandles = m_filterHandles.GetHandles();
    }

    // Poll set of filter handles
    // TODO: Need a thread-safe, abortable poll
    FilterHandlePtr handle = Poll(filterHandles);
    if (handle)
    {
      bool bDeviceHasLock = Channel()->HasLock();
      if (!bDeviceHasLock)
        usleep(100 * 1000); // 100ms

      // Read section data
      if (!ReadFilter(handle, data))
        continue;

      // Do the read even if we don't have a lock to flush any data that might
      // have come from a different transponder
      if (!bDeviceHasLock)
        continue;

      // Minimum number of bytes necessary to get section length
      if (data.size() > 3)
      {
        int len = (((data[1] & 0x0F) << 8) | (data[2] & 0xFF)) + 3;
        if (len == data.size())
        {
          CLockObject lock(m_mutex);

          // Distribute data to all attached filters:
          int pid = handle->GetFilterData().Pid();
          int tid = data[0];
          for (vector<cFilter*>::const_iterator it = m_filterHandles.GetFilters().begin();
              it != m_filterHandles.GetFilters().end(); ++it)
          {
            if ((*it)->Matches(pid, tid))
              (*it)->ProcessData(pid, tid, data);
          }
        }
        else if (m_nextLogTimeout.IsTimePast())
        {
          // log them only every 10 seconds
          dsyslog("read incomplete section - len = %d, r = %d", len, data.size());
          m_nextLogTimeout.Set(LOG_INCOMPLETE_SECTIONS_DELAY_S * 1000);
        }
      }
    }
  }

  return NULL;
}

void cDeviceSectionFilterSubsystem::RegisterFilter(cFilter* filter)
{
  assert(filter);

  CLockObject lock(m_mutex);

  // Build a vector of handles to associate with this filter
  vector<FilterHandlePtr> newHandles;

  // Obtain a handle for the filter's data, either by opening a new handle or
  // by obtaining an existing one.
  const vector<cFilterData>& vecData = filter->GetFilterData();
  for (vector<cFilterData>::const_iterator itData = vecData.begin(); itData != vecData.end(); ++itData)
  {
    const FilterHandlePtr& handle = m_filterHandles.GetHandle(*itData);
    if (handle)
    {
      // Copy the existing handle for the data
      dsyslog("Opened handle for filter PID=%u, TID=%u",
          handle->GetFilterData().Pid(), handle->GetFilterData().Tid());
      newHandles.push_back(handle);
    }
    else
    {
      // Open a new handle for the data
      FilterHandlePtr newHandle = OpenFilter(*itData);
      if (newHandle)
        newHandles.push_back(newHandle);
    }
  }

  m_filterHandles.InsertFilter(filter, newHandles);
}

void cDeviceSectionFilterSubsystem::UnregisterFilter(cFilter* filter)
{
  assert(filter);

  CLockObject lock(m_mutex);

  m_filterHandles.RemoveFilter(filter);
}

}
