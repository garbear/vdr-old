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

#include "DVBReceiverSubsystem.h"
#include "devices/linux/DVBDevice.h"
#include "devices/TSBuffer.h"
#include "utils/CommonMacros.h"

#include <fcntl.h>
#include <unistd.h>

namespace VDR
{

cDvbReceiverSubsystem::cDvbReceiverSubsystem(cDevice *device)
 : cDeviceReceiverSubsystem(device),
   m_tsBuffer(NULL),
   m_fd_dvr(-1)
{
}

bool cDvbReceiverSubsystem::OpenDvr()
{
  CloseDvr();
  m_fd_dvr = GetDevice<cDvbDevice>()->DvbOpen(DEV_DVB_DVR, O_RDONLY | O_NONBLOCK);
  if (m_fd_dvr >= 0)
    m_tsBuffer = new cTSBuffer(m_fd_dvr, MEGABYTE(5), Device()->CardIndex() + 1);
  return m_fd_dvr >= 0;
}

void cDvbReceiverSubsystem::CloseDvr()
{
  if (m_fd_dvr >= 0)
  {
    delete m_tsBuffer;
    m_tsBuffer = NULL;
    close(m_fd_dvr);
    m_fd_dvr = -1;
  }
}

bool cDvbReceiverSubsystem::GetTSPacket(uint8_t *&Data)
{
  if (m_tsBuffer)
  {
    Data = m_tsBuffer->Get();
    return true;
  }
  return false;
}

void cDvbReceiverSubsystem::DetachAllReceivers()
{
  PLATFORM::CLockObject lock(GetDevice<cDvbDevice>()->m_bondMutex);
  cDvbDevice *d = GetDevice<cDvbDevice>();

  do
  {
    d->Receiver()->cDeviceReceiverSubsystem::DetachAllReceivers();
    d = d->m_bondedDevice;
  } while (d && d !=  GetDevice<cDvbDevice>() && GetDevice<cDvbDevice>()->m_bNeedsDetachBondedReceivers);

  GetDevice<cDvbDevice>()->m_bNeedsDetachBondedReceivers = false;
}

}
