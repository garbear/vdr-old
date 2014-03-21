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
#pragma once

#include "Types.h"
#include "devices/DeviceSubsystem.h"
#include "dvb/filters/EIT.h"
#include "dvb/filters/FilterData.h"
#include "dvb/filters/NIT.h"
#include "dvb/filters/PAT.h"
#include "dvb/filters/SDT.h"
#include "threads/SystemClock.h" // for XbmcThreads::EndTime

#include <map>
#include <shared_ptr/shared_ptr.hpp>
#include <stdint.h>
#include <vector>

namespace VDR
{

class cFilterHandle
{
public:
  cFilterHandle(const cFilterData& filterData) : m_filterData(filterData) { }
  virtual ~cFilterHandle() { }

  const cFilterData& GetFilterData() const { return m_filterData; }

private:
  cFilterData m_filterData;
};

typedef VDR::shared_ptr<cFilterHandle> FilterHandlePtr;

/*!
 * \brief Container to hold filter handles.
 *
 * Vectors of filter handles are mapped to their device. Filter handles are
 * stored in shared pointers, so when a device and its filter handles are
 * removed from the map, the reference count of all orphaned filter handles will
 * drop to zero, closing the handle.
 *
 * This container is optimized for access to vectors of all filters and vectors
 * of all filter handles. This is done by keeping a separate cache vector for
 * each that can be returned in O(1) time.
 */
class cFilterHandleContainer
{
public:
  void InsertFilter(cFilter* filter, const std::vector<FilterHandlePtr>& filterHandles);
  void RemoveFilter(cFilter* filter);
  void Clear(void);

  const FilterHandlePtr& GetHandle(const cFilterData& data) const;

  const std::vector<cFilter*>&        GetFilters(void) const { return m_filters; }
  const std::vector<FilterHandlePtr>& GetHandles(void) const { return m_filterHandles; }

private:
  void UpdateCache(void);

  std::map<cFilter*, std::vector<FilterHandlePtr> > m_container;
  std::vector<cFilter*>                             m_filters;
  std::vector<FilterHandlePtr>                      m_filterHandles;
};

class cDeviceSectionFilterSubsystem : protected cDeviceSubsystem, protected PLATFORM::CThread
{
public:
  cDeviceSectionFilterSubsystem(cDevice *device);
  virtual ~cDeviceSectionFilterSubsystem() { }

  int GetSource(void);
  int GetTransponder(void);
  ChannelPtr GetChannel(void);
  void SetChannel(const ChannelPtr& channel);

  /*!
   * \brief A derived device that provides section data must call this function
   *        (typically in its constructor) to actually set up the section handler
   */
  void StartSectionHandler();

  /*!
   * \brief A device that has called StartSectionHandler() must call this
   *        function (typically in its destructor) to stop the section handler
   */
  void StopSectionHandler();

  /*!
   * \brief Attach a filter to this device and open the necessary handles.
   */
  void RegisterFilter(cFilter* filter);

  /*!
   * \brief Detach a filter from this device. Unused handles are automatically
   * closed.
   */
  void UnregisterFilter(cFilter* filter);

protected:
  void* Process(void);

  /*!
   * \brief Reads data from a handle for the given filter
   *
   * A derived class need not implement this function, because this is done by
   * the default implementation.
   */
  virtual bool ReadFilter(const FilterHandlePtr& handle, std::vector<uint8_t>& data) { return false; }

  /*!
   * \brief Wait an event on any of the provided handles
   * \return The handle that fired the event, or empty pointer if poll timed out
   */
  virtual FilterHandlePtr Poll(const std::vector<FilterHandlePtr>& filterHandles) { return FilterHandlePtr(); }

  /*!
   * \brief Opens a file handle for the given filter data
   *
   * A derived device that provides section data must implement this function.
   */
  virtual FilterHandlePtr OpenFilter(const cFilterData& filterData) { return FilterHandlePtr(); }

private:
  cEitFilter      m_eitFilter;
  cPatFilter      m_patFilter;
  cSdtFilter      m_sdtFilter;
  cNitFilter      m_nitFilter;

  cFilterHandleContainer m_filterHandles;

  ChannelPtr             m_channel;
  int                    m_iStatusCount;
  bool                   m_bEnabled;
  VDR::EndTime           m_nextLogTimeout;
  PLATFORM::CMutex       m_mutex;
};
}
