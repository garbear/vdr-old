/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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
#include "DeviceChannelSubsystem.h"
#include "dvb/filters/Filter.h"
#include "lib/platform/util/timeutils.h"
#include "utils/log/Log.h"
#include "utils/Tools.h"

#include <algorithm>
#include <unistd.h>

using namespace PLATFORM;
using namespace std;

// Wait 10 seconds between logging incomplete sections
#define LOG_INCOMPLETE_SECTIONS_DELAY_S  10

// When idle (no clients are waiting for GetSection()), we check every
// MAX_GETSECTION_DELAY_MS to see if GetSection() has been called. Thus, this is
// the maximum delay spent waiting for the polling thread to awaken. This can be
// reduced to 0 (no delay) by waiting on an event when idle.
#define MAX_IDLE_DELAY_MS 10

// When not idle (one or more clients are waiting for GetSection() to return)
// the filter resources are polled with a maximum timeout specified by:
//
// POLL_TIMEOUT_MS in DVBSectionFilterSubsystem.cpp
//
// If another call to GetSection() is made concurrently, it must wait for the
// current poll to return or time out. Thus, POLL_TIMEOUT_MS is the maximum
// delay spent waiting for its resources to be included in the polling process.
// This can be reduced to 0 (no delay) by implementing an abortable polling
// function.

// Wait 4 seconds for section to arrive before timing out
#define SECTION_TIMEOUT_MS  4000

