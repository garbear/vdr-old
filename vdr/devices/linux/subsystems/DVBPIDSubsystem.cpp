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

#include "DVBPIDSubsystem.h"
#include "devices/linux/DVBDevice.h" // for DEV_DVB_DEMUX
#include "utils/CommonMacros.h"
#include "Types.h"

#include <fcntl.h>
#include <linux/dvb/dmx.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace VDR
{

cDvbPIDSubsystem::cDvbPIDSubsystem(cDevice *device)
 : cDevicePIDSubsystem(device)
{
}

bool cDvbPIDSubsystem::SetPid(cPidHandle &handle, ePidType type, bool bOn)
{
  if (handle.pid)
  {
    dmx_pes_filter_params pesFilterParams;
    memset(&pesFilterParams, 0, sizeof(pesFilterParams));

    if (bOn)
    {
      if (handle.handle < 0)
      {
        handle.handle = GetDevice<cDvbDevice>()->DvbOpen(DEV_DVB_DEMUX, O_RDWR | O_NONBLOCK);
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
      CHECK(ioctl(handle.handle, DMX_STOP));

      if (type <= ptTeletext)
      {
        pesFilterParams.pid     = 0x1FFF;
        pesFilterParams.input   = DMX_IN_FRONTEND;
        pesFilterParams.output  = DMX_OUT_DECODER;
        pesFilterParams.pes_type= DMX_PES_OTHER;
        pesFilterParams.flags   = DMX_IMMEDIATE_START;
        CHECK(ioctl(handle.handle, DMX_SET_PES_FILTER, &pesFilterParams));
      }

      close(handle.handle);
      handle.handle = -1;
    }
  }
  return true;
}

}
