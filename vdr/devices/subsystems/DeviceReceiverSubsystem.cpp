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
#include "DeviceChannelSubsystem.h"
#include "DeviceCommonInterfaceSubsystem.h"
#include "DevicePIDSubsystem.h"
#include "DevicePlayerSubsystem.h"
#include "Config.h"
#include "devices/commoninterface/CI.h"
#include "devices/Device.h"
#include "devices/Receiver.h"
#include "devices/Remux.h"
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
 : cDeviceSubsystem(device)
{
}

void *cDeviceReceiverSubsystem::Process()
{
  if (!IsStopped() && OpenDvr())
  {
    while (!IsStopped())
    {
      // Read data from the DVR device:
      uint8_t *b = NULL;
      if (!GetTSPacket(b))
        break;

      if (!b)
        continue;

      int Pid = TsPid(b);

      // Check whether the TS packets are scrambled:
      bool DetachReceivers = false;
      bool DescramblingOk = false;
      int CamSlotNumber = 0;
      if (CommonInterface()->m_startScrambleDetection)
      {
        cCamSlot *cs = CommonInterface()->CamSlot();
        CamSlotNumber = cs ? cs->SlotNumber() : 0;
        if (CamSlotNumber)
        {
          bool Scrambled = b[3] & TS_SCRAMBLING_CONTROL;
          int t = time(NULL) - CommonInterface()->m_startScrambleDetection;
          if (Scrambled)
          {
            if (t > TS_SCRAMBLING_TIMEOUT)
              DetachReceivers = true;
          }
          else if (t > TS_SCRAMBLING_TIME_OK)
          {
            DescramblingOk = true;
            CommonInterface()->m_startScrambleDetection = 0;
          }
        }
      }

      // Distribute the packet to all attached receivers:
      CLockObject lock(m_mutexReceiver);
      for (std::list<cReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
      {
        cReceiver* receiver = *it;
//        dsyslog("received packet: pid=%d scrambled=%d check receiver %p", Pid, b[3] & TS_SCRAMBLING_CONTROL?1:0, receiver);
        if (receiver->WantsPid(Pid))
        {
          if (DetachReceivers)
          {
            ChannelCamRelations.SetChecked(receiver->ChannelID(), CamSlotNumber);
            Detach(receiver);
          }
          else
          {
            std::vector<uint8_t> buffer;
            buffer.assign(b, b + TS_SIZE);
            assert(buffer.size() == TS_SIZE);
            receiver->Receive(buffer);
          }

          if (DescramblingOk)
            ChannelCamRelations.SetDecrypt(receiver->ChannelID(), CamSlotNumber);
        }
      }
    }
    CloseDvr();
  }
  return NULL;
}

void cDeviceReceiverSubsystem::AttachReceiver(cReceiver* receiver)
{
  {
    CLockObject lock(m_mutexReceiver);
    m_receivers.push_back(receiver);
  }

  if (CommonInterface()->m_camSlot)
  {
    CommonInterface()->m_camSlot->StartDecrypting();
    CommonInterface()->m_startScrambleDetection = time(NULL);
  }

  CreateThread();
  dsyslog("receiver %p attached to %p", receiver, this);
}

void cDeviceReceiverSubsystem::Detach(cReceiver* receiver)
{
  assert(receiver);

  PLATFORM::CLockObject lock(m_mutexReceiver);

  if (std::find(m_receivers.begin(), m_receivers.end(), receiver) == m_receivers.end())
  {
    dsyslog("receiver %p is not attached to %p", receiver, this);
    return;
  }

  for (std::list<cReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
  {
    if (*it == receiver)
    {
      receiver->Activate(false);

      for (set<uint16_t>::const_iterator itPid = receiver->m_pids.begin(); itPid != receiver->m_pids.end(); ++itPid)
        PID()->DelPid(*itPid);

      m_receivers.erase(it);
      break;
    }
  }
  if (CommonInterface()->m_camSlot)
    CommonInterface()->m_camSlot->StartDecrypting();
  if (m_receivers.empty())
    StopThread(0);
  dsyslog("receiver %p detached from %p", receiver, this);
}

void cDeviceReceiverSubsystem::DetachAll(int pid)
{
  if (pid)
  {
    dsyslog("detaching all receivers for pid %d from %p", pid, this);
    PLATFORM::CLockObject lock(m_mutexReceiver);
    for (std::list<cReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
    {
      if ((*it)->WantsPid(pid))
      {
        Detach(*it);
        //XXX
        it = m_receivers.begin();
      }
    }
  }
}

void cDeviceReceiverSubsystem::DetachAllReceivers()
{
  PLATFORM::CLockObject lock(m_mutexReceiver);
  for (std::list<cReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); it = m_receivers.begin())
    Detach(*it);
}

bool cDeviceReceiverSubsystem::OpenVideoInput(const ChannelPtr& channel, cVideoBuffer* videoBuffer)
{
  m_VideoInput.SetVideoBuffer(videoBuffer);

  const set<uint16_t>& pids = m_VideoInput.GetPids();
  for (set<uint16_t>::const_iterator it = pids.begin(); it != pids.end(); ++it)
  {
    if (!PID()->AddPid(*it))
    {
      for (set<uint16_t>::const_iterator it2 = pids.begin(); *it2 != *it; ++it2)
	PID()->DelPid(*it2);

      dsyslog("receiver %p cannot be added to the pid subsys", &m_VideoInput);
      m_VideoInput.ResetMembers();
      return false;
    }
  }

  m_VideoInput.Activate(true);

  AttachReceiver(&m_VideoInput);

  m_VideoInput.SetChannel(channel);
  m_VideoInput.PmtChange();

  return true;
}

void cDeviceReceiverSubsystem::CloseVideoInput(void)
{
  Detach(&m_VideoInput);
  m_VideoInput.ResetMembers();
}

bool cDeviceReceiverSubsystem::Receiving(void) const
{
  PLATFORM::CLockObject lock(m_mutexReceiver);
  return !m_receivers.empty();
}

}
