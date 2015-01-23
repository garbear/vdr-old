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

#include "DVBReceiverSubsystem.h"
#include "devices/Remux.h"
#include "devices/linux/DVBDevice.h"
#include "dvb/PsiBuffer.h" // for PSI_MAX_SIZE
#include "filesystem/File.h"
#include "filesystem/Poller.h"
#include "utils/log/Log.h"
#include "utils/Ringbuffer.h"
#include "utils/StringUtils.h"
#include "utils/Tools.h"

#include <fcntl.h>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <linux/dvb/dmx.h>
#include <unistd.h>

using namespace std;

#define TS_PACKET_BUFFER_SIZE    (25 * TS_SIZE) // Buffer up to 25 TS packets
#define FILE_DESCRIPTOR_INVALID  (-1)
#define POLL_TIMEOUT_MS          1000

#define PID_DEBUGGING(x...) dsyslog(x)
//#define PID_DEBUGGING(x...) {}

namespace VDR
{

// --- cDvbResource ------------------------------------------------

enum RESOURCE_TYPE
{
  RESOURCE_TYPE_STREAMING,
  RESOURCE_TYPE_MULTIPLEXING
};

class cDvbResource : public cPidResource
{
public:
  cDvbResource(uint16_t pid, RESOURCE_TYPE type, const cDvbDevice* device)
   : cPidResource(pid),
     m_handle(FILE_DESCRIPTOR_INVALID),
     m_device(device),
     m_type(type)
  {
  }

  virtual ~cDvbResource(void) { }

  RESOURCE_TYPE Type(void) const { return m_type; }
  int Handle(void) const { return m_handle; }

  virtual void Close(void);

protected:
  int                     m_handle;
  const cDvbDevice* const m_device;
  PLATFORM::CMutex        m_mutex;

private:
  const RESOURCE_TYPE     m_type;
};

void cDvbResource::Close(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (m_handle != FILE_DESCRIPTOR_INVALID)
  {
    if (ioctl(m_handle, DMX_STOP) < 0)
      LOG_ERROR;

    close(m_handle);
    m_handle = FILE_DESCRIPTOR_INVALID;
    PID_DEBUGGING("Closed %s", ToString().c_str());
  }
}

// --- cDvbStreamingResource ------------------------------------------------

class cDvbStreamingResource : public cDvbResource
{
public:
  cDvbStreamingResource(uint16_t pid, uint8_t tid, uint8_t mask, const cDvbDevice* device)
   : cDvbResource(pid, RESOURCE_TYPE_STREAMING, device),
     m_tid(tid),
     m_mask(mask)
  {
  }

  virtual ~cDvbStreamingResource(void) { Close(); }

  virtual bool Equals(const cPidResource* other) const;
  virtual bool Equals(uint16_t pid) const { return false; }

  virtual bool Open(void);

  virtual bool Read(const uint8_t** outdata, size_t* outlen);

  uint8_t Tid(void) const  { return m_tid; }
  uint8_t Mask(void) const { return m_mask; }

