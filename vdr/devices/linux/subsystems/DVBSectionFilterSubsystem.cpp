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
#include "devices/PIDResource.h"
#include "devices/linux/DVBDevice.h" // for DEV_DVB_DEMUX
//#include "../../../../sections.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/Tools.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/dvb/dmx.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>

using namespace std;

#define READ_BUFFER_SIZE  4096 // From cSectionHandler
#define POLL_TIMEOUT_MS   1000

#define FILE_DESCRIPTOR_INVALID  (-1)

namespace VDR
{

enum AM_AMX_SOURCE
{
  AM_DMX_SRC_TS0,
  AM_DMX_SRC_TS1,
  AM_DMX_SRC_TS2,
  AM_DMX_SRC_HIU
};

dmx_sct_filter_params ToFilterParams(uint16_t pid, uint8_t tid, uint8_t mask)
{
  dmx_sct_filter_params sctFilterParams = { };
  sctFilterParams.pid = pid;
  sctFilterParams.timeout = 0; // seconds to wait for section to be loaded, 0 == no timeout
  sctFilterParams.flags = 0; // amlogic dvbplayer uses DMX_CHECK_CRC
  sctFilterParams.filter.filter[0] = tid;
  sctFilterParams.filter.mask[0] = mask;
  return sctFilterParams;
}

// --- cDvbFilterResource -----------------------------------------------------

class cDvbFilterResource : public cPidResource
{
public:
  cDvbFilterResource(uint16_t pid, uint8_t tid, uint8_t mask, const std::string& strDvbPath)
    : cPidResource(pid),
      m_tid(tid),
      m_mask(mask),
      m_strDvbPath(strDvbPath),
      m_fileDescriptor(-1)
  {
  }

  // Use RAII pattern to close file when no longer referenced
  virtual ~cDvbFilterResource(void) { Close(); }

  virtual bool Open(void);
  virtual void Close(void);

  uint8_t Tid(void)            const { return m_tid; }
  uint8_t Mask(void)           const { return m_mask; }
  int     FileDescriptor(void) const { return m_fileDescriptor; }

private:
  const uint8_t     m_tid;
  const uint8_t     m_mask;
  const std::string m_strDvbPath;
  int               m_fileDescriptor;
};

typedef shared_ptr<cDvbFilterResource> DvbFilterResourcePtr;

bool cDvbFilterResource::Open(void)
{
  if (m_fileDescriptor == FILE_DESCRIPTOR_INVALID)
  {
    // Don't open with O_NONBLOCK flag so that reads will wait for a full section
    m_fileDescriptor = open(m_strDvbPath.c_str(), O_RDWR);

    if (m_fileDescriptor >= 0)
    {
      dmx_sct_filter_params sctFilterParams = ToFilterParams(Pid(), m_tid, m_mask);

      if (ioctl(m_fileDescriptor, DMX_SET_FILTER, &sctFilterParams) < 0 ||
          ioctl(m_fileDescriptor, DMX_START, 0)                     < 0)
      {
        esyslog("Can't set filter (pid=%u, tid=0x%02X, mask=0x%02X): %m", Pid(), m_tid, m_mask);
        Close();
      }
    }
    else
    {
      esyslog("Can't open filter handle on '%s'", m_strDvbPath.c_str());
    }
  }
  return m_fileDescriptor != FILE_DESCRIPTOR_INVALID;
}

void cDvbFilterResource::Close(void)
{
  if (m_fileDescriptor != FILE_DESCRIPTOR_INVALID)
  {
    close(m_fileDescriptor);
    m_fileDescriptor = FILE_DESCRIPTOR_INVALID;
  }
}

// --- cDvbSectionFilterSubsystem ---------------------------------------------

cDvbSectionFilterSubsystem::cDvbSectionFilterSubsystem(cDevice *device)
 : cDeviceSectionFilterSubsystem(device)
{
}

PidResourcePtr cDvbSectionFilterSubsystem::OpenResource(uint16_t pid, uint8_t tid, uint8_t mask)
{
  // TODO: Magic code that makes everything work on Android
#if defined(TARGET_ANDROID)
  static bool bSetSourceOnce = false;
  if (!bSetSourceOnce)
  {
    bSetSourceOnce = true;

    CFile demuxSource;
    string strPath = StringUtils::Format("/sys/class/stb/demux%d_source", Device<cDvbDevice>()->Frontend());

    if (demuxSource.OpenForWrite(strPath, false))
    {
      AM_AMX_SOURCE src = AM_DMX_SRC_TS2;
      string cmd;

      switch(src)
      {
      case AM_DMX_SRC_TS0:
        cmd = "ts0";
        break;
      case AM_DMX_SRC_TS1:
        cmd = "ts1";
        break;
      case AM_DMX_SRC_TS2:
        cmd = "ts2";
        break;
      case AM_DMX_SRC_HIU:
        cmd = "hiu";
        break;
      default:
        dsyslog("Demux source not supported: %d", src);
      }

      if (!cmd.empty())
        demuxSource.Write(cmd.c_str(), cmd.length());
      demuxSource.Close();
    }
    else
    {
      dsyslog("Can't open %s", strPath.c_str());
    }
  }
#endif

  std::string strDvbPath = Device<cDvbDevice>()->DvbPath(DEV_DVB_DEMUX);
  PidResourcePtr handle = PidResourcePtr(new cDvbFilterResource(pid, tid, mask, strDvbPath));

  if (!handle->Open())
    handle.reset();

  return handle;
}

bool cDvbSectionFilterSubsystem::ReadResource(const PidResourcePtr& handle, std::vector<uint8_t>& data)
{
  cDvbFilterResource* dvbResource = static_cast<cDvbFilterResource*>(handle.get());
  if (dvbResource)
  {
    uint8_t buffer[READ_BUFFER_SIZE];

    // Perform a blocking read
    ssize_t read = safe_read(dvbResource->FileDescriptor(), buffer, sizeof(buffer));
    if (read > 0)
    {
      data.assign(buffer, buffer + read);
      return true;
    }
  }
  return false;
}

PidResourcePtr cDvbSectionFilterSubsystem::Poll(const PidResourceSet& filterResources)
{
  vector<pollfd> vecPfds;

  vecPfds.reserve(filterResources.size());

  for (PidResourceSet::const_iterator it = filterResources.begin(); it != filterResources.end(); ++it)
  {
    PidResourcePtr resource = *it;

    assert(resource.get());

    cDvbFilterResource* handle = static_cast<cDvbFilterResource*>(resource.get());
    if (!handle)
    {
      // Fail hard and fast if vector contains invalid handle
      esyslog("cDvbSectionFilterSubsystem: Encountered empty handle!");
      return PidResourcePtr();
    }

    pollfd pfd;
    pfd.fd = handle->FileDescriptor();
    pfd.events = POLLIN | POLLERR;
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
        if (vecPfds[i].revents & (POLLIN | POLLERR))
        {
          signaledFd = vecPfds[i].fd;
          break;
        }
      }

      // Look for handle that corresponds to fd
      if (signaledFd != -1)
      {
        for (PidResourceSet::const_iterator it = filterResources.begin(); it != filterResources.end(); ++it)
        {
          cDvbFilterResource* handle = static_cast<cDvbFilterResource*>(it->get());
          if (handle && handle->FileDescriptor() == signaledFd)
            return *it;
        }
      }
    }
  }
  return PidResourcePtr();
}

}
