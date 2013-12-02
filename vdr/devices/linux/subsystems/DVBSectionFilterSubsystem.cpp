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

#include <linux/dvb/dmx.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace std;

cDvbSectionFilterSubsystem::cDvbSectionFilterSubsystem(cDevice *device)
 : cDeviceSectionFilterSubsystem(device)
{
}

int cDvbSectionFilterSubsystem::OpenFilter(u_short pid, u_char tid, u_char mask)
{
  string fileName = cDvbDevice::DvbName(DEV_DVB_DEMUX, GetDevice<cDvbDevice>()->m_adapter, GetDevice<cDvbDevice>()->m_frontend);
  int f = open(fileName.c_str(), O_RDWR | O_NONBLOCK);
  if (f >= 0)
  {
    dmx_sct_filter_params sctFilterParams;
    memset(&sctFilterParams, 0, sizeof(sctFilterParams));
    sctFilterParams.pid = pid;
    sctFilterParams.timeout = 0;
    sctFilterParams.flags = DMX_IMMEDIATE_START;
    sctFilterParams.filter.filter[0] = tid;
    sctFilterParams.filter.mask[0] = mask;

    if (ioctl(f, DMX_SET_FILTER, &sctFilterParams) >= 0)
      return f;
    else
    {
      esyslog("ERROR: can't set filter (pid=%d, tid=%02X, mask=%02X): %m", pid, tid, mask);
      close(f);
    }
  }
  else
    esyslog("ERROR: can't open filter handle on '%s'", fileName.c_str());
  return -1;
}

void cDvbSectionFilterSubsystem::CloseFilter(int handle)
{
  close(handle);
}
