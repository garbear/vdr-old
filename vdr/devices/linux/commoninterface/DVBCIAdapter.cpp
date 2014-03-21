/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "DVBCIAdapter.h"
#include "devices/Device.h"

#include <linux/dvb/ca.h>
#include <sys/ioctl.h>

namespace VDR
{

cDvbCiAdapter::cDvbCiAdapter(cDevice *device, int fd)
 : m_device(device),
   m_fd(fd)
{
  //SetDescription("CI adapter on device %d", m_device->DeviceNumber());

  ca_caps_t Caps;
  if (ioctl(m_fd, CA_GET_CAP, &Caps) == 0)
  {
    if ((Caps.slot_type & CA_CI_LINK) != 0)
    {
      int NumSlots = Caps.slot_num;
      if (NumSlots > 0)
      {
        for (int i = 0; i < NumSlots; i++)
          new cCamSlot(this);
        CreateThread();
      }
      else
      {
        //esyslog("ERROR: no CAM slots found on device %d", m_device->DeviceNumber());
      }
    }
    else
    {
      //isyslog("device %d doesn't support CI link layer interface", m_device->DeviceNumber());
    }
  }
  else
  {
    //esyslog("ERROR: can't get CA capabilities on device %d", m_device->DeviceNumber());
  }
}

cDvbCiAdapter *cDvbCiAdapter::CreateCiAdapter(cDevice *device, int fd)
{
  // TODO check whether a CI is actually present?
  if (device)
    return new cDvbCiAdapter(device, fd);
  return NULL;
}

cDvbCiAdapter::~cDvbCiAdapter()
{
  StopThread(3000);
}

int cDvbCiAdapter::Read(uint8_t *buffer, int maxLength)
{
  if (buffer && maxLength > 0) {
     struct pollfd pfd[1];
     pfd[0].fd = m_fd;
     pfd[0].events = POLLIN;
     if (poll(pfd, 1, CAM_READ_TIMEOUT) > 0 && (pfd[0].revents & POLLIN)) {
        int n = safe_read(m_fd, buffer, maxLength);
        if (n >= 0)
           return n;
        esyslog("ERROR: can't read from CI adapter on device %d: %m", m_device->CardIndex());
        }
     }
  return 0;
}

void cDvbCiAdapter::Write(const uint8_t *buffer, int length)
{
  if (buffer && length > 0) {
     if (safe_write(m_fd, buffer, length) != length)
        esyslog("ERROR: can't write to CI adapter on device %d: %m", m_device->CardIndex());
     }
}

bool cDvbCiAdapter::Reset(int slot)
{
  if (ioctl(m_fd, CA_RESET, 1 << slot) != -1)
     return true;
  else
     esyslog("ERROR: can't reset CAM slot %d on device %d: %m", slot, m_device->CardIndex());
  return false;
}

eModuleStatus cDvbCiAdapter::ModuleStatus(int slot)
{
  ca_slot_info_t sinfo;
  sinfo.num = slot;
  if (ioctl(m_fd, CA_GET_SLOT_INFO, &sinfo) != -1) {
     if ((sinfo.flags & CA_CI_MODULE_READY) != 0)
        return msReady;
     else if ((sinfo.flags & CA_CI_MODULE_PRESENT) != 0)
        return msPresent;
     }
  else
     esyslog("ERROR: can't get info of CAM slot %d on device %d: %m", slot, m_device->CardIndex());
  return msNone;
}

bool cDvbCiAdapter::Assign(DevicePtr device, bool query /* = false */)
{
  // The CI is hardwired to its device, so there's not really much to do here
  if (device)
     return device.get() == m_device;
  return true;
}

}
