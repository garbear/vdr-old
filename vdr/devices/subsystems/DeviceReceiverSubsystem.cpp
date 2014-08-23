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

#include "DeviceReceiverSubsystem.h"
#include "DeviceCommonInterfaceSubsystem.h"
#include "Config.h"
#include "devices/commoninterface/CI.h"
#include "devices/Device.h"
#include "devices/Receiver.h"
#include "devices/Remux.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"

#include <algorithm>
#include <assert.h>

using namespace PLATFORM;
using namespace std;

#define TS_SCRAMBLING_TIMEOUT     3 // seconds to wait until a TS becomes unscrambled
#define TS_SCRAMBLING_TIME_OK    10 // seconds before a Channel/CAM combination is marked as known to decrypt

namespace VDR
{

cDeviceReceiverSubsystem::cDeviceReceiverSubsystem(cDevice *device)
 : cDeviceSubsystem(device),
   m_pidHandles(),
   m_ringBuffer(MEGABYTE(5), TS_SIZE, true, "TS"),
   m_bDelivered(false)
{
}

cDeviceReceiverSubsystem::~cDeviceReceiverSubsystem(void)
{
  DetachAllReceivers();
}

void *cDeviceReceiverSubsystem::Process()
{
  if (!IsStopped() && OpenDvr())
  {
    while (!IsStopped())
    {
      Read(m_ringBuffer);

      uint8_t *b = NULL;
      if (!GetTSPacket(b))
        break;

      if (!b)
        continue;

      std::vector<uint8_t> buffer;
      buffer.assign(b, b + TS_SIZE);
      assert(buffer.size() == TS_SIZE);

      uint16_t Pid = TsPid(b);

      // Distribute the packet to all attached receivers:
      CLockObject lock(m_mutexReceiver);
      for (ReceiverPidMap::iterator itPair = m_receiverPids.begin(); itPair != m_receiverPids.end(); ++itPair)
      {
        iReceiver* receiver = itPair->first;
        const std::set<uint16_t>& pids = itPair->second;

        if (pids.find(Pid) != pids.end())
          receiver->Receive(buffer);
      }
    }

    CloseDvr();
  }
  return NULL;
}

bool cDeviceReceiverSubsystem::GetTSPacket(uint8_t*& data)
{
  int count = 0;
  if (m_bDelivered)
  {
    m_ringBuffer.Del(TS_SIZE);
    m_bDelivered = false;
  }
  uint8_t* p = m_ringBuffer.Get(count);
  if (p && count >= TS_SIZE)
  {
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
      esyslog("ERROR: skipped %d bytes to sync on TS packet on device %d", count, Device()->Index());
      return false;
    }

    m_bDelivered = true;
    data = p;
    return true;
  }
  return false;
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, const ChannelPtr& channel)
{
  CLockObject lock(m_mutexReceiver);

  if (m_receiverPids.find(receiver) != m_receiverPids.end())
  {
    dsyslog("Receiver already attached, skipping");
    return false;
  }

  // TODO
  set<uint16_t> pids = channel->GetPids();
  if (pids.empty())
    return false;
  for (set<uint16_t>::const_iterator itPid = pids.begin(); itPid != pids.end(); ++itPid)
  {
    if (!AddPid(*itPid))
    {
      for (set<uint16_t>::const_iterator itPid2 = pids.begin(); itPid2 != itPid; ++itPid)
        DeletePid(*itPid2);
      return false;
    }
  }

  receiver->Start();

  m_receiverPids[receiver] = pids;

  if (CommonInterface()->m_camSlot)
  {
    CommonInterface()->m_camSlot->StartDecrypting();
    CommonInterface()->m_startScrambleDetection = time(NULL);
  }

  if (!IsRunning())
    CreateThread();

  dsyslog("receiver %p attached to %p", receiver, this);

  return true;
}

