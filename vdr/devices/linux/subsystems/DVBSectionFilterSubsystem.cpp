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

#include "DVBSectionFilterSubsystem.h"
#include "devices/linux/DVBDevice.h" // for DEV_DVB_DEMUX
#include "dvb/filters/FilterResource.h"
//#include "../../../../sections.h"
#include "utils/log/Log.h"
#include "utils/Tools.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/dvb/dmx.h>
#include <string>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>

using namespace std;

#define READ_BUFFER_SIZE  4096 // From cSectionHandler
#define POLL_TIMEOUT_MS   1000

namespace VDR
{

// --- cDvbFilterResource -------------------------------------------------------

class cDvbFilterResource : public cFilterResource
{
public:
  cDvbFilterResource(uint16_t pid, uint8_t tid, uint8_t mask, int fileDescriptor)
    : cFilterResource(pid, tid, mask),
      m_fileDescriptor(fileDescriptor)
  {
  }

  // Use RAII pattern to close file when no longer referenced
  virtual ~cDvbFilterResource()
  {
    close(m_fileDescriptor);
    dsyslog("Closed handle for filter PID=%u, TID=0x%02X", GetPid(), GetTid());
  }

  int GetFileDescriptor() const { return m_fileDescriptor; }

private:
  const int m_fileDescriptor;
};

// --- cDvbSectionFilterSubsystem ---------------------------------------------

cDvbSectionFilterSubsystem::cDvbSectionFilterSubsystem(cDevice *device)
 : cDeviceSectionFilterSubsystem(device)
{
}

dmx_sct_filter_params ToFilterParams(uint16_t pid, uint8_t tid, uint8_t mask)
{
  dmx_sct_filter_params sctFilterParams = { };
  sctFilterParams.pid = pid;
  sctFilterParams.timeout = 0; // seconds to wait for section to be loaded, 0 == no timeout
  sctFilterParams.flags = DMX_IMMEDIATE_START;
  sctFilterParams.filter.filter[0] = tid;
  sctFilterParams.filter.mask[0] = mask;
  return sctFilterParams;
}

FilterResourcePtr cDvbSectionFilterSubsystem::OpenResourceInternal(uint16_t pid, uint8_t tid, uint8_t mask)
{
  string fileName = cDvbDevice::DvbName(DEV_DVB_DEMUX, GetDevice<cDvbDevice>()->Adapter(), GetDevice<cDvbDevice>()->Frontend());

  // Don't open with O_NONBLOCK flag so that reads will wait for a full section
  int fd = open(fileName.c_str(), O_RDWR);
  if (fd >= 0)
  {
    dmx_sct_filter_params sctFilterParams = ToFilterParams(pid, tid, mask);

    if (ioctl(fd, DMX_SET_FILTER, &sctFilterParams) >= 0)
    {
      dsyslog("Opened handle for filter PID=%u, TID=0x%02X, mask=0x%02X", pid, tid, mask);
      return FilterResourcePtr(new cDvbFilterResource(pid, tid, mask, fd));
    }
    else
    {
      esyslog("ERROR: can't set filter (pid=%u, tid=0x%02X, mask=0x%02X): %s",
          pid, tid, mask, strerror(errno));
      close(fd);
    }
  }
  else
    esyslog("ERROR: can't open filter handle on '%s'", fileName.c_str());

  return FilterResourcePtr();
}

bool cDvbSectionFilterSubsystem::ReadResource(const FilterResourcePtr& handle, std::vector<uint8_t>& data)
{
  //cDvbFilterResource* dvbResource = dynamic_cast<cDvbFilterResource*>(handle.get()); // TODO: Segfaults
  cDvbFilterResource* dvbResource = (cDvbFilterResource*)handle.get();
  if (dvbResource)
  {
    uint8_t buffer[READ_BUFFER_SIZE];

    // Perform a blocking read
    ssize_t read = safe_read(dvbResource->GetFileDescriptor(), buffer, sizeof(buffer));
    if (read > 0)
    {
      data.assign(buffer, buffer + read);
      return true;
    }
  }
  return false;
}

FilterResourcePtr cDvbSectionFilterSubsystem::Poll(const FilterResourceCollection& filterResources)
{
  vector<pollfd> vecPfds;

  vecPfds.reserve(filterResources.size());

  for (FilterResourceCollection::const_iterator it = filterResources.begin(); it != filterResources.end(); ++it)
  {
    FilterResourcePtr resource = *it;

    assert(resource.get());

    //cDvbFilterResource* handle = dynamic_cast<cDvbFilterResource*>(resource.get()); // TODO: Segfaults
    cDvbFilterResource* handle = (cDvbFilterResource*)resource.get();
    if (!handle)
    {
      // Fail hard and fast if vector contains invalid handle
      esyslog("cDvbSectionFilterSubsystem: Encountered empty handle!");
      return FilterResourcePtr();
    }

    pollfd pfd;
    pfd.fd = handle->GetFileDescriptor();
    pfd.events = POLLIN;
    pfd.revents = 0;
    vecPfds.push_back(pfd);
  }

  if (!vecPfds.empty())
  {
    if (poll(vecPfds.data(), vecPfds.size(), POLL_TIMEOUT_MS) > 0)
    {
      // Look for fd that signaled poll()
      int signaledFd = -1;
      for (unsigned int i = 0; i < vecPfds.size(); i++)
      {
        if (vecPfds[i].revents & POLLIN)
        {
          signaledFd = vecPfds[i].fd;
          break;
        }
      }

      // Look for handle that corresponds to fd
      if (signaledFd != -1)
      {
        for (FilterResourceCollection::const_iterator it = filterResources.begin(); it != filterResources.end(); ++it)
        {
          //cDvbFilterResource* handle = dynamic_cast<cDvbFilterResource*>(it->get()); // TODO: Segfaults
          cDvbFilterResource* handle = (cDvbFilterResource*)it->get();
          if (handle && handle->GetFileDescriptor() == signaledFd)
            return *it;
        }
      }
    }
  }
  return FilterResourcePtr();
}

}
