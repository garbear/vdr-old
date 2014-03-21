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

#include "DVBSectionFilterSubsystem.h"
#include "../../../devices/linux/DVBDevice.h" // for DEV_DVB_DEMUX
//#include "../../../../sections.h"
#include "utils/Tools.h"

#include <errno.h>
#include <linux/dvb/dmx.h>
#include <string>
#include <string.h>
#include <sys/ioctl.h>
//#include <sys/poll.h>
#include <unistd.h>

using namespace std;

namespace VDR
{

#define READ_BUFFER_SIZE  4096 // From cSectionHandler

// Class cDvbFilterHandle

class cDvbFilterHandle : public cFilterHandle
{
public:
  cDvbFilterHandle(const cFilterData& filterData, int handle)
      : cFilterHandle(filterData),
        m_handle(handle)
  {
  }

  virtual ~cDvbFilterHandle()
  {
    close(m_handle);
  }

  int GetHandle() const { return m_handle; }

private:
  int m_handle;
};

// Class cDvbSectionFilterSubsystem

cDvbSectionFilterSubsystem::cDvbSectionFilterSubsystem(cDevice *device)
 : cDeviceSectionFilterSubsystem(device)
{
}

dmx_sct_filter_params ToFilterParams(const cFilterData& filterData)
{
  dmx_sct_filter_params sctFilterParams = { };
  sctFilterParams.pid = filterData.Pid();
  sctFilterParams.timeout = 0; // seconds to wait for section to be loaded, 0 == no timeout
  sctFilterParams.flags = DMX_IMMEDIATE_START;
  sctFilterParams.filter.filter[0] = filterData.Tid();
  sctFilterParams.filter.mask[0] = filterData.Mask();
  return sctFilterParams;
}

FilterHandlePtr cDvbSectionFilterSubsystem::OpenFilter(const cFilterData& filterData)
{
  string fileName = cDvbDevice::DvbName(DEV_DVB_DEMUX, GetDevice<cDvbDevice>()->Adapter(), GetDevice<cDvbDevice>()->Frontend());

  int fd = open(fileName.c_str(), O_RDWR | O_NONBLOCK);
  if (fd >= 0)
  {
    dmx_sct_filter_params sctFilterParams = ToFilterParams(filterData);

    if (ioctl(fd, DMX_SET_FILTER, &sctFilterParams) >= 0)
      return FilterHandlePtr(new cDvbFilterHandle(filterData, fd));
    else
    {
      esyslog("ERROR: can't set filter (pid=%d, tid=%02X, mask=0x%02X): %s",
          filterData.Pid(), filterData.Tid(), filterData.Mask(), strerror(errno));
      close(fd);
    }
  }
  else
    esyslog("ERROR: can't open filter handle on '%s'", fileName.c_str());

  return FilterHandlePtr();
}

bool cDvbSectionFilterSubsystem::ReadFilter(const FilterHandlePtr& handle, std::vector<uint8_t>& data)
{
  cDvbFilterHandle* dvbHandle = dynamic_cast<cDvbFilterHandle*>(handle.get());
  if (dvbHandle)
  {
    uint8_t buffer[READ_BUFFER_SIZE];
    ssize_t read = safe_read(dvbHandle->GetHandle(), buffer, sizeof(buffer));
    if (read > 0)
    {
      data.assign(buffer, buffer + read);
      return true;
    }
  }
  return false;
}

FilterHandlePtr cDvbSectionFilterSubsystem::Poll(const std::vector<FilterHandlePtr>& filterHandles)
{
  vector<pollfd> vecPfds;
  vecPfds.reserve(filterHandles.size());

  for (vector<FilterHandlePtr>::const_iterator it = filterHandles.begin(); it != filterHandles.end(); ++it)
  {
    cDvbFilterHandle* handle = dynamic_cast<cDvbFilterHandle*>(it->get());
    if (!handle)
    {
      // Fail hard and fast if vector contains invalid handle
      esyslog("cDvbSectionFilterSubsystem: Encountered empty handle!");
      return FilterHandlePtr();
    }

    pollfd pfd;
    pfd.fd = handle->GetHandle();
    pfd.events = POLLIN;
    pfd.revents = 0;
    vecPfds.push_back(pfd);
  }

  if (!vecPfds.empty())
  {
    const unsigned int timeoutMs = 1000;
    if (poll(vecPfds.data(), vecPfds.size(), timeoutMs) > 0)
    {
      for (unsigned int i = 0; i < vecPfds.size(); i++)
      {
        if (vecPfds[i].revents & POLLIN)
          return filterHandles[i];
      }
    }
  }
  return FilterHandlePtr();
}

}
