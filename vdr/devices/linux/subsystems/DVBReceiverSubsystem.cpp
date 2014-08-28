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
#include "filesystem/Poller.h"
#include "utils/log/Log.h"
#include "utils/Ringbuffer.h"

#include <fcntl.h>
#include <string>
#include <sys/ioctl.h>
#include <linux/dvb/dmx.h>
#include <unistd.h>

using namespace std;

#define TS_PACKET_BUFFER_SIZE    (5 * TS_SIZE) // Buffer up to 5 TS packets
#define FILE_DESCRIPTOR_INVALID  (-1)

namespace VDR
{

// --- cDvbReceiverResource ------------------------------------------------

class cDvbReceiverResource : public cPidResource
{
public:
  cDvbReceiverResource(uint16_t pid, STREAM_TYPE streamType, const std::string& strDvbPath);
  virtual ~cDvbReceiverResource(void) { Close(); }

  virtual bool Open(void);
  virtual void Close(void);

  uint8_t StreamType(void) const { return m_streamType; }

private:
  const STREAM_TYPE m_streamType;
  const std::string m_strDvbPath;
  int               m_handle;
};

typedef shared_ptr<cDvbReceiverResource> DvbReceiverResourcePtr;

cDvbReceiverResource::cDvbReceiverResource(uint16_t pid, STREAM_TYPE streamType, const std::string& strDvbPath)
 : cPidResource(pid),
   m_streamType(streamType),
   m_strDvbPath(strDvbPath),
   m_handle(FILE_DESCRIPTOR_INVALID)
{
}

bool cDvbReceiverResource::Open(void)
{
  if (m_handle == FILE_DESCRIPTOR_INVALID)
  {
    m_handle = open(m_strDvbPath.c_str(), O_RDWR | O_NONBLOCK);
    if (m_handle == FILE_DESCRIPTOR_INVALID)
    {
      esyslog("Couldn't open PES filter (pid=%u, streamType=%u)", Pid(), m_streamType);
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
      esyslog("Couldn't set PES filter (pid=%u, streamType=%u)", Pid(), m_streamType);
      Close();
      return false;
    }
  }

  assert(m_handle >= 0);
  dsyslog("Opened PES filter (pid=%u, streamType=%u)", Pid(), m_streamType);

  return true;
}

void cDvbReceiverResource::Close(void)
{
  if (m_handle != FILE_DESCRIPTOR_INVALID)
  {
    if (ioctl(m_handle, DMX_STOP) < 0)
      LOG_ERROR;

    /* TODO: Why was this called on close?

    if (StreamType() <= ptTeletext)
    {
      dmx_pes_filter_params pesFilterParams = { };

      pesFilterParams.pid     = 0x1FFF;
      pesFilterParams.input   = DMX_IN_FRONTEND;
      pesFilterParams.output  = DMX_OUT_DECODER;
      pesFilterParams.pes_type= DMX_PES_OTHER;
      pesFilterParams.flags   = DMX_IMMEDIATE_START;

      if (ioctl(m_handle, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
        LOG_ERROR;
    }

    */

    close(m_handle);
    m_handle = FILE_DESCRIPTOR_INVALID;
    dsyslog("Closed PES filter (pid=%u, streamType=%u)");
  }
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

bool cDvbReceiverSubsystem::Poll(void)
{
  cPoller Poller(m_fd_dvr);
  return Poller.Poll(100);
}

bool cDvbReceiverSubsystem::Read(vector<uint8_t>& data)
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
      return false;
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
      return false;
    }

    m_ringBuffer.Del(TS_SIZE);
    data.assign(p, p + TS_SIZE);
    return true;
  }

  return false;
}

PidResourcePtr cDvbReceiverSubsystem::OpenResource(uint16_t pid, STREAM_TYPE streamType)
{
  std::string strDvbPath = Device<cDvbDevice>()->DvbPath(DEV_DVB_DEMUX);
  PidResourcePtr handle = PidResourcePtr(new cDvbReceiverResource(pid, streamType, strDvbPath));

  if (!handle->Open())
    handle.reset();

  return handle;
}

}
