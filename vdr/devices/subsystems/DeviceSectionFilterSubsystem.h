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
#pragma once

#include "devices/DeviceSubsystem.h"
#include "dvb/DVBTypes.h"
#include "dvb/filters/FilterResource.h"
#include "platform/threads/mutex.h"

#include <set>
#include <shared_ptr/shared_ptr.hpp>
#include <stdint.h>
#include <vector>

namespace VDR
{

class cFilter;

/*!
 * The section filter subsystem is responsible for distributing DVB sections
 * from the device to a collection of filters. A section can come from one of
 * several resources, for example from a DVB demux file (/dev/dvb/adapterX/demuxY).
 */
class cDeviceSectionFilterSubsystem : protected cDeviceSubsystem, protected PLATFORM::CThread
{
private:
  /*!
   * When a client calls cDeviceSectionFilterSubsystem::GetSection(), a request
   * to poll the specified resources is submitted to the polling thread. When
   * one of the resources is ready to provide a section, it will be placed in
   * m_activeResource and WaitForSection() will finish blocking.
   */
  class cFilterResourceRequest
  {
  public:
    cFilterResourceRequest(const FilterResourceCollection& filterResources);
    ~cFilterResourceRequest(void);

    /*!
     * Return all resources being polled.
     */
    const FilterResourceCollection& GetResources(void) const { return m_resources; }

    /*!
     * Returns the resource that received the section, or an empty pointer if no
     * resource has received a section yet.
     */
    FilterResourcePtr GetActiveResource(void) const { return m_activeResource; }

    /*!
     * Block until a resource receives a section. Allows the thread to sleep
     * until one of the resources has a section ready.
     */
    void WaitForSection(void);

    /*!
     * Called by cDeviceSectionFilterSubsystem::HandleSection() when a section
     * has been received from the section handler. Any threads blocking on this
     * request via WaitForSection() will return.
     */
    void HandleSection(const FilterResourcePtr& resource);

    void Abort(void);

  private:
    FilterResourceCollection m_resources;      // The filter resources that this request is polling
    FilterResourcePtr        m_activeResource; // Resource ready to be read
    PLATFORM::CEvent         m_readyEvent;     // Fired when resource is ready to be read
  };

  typedef std::vector<shared_ptr<cFilterResourceRequest> > ResourceRequestCollection;

public:
  cDeviceSectionFilterSubsystem(cDevice* device);
  virtual ~cDeviceSectionFilterSubsystem(void) { StopSectionHandler(); }

  void StartSectionHandler(void);
  void StopSectionHandler(void);

  /*!
   * Register/unregister a filter. This is used to track which filter resources
   * are open.
   */
  void RegisterFilter(const cFilter* filter);
  void UnregisterFilter(const cFilter* filter);

  /*!
   * Open a resource internally for the given PID/TID. If the resource is already
   * help by another registered filter, a pointer to the open resource will be
   * returned instead. When all filters are destroyed, all references to a
   * resource will be lost and the resource will close automatically
   * (RAII pattern).
   */
  FilterResourcePtr OpenResource(uint16_t pid, uint8_t tid, uint8_t mask);

  /*!
   * Wait on a collection of filter resources until one of them has received a
   * section. If this returns true, a section will be placed in data and pid
   * will be set to the pid of the section. Blocks until a section is received
   * or false is returned.
   */
  bool GetSection(const FilterResourceCollection& filterResources, uint16_t& pid, std::vector<uint8_t>& data);

protected:
  /*!
   * Polling thread.
   */
  void* Process(void);

  /*!
   * Open a resource for the given filter data. Returns the resource, or an
   * empty pointer if open failed or wasn't implemented.
   */
  virtual FilterResourcePtr OpenResourceInternal(uint16_t pid, uint8_t tid, uint8_t mask) = 0;

  /*!
   * Read data from a resource.
   */
  virtual bool ReadResource(const FilterResourcePtr& resource, std::vector<uint8_t>& data) = 0;

  /*!
   * Wait on any of the provided resources until one receives a section. Returns
   * the resource that received the section, or an empty pointer if no filters
   * were received by any sections.
   */
  virtual FilterResourcePtr Poll(const FilterResourceCollection& filterResources) = 0;

private:
  /*!
   * Scan registered filters for an existing resource with the specified ID.
   */
  FilterResourcePtr GetOpenResource(uint16_t pid, uint8_t tid, uint8_t mask);

  /*!
   * Accumulate resources of all active filters (those who are waiting on a call
   * to GetSection()).
   */
  FilterResourceCollection GetActiveResources(void);

  /*!
   * Called when a section has been read from the device.
   */
  void HandleSection(const FilterResourcePtr& resource);

  ResourceRequestCollection m_activePollRequests;
  std::set<const cFilter*>  m_registeredFilters;
  PLATFORM::CMutex          m_mutex;
};

}