  virtual std::string ToString(void) const;

private:
  uint8_t m_tid;
  uint8_t m_mask;
};

bool cDvbStreamingResource::Equals(const cPidResource* other) const
{
  const cDvbStreamingResource* dvbOther = dynamic_cast<const cDvbStreamingResource*>(other);
  return dvbOther                   &&
         Pid()  == dvbOther->Pid()  &&
         Tid()  == dvbOther->Tid()  &&
         Mask() == dvbOther->Mask();
}

bool cDvbStreamingResource::Open(void)
{
  // Calculate strings
  PLATFORM::CLockObject lock(m_mutex);
  if (m_handle == FILE_DESCRIPTOR_INVALID)
  {
    m_handle = open(m_device->DvbPath(DEV_DVB_DEMUX).c_str(), O_RDWR | O_NONBLOCK);
    if (m_handle == FILE_DESCRIPTOR_INVALID)
    {
      esyslog("Couldn't open %s: invalid handle", ToString().c_str());
      return false;
    }

    dmx_sct_filter_params sctFilterParams = { };

    sctFilterParams.pid = Pid();
    sctFilterParams.timeout = 0; // seconds to wait for section to be loaded, 0 == no timeout
    sctFilterParams.flags = DMX_IMMEDIATE_START;
    sctFilterParams.filter.filter[0] = m_tid;
    sctFilterParams.filter.mask[0] = m_mask;

    if (ioctl(m_handle, DMX_SET_FILTER, &sctFilterParams) < 0)
    {
      esyslog("Couldn't open %s: ioctl failed", ToString().c_str());
      Close();
      return false;
    }
  }

  assert(m_handle >= 0);
  PID_DEBUGGING("Opened %s", ToString().c_str());

  return true;
}

bool cDvbStreamingResource::Read(const uint8_t** outdata, size_t* outlen)
{
  cPsiBuffer* buffer = Buffer();
  if (!buffer)
    return false;

  ssize_t bytesRead = safe_read(m_handle, buffer->Data(), PSI_MAX_SIZE);
  if (read > 0)
  {
    *outdata = buffer->Data();
    *outlen  = bytesRead;
    return true;
  }
  return false;
}

std::string cDvbStreamingResource::ToString(void) const
{
  return StringUtils::Format("[PID 0x%04X, TID 0x%02X, MASK 0x%02X]", Pid(), m_tid, m_mask);
}

// --- cDvbMultiplexedResource ------------------------------------------------

class cDvbMultiplexedResource : public cDvbResource
{
public:
  cDvbMultiplexedResource(uint16_t pid, STREAM_TYPE streamType, const cDvbDevice* device)
   : cDvbResource(pid, RESOURCE_TYPE_MULTIPLEXING, device),
     m_streamType(streamType)
  {
  }

  virtual ~cDvbMultiplexedResource(void) { Close(); }

  virtual bool Equals(const cPidResource* other) const;
  virtual bool Equals(uint16_t pid) const { return Pid() == pid; }

  virtual bool Open(void);

