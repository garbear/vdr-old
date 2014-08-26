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
#include "devices/linux/DVBDevice.h"
#include "filesystem/Poller.h"
#include "utils/log/Log.h"
#include "utils/Ringbuffer.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/dvb/dmx.h>
#include <unistd.h>

namespace VDR
{

cDvbReceiverSubsystem::cDvbReceiverSubsystem(cDevice *device)
 : cDeviceReceiverSubsystem(device),
   m_fd_dvr(-1)
{
}

bool cDvbReceiverSubsystem::OpenDvr()
{
  CloseDvr();
  m_fd_dvr = open(Device<cDvbDevice>()->DvbPath(DEV_DVB_DVR).c_str(), O_RDONLY | O_NONBLOCK);
  return m_fd_dvr >= 0;
}

void cDvbReceiverSubsystem::CloseDvr()
{
  if (m_fd_dvr >= 0)
  {
    close(m_fd_dvr);
    m_fd_dvr = -1;
  }
}

void cDvbReceiverSubsystem::Read(cRingBufferLinear& ringBuffer)
{
  cPoller Poller(m_fd_dvr);
  if (Poller.Poll(100))
  {
    int r = ringBuffer.Read(m_fd_dvr);
    if (r < 0)
    {
      if (errno == EOVERFLOW)
      {
        esyslog("ERROR: driver buffer overflow on device %d", Device()->Index());
      }
      else if (errno != 0 && errno != EAGAIN && errno != EINTR)
      {
        LOG_ERROR;
        return;
      }
    }
  }
}

bool cDvbReceiverSubsystem::SetPid(cPidHandle& handle, ePidType type, bool bOn)
{
  if (handle.pid)
  {
    dmx_pes_filter_params pesFilterParams;
    memset(&pesFilterParams, 0, sizeof(pesFilterParams));

    if (bOn)
    {
      if (handle.handle < 0)
      {
        handle.handle = open(Device<cDvbDevice>()->DvbPath(DEV_DVB_DEMUX).c_str(), O_RDWR | O_NONBLOCK);
        if (handle.handle < 0)
        {
          LOG_ERROR;
          return false;
        }
      }

      pesFilterParams.pid     = handle.pid;
      pesFilterParams.input   = DMX_IN_FRONTEND;
      pesFilterParams.output  = DMX_OUT_TS_TAP;
      pesFilterParams.pes_type= DMX_PES_OTHER;
      pesFilterParams.flags   = DMX_IMMEDIATE_START;

      if (ioctl(handle.handle, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
      {
        LOG_ERROR;
        return false;
      }
    }
    else if (!handle.used)
    {
      if (ioctl(handle.handle, DMX_STOP) < 0)
        LOG_ERROR;

      if (type <= ptTeletext)
      {
        pesFilterParams.pid     = 0x1FFF;
        pesFilterParams.input   = DMX_IN_FRONTEND;
        pesFilterParams.output  = DMX_OUT_DECODER;
        pesFilterParams.pes_type= DMX_PES_OTHER;
        pesFilterParams.flags   = DMX_IMMEDIATE_START;

        if (ioctl(handle.handle, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
          LOG_ERROR;
      }

      close(handle.handle);
      handle.handle = -1;
    }
  }
  return true;
}

}