namespace VDR
{

// --- cDeviceSectionFilterSubsystem::cFilterHandlePollRequest-----------------

cDeviceSectionFilterSubsystem::cResourceRequest::cResourceRequest(const PidResourceSet& filterResources)
 : m_resources(filterResources)
{
}

cDeviceSectionFilterSubsystem::cResourceRequest::~cResourceRequest(void)
{
  Abort();
}

bool cDeviceSectionFilterSubsystem::cResourceRequest::WaitForSection(void)
{
  return m_readyEvent.Wait(SECTION_TIMEOUT_MS);
}

void cDeviceSectionFilterSubsystem::cResourceRequest::HandleSection(const PidResourcePtr& resource)
{
  // Record the resource that is ready to provide a section
  m_activeResource = resource;
  m_readyEvent.Broadcast();
}

void cDeviceSectionFilterSubsystem::cResourceRequest::Abort(void)
{
  m_readyEvent.Broadcast();
}

// --- cDeviceSectionFilterSubsystem ------------------------------------------

cDeviceSectionFilterSubsystem::cDeviceSectionFilterSubsystem(cDevice* device)
 : cDeviceSubsystem(device)
{
}

void cDeviceSectionFilterSubsystem::Start(void)
{
  if (!IsRunning())
    CreateThread(true);
}

void cDeviceSectionFilterSubsystem::Stop(void)
{
  {
    CLockObject lock(m_mutex);

    // Look for any poll requests that were waiting on the resource
    for (ResourceRequestVector::iterator it = m_activePollRequests.begin(); it != m_activePollRequests.end(); ++it)
      (*it)->Abort();
  }
  StopThread(0);
}

void cDeviceSectionFilterSubsystem::RegisterFilter(const cFilter* filter)
{
  CLockObject lock(m_mutex);

  m_registeredFilters.insert(filter);
}

void cDeviceSectionFilterSubsystem::UnregisterFilter(const cFilter* filter)
{
  CLockObject lock(m_mutex);

  set<const cFilter*>::iterator it = m_registeredFilters.find(filter);
  assert(it != m_registeredFilters.end());
  m_registeredFilters.erase(it);
}

PidResourcePtr cDeviceSectionFilterSubsystem::OpenResourceInternal(uint16_t pid, uint8_t tid, uint8_t mask)
{
  PidResourcePtr newResource;
  PidResourcePtr existingResource = GetOpenResource(newResource);

  if (existingResource)
    return existingResource;

  newResource = CreateResource(pid, tid, mask);
  if (newResource && newResource->Open())
    return newResource;

  return PidResourcePtr();
}

bool cDeviceSectionFilterSubsystem::GetSection(const PidResourceSet& filterResources, uint16_t& pid, std::vector<uint8_t>& data)
{
  // If we don't have a tuner lock, no sections are being received
  if (!Channel()->HasLock())
    return false;

  // Create a new request to poll filter resources. It will be polled until:
  //
  //   - Polling thread: One of its resources has received a section and is
  //                     ready to be read, or thread is aborted.
  //   - This thread:    WaitForSection() times out in this thread
  //
  // Because a possible race exists between these conditions, we need to check
  // for both of these invariants when reading the resource.

  shared_ptr<cResourceRequest> pollRequest = shared_ptr<cResourceRequest>(new cResourceRequest(filterResources));

  // Record it so that Process() can access it
  {
    CLockObject lock(m_mutex);
    m_activePollRequests.push_back(pollRequest);
  }

  // Block until a resources receives a section
  const bool bTimedOut = !pollRequest->WaitForSection();

  {
    CLockObject lock(m_mutex);

    const bool bAborted  = !pollRequest->GetActiveResource();

    // The polling thread will autonomously remove the poll request after being
    // notified that it's ready to receive a section.
    ResourceRequestVector::iterator it = std::find(m_activePollRequests.begin(), m_activePollRequests.end(), pollRequest);
    if (bAborted)
    {
      assert(it != m_activePollRequests.end());
      m_activePollRequests.erase(it);
    }
    else
    {
      assert(it == m_activePollRequests.end());
    }

    // Must check both invariants
    if (!bTimedOut && !bAborted)
    {
      if (ReadResource(pollRequest->GetActiveResource(), data))
      {
        pid = pollRequest->GetActiveResource()->Pid();
        return true;
      }
      else
      {
        dsyslog("Failed to read section with PIDs %s: failed to read from section filter resource", ToString(filterResources).c_str());
      }
    }
    else
    {
      dsyslog("Failed to read section with PIDs %s: %s", ToString(filterResources).c_str(), bTimedOut ? "timed out" : "aborted");
    }
  }

  return false;
}

void* cDeviceSectionFilterSubsystem::Process(void)
{
  if (!Initialise())
    return NULL;

  // Buffer for section data
  std::vector<uint8_t> data;

  CTimeout nextLogTimeout(LOG_INCOMPLETE_SECTIONS_DELAY_S * 1000);

  while (!IsStopped())
  {
    // Accumulate all resources from active poll requests into a vector
    PidResourceSet activeResources = GetActiveResources();

    if (activeResources.empty())
    {
      // TODO: Wait on event instead of sleeping
      usleep(MAX_IDLE_DELAY_MS * 1000);
      continue;
    }

    // TODO: Need a thread-safe, abortable poll
    PidResourcePtr resource = Poll(activeResources);
    if (resource && !IsStopped())
    {
      // TODO: We shouldn't lose channel lock, should we?
      bool bDeviceHasLock = Channel()->HasLock();
      if (!bDeviceHasLock)
        usleep(100 * 1000); // 100ms?

      if (!ReadResource(resource, data))
        continue;

      // Do the read even if we don't have a lock to flush any data that might
      // have come from a different transponder
      if (!bDeviceHasLock)
        continue;

      // Minimum number of bytes necessary to get section length
      if (data.size() > 3)
      {
        unsigned int len = (((data[1] & 0x0F) << 8) | (data[2] & 0xFF)) + 3;
        if (len == data.size())
        {
          HandleSection(resource);
        }
        else if (nextLogTimeout.TimeLeft() == 0)
        {
          // log every 10 seconds
          dsyslog("read incomplete section - len = %d, read = %d", len, data.size());
          nextLogTimeout.Init(LOG_INCOMPLETE_SECTIONS_DELAY_S * 1000);
        }
      }
    }
  }

  {
    CLockObject lock(m_mutex);

    // Look for any poll requests that were waiting on the resource
    for (ResourceRequestVector::iterator it = m_activePollRequests.begin(); it != m_activePollRequests.end(); ++it)
      (*it)->Abort();
  }

  Deinitialise();

  return NULL;
}

PidResourcePtr cDeviceSectionFilterSubsystem::GetOpenResource(const PidResourcePtr& needle)
{
  CLockObject lock(m_mutex);

  // Scan registered filters for the resource
  for (set<const cFilter*>::const_iterator itFilter = m_registeredFilters.begin(); itFilter != m_registeredFilters.end(); ++itFilter)
  {
    const PidResourceSet& haystack = (*itFilter)->GetResources();
    for (PidResourceSet::const_iterator itResource = haystack.begin(); itResource != haystack.end(); ++itResource)
    {
      if ((*itResource)->Equals(needle.get()))
        return *itResource;
    }
  }

  return PidResourcePtr();
}

PidResourceSet cDeviceSectionFilterSubsystem::GetActiveResources(void) const
{
  CLockObject lock(m_mutex);

  PidResourceSet activeResources;

  // Enumerate all active filters (those who are waiting on a call to GetSection())
  for (ResourceRequestVector::const_iterator it = m_activePollRequests.begin(); it != m_activePollRequests.end(); ++it)
  {
    const PidResourceSet& filterResources = (*it)->GetResources();
    activeResources.insert(filterResources.begin(), filterResources.end());
  }

  return activeResources;
}

void cDeviceSectionFilterSubsystem::HandleSection(const PidResourcePtr& resource)
{
  CLockObject lock(m_mutex);

  // Fix for erasing requests from m_activePollRequests mid-loop
  ResourceRequestVector expiredPollRequests;

  // Look for any poll requests that were waiting on the resource
  for (ResourceRequestVector::iterator it = m_activePollRequests.begin(); it != m_activePollRequests.end(); ++it)
  {
    const PidResourceSet& filterResources = (*it)->GetResources();
    PidResourceSet::const_iterator it2 = filterResources.find(resource);
    if (it2 != filterResources.end())
    {
      // Found a poll request waiting on the resource, notify clients
      (*it)->HandleSection(resource);

      // Don't service this request twice
      expiredPollRequests.push_back(*it);
    }
  }

  for (ResourceRequestVector::iterator it = expiredPollRequests.begin(); it != expiredPollRequests.end(); ++it)
  {
    ResourceRequestVector::iterator it2 = std::find(m_activePollRequests.begin(), m_activePollRequests.end(), *it);
    assert(it2 != m_activePollRequests.end());
    m_activePollRequests.erase(it2);
  }
}

std::string cDeviceSectionFilterSubsystem::ToString(const PidResourceSet& resources)
{
  std::string retval;

  for (PidResourceSet::const_iterator it = resources.begin(); it != resources.end(); ++it)
    retval.append((*it)->ToString());

  if (resources.empty())
    retval = "[none]";

  return retval;
}

}
