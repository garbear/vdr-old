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

#include "DeviceReceiverSubsystem.h"
#include "DeviceChannelSubsystem.h"
#include "DeviceCommonInterfaceSubsystem.h"
#include "DevicePIDSubsystem.h"
#include "DevicePlayerSubsystem.h"
#include "devices/commoninterface/CI.h"
#include "devices/Device.h"
#include "Config.h"
#include "devices/Receiver.h"

#include <algorithm>

using namespace std;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))
#endif

// activate the following line if you need it - actually the driver should be fixed!
#define WAIT_FOR_TUNER_LOCK 0

cDeviceReceiverSubsystem::cDeviceReceiverSubsystem(cDevice *device)
 : cDeviceSubsystem(device)
{
  for (int i = 0; i < ARRAY_SIZE(m_receivers); i++)
    m_receivers[i] = NULL;
}

int cDeviceReceiverSubsystem::Priority()
{
  int priority = IDLEPRIORITY;
  if (Device()->IsPrimaryDevice() && !Player()->Replaying() && Channel()->HasProgramme())
    priority = TRANSFERPRIORITY; // we use the same value here, no matter whether it's actual Transfer Mode or real live viewing

  PLATFORM::CLockObject lock(m_mutexReceiver);

  for (int i = 0; i < MAXRECEIVERS; i++)
  {
    if (m_receivers[i])
      priority = std::max(m_receivers[i]->priority, priority);
  }
  return priority;
}

bool cDeviceReceiverSubsystem::Receiving()
{
  PLATFORM::CLockObject lock(m_mutexReceiver);
  for (int i = 0; i < ARRAY_SIZE(m_receivers); i++)
  {
    if (m_receivers[i])
      return true;
  }
  return false;
}

bool cDeviceReceiverSubsystem::AttachReceiver(cReceiver *receiver)
{
  if (!receiver)
    return false;
  if (receiver->device == Device())
    return true;
#if WAIT_FOR_TUNER_LOCK
#define TUNER_LOCK_TIMEOUT 5000 // ms
  if (!Channel()->HasLock(TUNER_LOCK_TIMEOUT))
  {
    esyslog("ERROR: device %d has no lock, can't attach receiver!", Device()->CardIndex() + 1);
    return false;
  }
#endif
  PLATFORM::CLockObject lock(m_mutexReceiver);
  for (int i = 0; i < ARRAY_SIZE(m_receivers); i++)
  {
    if (!m_receivers[i])
    {
      for (int n = 0; n < receiver->numPids; n++)
      {
        if (!PID()->AddPid(receiver->pids[n]))
        {
          for ( ; n-- > 0; )
            PID()->DelPid(receiver->pids[n]);
          return false;
        }
      }
      receiver->Activate(true);
      Device()->Lock();
      receiver->device = Device();
      m_receivers[i] = receiver;
      Device()->Unlock();
      if (CommonInterface()->m_camSlot)
      {
        CommonInterface()->m_camSlot->StartDecrypting();
        CommonInterface()->m_startScrambleDetection = time(NULL);
      }
      Device()->CreateThread();
      return true;
    }
  }
  esyslog("ERROR: no free receiver slot!");
  return false;
}

void cDeviceReceiverSubsystem::Detach(cReceiver *receiver)
{
  if (!receiver || receiver->device != Device())
    return;
  bool receiversLeft = false;
  PLATFORM::CLockObject lock(m_mutexReceiver);
  for (int i = 0; i < ARRAY_SIZE(m_receivers); i++)
  {
    if (m_receivers[i] == receiver)
    {
      Device()->Lock();
      m_receivers[i] = NULL;
      receiver->device = NULL;
      Device()->Unlock();
      receiver->Activate(false);
      for (int n = 0; n < receiver->numPids; n++)
        PID()->DelPid(receiver->pids[n]);
    }
    else if (m_receivers[i])
      receiversLeft = true;
  }
  if (CommonInterface()->m_camSlot)
    CommonInterface()->m_camSlot->StartDecrypting();
  if (!receiversLeft)
    Device()->StopThread();
}

void cDeviceReceiverSubsystem::DetachAll(int pid)
{
  if (pid)
  {
    PLATFORM::CLockObject lock(m_mutexReceiver);
    for (int i = 0; i < ARRAY_SIZE(m_receivers); i++)
    {
      cReceiver *receiver = m_receivers[i];
      if (receiver && receiver->WantsPid(pid))
        Detach(receiver);
    }
  }
}

void cDeviceReceiverSubsystem::DetachAllReceivers()
{
  PLATFORM::CLockObject lock(m_mutexReceiver);
  for (int i = 0; i < ARRAY_SIZE(m_receivers); i++)
    Detach(m_receivers[i]);
}