void cDeviceReceiverSubsystem::DetachReceiver(iReceiver* receiver)
{
  CLockObject lock(m_mutexReceiver);

  ReceiverPidMap::iterator it = m_receiverPids.find(receiver);
  if (it == m_receiverPids.end())
  {
    dsyslog("Receiver is not attached?");
    return;
  }

  receiver->Stop();

  const set<uint16_t>& pids = it->second;
  for (set<uint16_t>::const_iterator itPid = pids.begin(); itPid != pids.end(); ++itPid)
    DeletePid(*itPid);

  m_receiverPids.erase(it);

  if (CommonInterface()->m_camSlot)
    CommonInterface()->m_camSlot->StartDecrypting();

  if (m_receiverPids.empty())
    StopThread(0);

  dsyslog("receiver %p detached from %p", receiver, this);
}

void cDeviceReceiverSubsystem::DetachAllReceivers(void)
{
  CLockObject lock(m_mutexReceiver);
  while (!m_receiverPids.empty())
    DetachReceiver(m_receiverPids.begin()->first);
}

bool cDeviceReceiverSubsystem::Receiving(void) const
{
  CLockObject lock(m_mutexReceiver);
  return !m_receiverPids.empty();
}

bool cDeviceReceiverSubsystem::AddPid(uint16_t pid, ePidType pidType /* = ptOther */, uint8_t StreamType /* = 0 */)
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
        if (!SetPid(m_pidHandles[n], (ePidType)n, true))
        {
          esyslog("ERROR: can't set PID %d on device %d", pid, Device()->Index());
          if (pidType <= ptTeletext)
            Receiver()->DetachAll(pid);
          DeletePid(pid, pidType);
          return false;
        }

        if (CommonInterface()->m_camSlot)
          CommonInterface()->m_camSlot->SetPid(pid, true);
      }
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
      esyslog("ERROR: no free slot for PID %d on device %d", pid, Device()->Index());
      return false;
    }

    if (n >= 0)
    {
      m_pidHandles[n].pid = pid;
      m_pidHandles[n].streamType = StreamType;
      m_pidHandles[n].used = 1;

      if (!SetPid(m_pidHandles[n], (ePidType)n, true))
      {
        esyslog("ERROR: can't set PID %d on device %d", pid, Device()->Index());
        if (pidType <= ptTeletext)
          Receiver()->DetachAll(pid);
        DeletePid(pid, pidType);
        return false;
      }

      if (CommonInterface()->m_camSlot)
        CommonInterface()->m_camSlot->SetPid(pid, true);
    }
  }
  return true;
}

bool cDeviceReceiverSubsystem::HasPid(uint16_t pid) const
{
  CLockObject lock(m_mutexReceiver);

  for (int i = 0; i < ARRAY_SIZE(m_pidHandles); i++)
  {
    if (m_pidHandles[i].pid == pid)
      return true;
  }
  return false;
}

void cDeviceReceiverSubsystem::DeletePid(uint16_t pid, ePidType pidType /* = ptOther */)
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
      if (--m_pidHandles[n].used < 2)
      {
        SetPid(m_pidHandles[n], (ePidType)n, false);
        if (m_pidHandles[n].used == 0)
        {
          m_pidHandles[n].handle = -1;
          m_pidHandles[n].pid = 0;

          if (CommonInterface()->m_camSlot)
            CommonInterface()->m_camSlot->SetPid(pid, false);
        }
      }
    }
  }
}

void cDeviceReceiverSubsystem::DetachAll(uint16_t pid)
{
  if (pid)
  {
    dsyslog("detaching all receivers for pid %d from %p", pid, this);
    PLATFORM::CLockObject lock(m_mutexReceiver);

    ReceiverPidMap receiverPidsCopy = m_receiverPids;

    for (ReceiverPidMap::iterator itPair = receiverPidsCopy.begin(); itPair != receiverPidsCopy.end(); ++itPair)
    {
      iReceiver* receiver = itPair->first;
      const set<uint16_t>& pids = itPair->second;

      if (pids.find(pid) != pids.end())
        DetachReceiver(receiver);
    }
  }
}

}
