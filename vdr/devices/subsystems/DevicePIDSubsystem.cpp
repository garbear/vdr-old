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

#include "DevicePIDSubsystem.h"
#include "DeviceCommonInterfaceSubsystem.h"
#include "DeviceReceiverSubsystem.h"
#include "devices/commoninterface/CI.h"
#include "devices/Device.h"
#include "utils/Tools.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))
#endif

#define PID_DEBUG 0

cDevicePIDSubsystem::cDevicePIDSubsystem(cDevice *device)
 : cDeviceSubsystem(device)
{
}

void cDevicePIDSubsystem::DelLivePids()
{
  for (int i = ptAudio; i < ptOther; i++)
  {
    if (m_pidHandles[i].pid)
      DelPid(m_pidHandles[i].pid, (ePidType)i);
  }
}

bool cDevicePIDSubsystem::HasPid(int pid) const
{
  for (int i = 0; i < ARRAY_SIZE(m_pidHandles); i++)
  {
    if (m_pidHandles[i].pid == pid)
      return true;
  }
  return false;
}

bool cDevicePIDSubsystem::AddPid(int pid, ePidType pidType /* = ptOther */, int StreamType /* = 0 */)
{
  if (pid || pidType == ptPcr)
  {
    int n = -1;
    int a = -1;
    if (pidType != ptPcr) // PPID always has to be explicit
    {
      for (int i = 0; i < ARRAY_SIZE(m_pidHandles); i++)
      {
        if (i != ptPcr)
        {
          if (m_pidHandles[i].pid == pid)
            n = i;
          else if (a < 0 && i >= ptOther && !m_pidHandles[i].used)
            a = i;
        }
      }
    }
    if (n >= 0)
    {
      // The pid is already in use
      if (++m_pidHandles[n].used == 2 && n <= ptTeletext)
      {
        // It's a special PID that may have to be switched into "tap" mode
        PrintPIDs("A");
        if (!SetPid(m_pidHandles[n], n, true))
        {
          esyslog("ERROR: can't set PID %d on device %d", pid, Device()->CardIndex() + 1);
          if (pidType <= ptTeletext)
            Receiver()->DetachAll(pid);
          DelPid(pid, pidType);
          return false;
        }
        if (CommonInterface()->m_camSlot)
          CommonInterface()->m_camSlot->SetPid(pid, true);
      }
      PrintPIDs("a");
      return true;
    }
    else if (pidType < ptOther)
    {
      // The pid is not yet in use and it is a special one
      n = pidType;
    }
    else if (a >= 0)
    {
      // The pid is not yet in use and we have a free slot
      n = a;
    }
    else
    {
      esyslog("ERROR: no free slot for PID %d on device %d", pid, Device()->CardIndex() + 1);
      return false;
    }
    if (n >= 0)
    {
      m_pidHandles[n].pid = pid;
      m_pidHandles[n].streamType = StreamType;
      m_pidHandles[n].used = 1;
      PrintPIDs("C");
      if (!SetPid(m_pidHandles[n], n, true))
      {
        esyslog("ERROR: can't set PID %d on device %d", pid, Device()->CardIndex() + 1);
        if (pidType <= ptTeletext)
          Receiver()->DetachAll(pid);
        DelPid(pid, pidType);
        return false;
      }
      if (CommonInterface()->m_camSlot)
        CommonInterface()->m_camSlot->SetPid(pid, true);
    }
  }
  return true;
}

void cDevicePIDSubsystem::DelPid(int pid, ePidType pidType /* = ptOther */)
{
  if (pid || pidType == ptPcr)
  {
    int n = -1;
    if (pidType == ptPcr)
      n = pidType; // PPID always has to be explicit
    else
    {
      for (int i = 0; i < ARRAY_SIZE(m_pidHandles); i++)
      {
        if (m_pidHandles[i].pid == pid)
        {
          n = i;
          break;
        }
      }
    }
    if (n >= 0 && m_pidHandles[n].used)
    {
      PrintPIDs("D");
      if (--m_pidHandles[n].used < 2)
      {
        SetPid(m_pidHandles[n], n, false);
        if (m_pidHandles[n].used == 0)
        {
          m_pidHandles[n].handle = -1;
          m_pidHandles[n].pid = 0;
          if (CommonInterface()->m_camSlot)
            CommonInterface()->m_camSlot->SetPid(pid, false);
        }
      }
      PrintPIDs("E");
    }
  }
}

void cDevicePIDSubsystem::PrintPIDs(const char *s)
{
#if PID_DEBUG
  char b[500];
  char *q = b;
  q += sprintf(q, "%d %s ", Device()->CardIndex(), s);
  for (int i = 0; i < ARRAY_SIZE(m_pidHandles); i++)
    q += sprintf(q, " %s%4d %d", i == ptOther ? "* " : "", m_pidHandles[i].pid, m_pidHandles[i].used);
  dsyslog("%s", b);
#endif
}