  uint8_t StreamType(void) const { return m_streamType; }
  std::string ToString(void) const;

private:
  const STREAM_TYPE m_streamType;
};

bool cDvbMultiplexedResource::Equals(const cPidResource* other) const
{
  const cDvbMultiplexedResource* dvbOther = dynamic_cast<const cDvbMultiplexedResource*>(other);
  return dvbOther && Pid() == dvbOther->Pid();
}

bool cDvbMultiplexedResource::Open(void)
{
  // Calculate strings
  PLATFORM::CLockObject lock(m_mutex);
  if (m_handle == FILE_DESCRIPTOR_INVALID)
  {
    m_handle = open(m_device->DvbPath(DEV_DVB_DEMUX).c_str(), O_RDWR | O_NONBLOCK);
    if (m_handle == FILE_DESCRIPTOR_INVALID)
    {
      esyslog("Couldn't open %s: invalid handle", ToString().c_str());
      return false;
    }

    dmx_pes_filter_params pesFilterParams = { };

    pesFilterParams.pid     = Pid();
    pesFilterParams.input   = DMX_IN_FRONTEND;
    pesFilterParams.output  = DMX_OUT_TS_TAP;
    pesFilterParams.pes_type= DMX_PES_OTHER;
    pesFilterParams.flags   = DMX_IMMEDIATE_START;

    if (ioctl(m_handle, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
    {
      esyslog("Couldn't open %s: ioctl failed", ToString().c_str());
      Close();
      return false;
    }
  }

  assert(m_handle >= 0);
  PID_DEBUGGING("Opened %s", ToString().c_str());

  return true;
}

std::string cDvbMultiplexedResource::ToString(void) const
{
  return StringUtils::Format("[PID 0x%04X, Type 0x%02X]", Pid(), StreamType());
}

// --- cDvbReceiverSubsystem -----------------------------------------------

cDvbReceiverSubsystem::cDvbReceiverSubsystem(cDevice *device)
 : cDeviceReceiverSubsystem(device),
   m_fd_dvr(FILE_DESCRIPTOR_INVALID),
   m_ringBuffer(TS_PACKET_BUFFER_SIZE, TS_SIZE, false, "TS")
{
}

bool cDvbReceiverSubsystem::Initialise(void)
{
  Deinitialise();

  m_fd_dvr = open(Device<cDvbDevice>()->DvbPath(DEV_DVB_DVR).c_str(), O_RDONLY | O_NONBLOCK);
  if (m_fd_dvr == FILE_DESCRIPTOR_INVALID)
    return false;

  return true;
}

void cDvbReceiverSubsystem::Deinitialise(void)
{
  if (m_fd_dvr >= 0)
  {
    close(m_fd_dvr);
    m_fd_dvr = FILE_DESCRIPTOR_INVALID;
  }

  m_ringBuffer.Clear();
}

POLL_RESULT cDvbReceiverSubsystem::Poll(PidResourcePtr& streamingResource)
{
  bool bMultiplexedResources = false; // Set to true if any resources are multiplexed

  // Build a list of file descriptors to poll
  vector<pollfd> vecPfds;

  // Add the file descriptors of all streaming resources
  std::set<PidResourcePtr> resources = GetResources();
  for (std::set<PidResourcePtr>::const_iterator it = resources.begin(); it != resources.end(); ++it)
  {
    const cDvbResource* resource = dynamic_cast<const cDvbResource*>(it->get());
    if (resource && resource->Type() == RESOURCE_TYPE_STREAMING)
    {
      pollfd pfd = { resource->Handle(), POLLIN | POLLERR, 0 };
      vecPfds.push_back(pfd);
    }
    else if (resource && resource->Type() == RESOURCE_TYPE_MULTIPLEXING)
    {
      bMultiplexedResources = true;
    }
  }

  // Add the file descriptor for the multiplexed resources
  if (bMultiplexedResources)
  {
    pollfd pfd = { m_fd_dvr, POLLIN | POLLERR, 0 };
    vecPfds.push_back(pfd);
  }

  if (!vecPfds.empty())
  {
    if (poll(vecPfds.data(), vecPfds.size(), POLL_TIMEOUT_MS) > 0)
    {
      // Look for file descriptor that signaled poll()
      for (unsigned int i = 0; i < vecPfds.size(); i++)
      {
        if (vecPfds[i].revents & (POLLIN | POLLERR))
        {
          const int signaledFd = vecPfds[i].fd;

          // Check if the multiplexed resource is ready
          if (signaledFd == m_fd_dvr)
            return POLL_RESULT_MULTIPLEXED_READY;

          // Check if a streaming resource is ready
          for (std::set<PidResourcePtr>::const_iterator it = resources.begin(); it != resources.end(); ++it)
          {
            const cDvbResource* resource = dynamic_cast<const cDvbResource*>(it->get());
            if (resource && resource->Handle() == signaledFd)
            {
              streamingResource = *it;
              return POLL_RESULT_STREAMING_READY;
            }
          }
        }
      }
    }
  }

  return POLL_RESULT_NOT_READY;
}

void cDvbReceiverSubsystem::Consumed(void)
{
  m_ringBuffer.Del(TS_SIZE);
}

TsPacket cDvbReceiverSubsystem::ReadMultiplexed(void)
{
  int count = 0;
  if (!m_ringBuffer.Get(count) || count < TS_SIZE)
  {
    if (m_ringBuffer.Read(m_fd_dvr) <= 0)
    {
      if (errno == EOVERFLOW)
        esyslog("Driver buffer overflow on device %d", Device()->Index());
      else if (errno != 0 && errno != EAGAIN && errno != EINTR)
        esyslog("Error reading dvr device: %m");
      return NULL;
    }
  }

  uint8_t* p = m_ringBuffer.Get(count);
  while (p && count >= TS_SIZE)
  {
    // Check for TS sync byte
    if (p[0] != TS_SYNC_BYTE)
    {
      for (int i = 1; i < count; i++)
      {
        if (p[i] == TS_SYNC_BYTE)
        {
          count = i;
          break;
        }
      }

      m_ringBuffer.Del(count);
      esyslog("Skipped %d bytes to sync on TS packet on device %d", count, Device()->Index());
      return NULL;
    }

    return p;
  }

  return NULL;
}

cDeviceReceiverSubsystem::PidResourcePtr cDvbReceiverSubsystem::CreateStreamingResource(uint16_t pid, uint8_t tid, uint8_t mask)
{
  return PidResourcePtr(new cDvbStreamingResource(pid, tid, mask, Device<cDvbDevice>()));
}

cDeviceReceiverSubsystem::PidResourcePtr cDvbReceiverSubsystem::CreateMultiplexedResource(uint16_t pid, STREAM_TYPE streamType)
{
  return PidResourcePtr(new cDvbMultiplexedResource(pid, streamType, Device<cDvbDevice>()));
}

}
