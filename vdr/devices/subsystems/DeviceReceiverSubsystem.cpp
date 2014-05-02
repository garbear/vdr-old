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
#include "utils/log/Log.h"

#include <algorithm>

using namespace std;

namespace VDR
{

cDeviceReceiverSubsystem::cDeviceReceiverSubsystem(cDevice *device)
 : cDeviceSubsystem(device)
{
}

int cDeviceReceiverSubsystem::Priority(void) const
{
  int priority = IDLEPRIORITY;

  PLATFORM::CLockObject lock(m_mutexReceiver);
  for (std::list<cReceiver*>::const_iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
    priority = std::max((*it)->Priority(), priority);

  return priority;
}

bool cDeviceReceiverSubsystem::Receiving(void) const
{
  PLATFORM::CLockObject lock(m_mutexReceiver);
  return !m_receivers.empty();
}

bool cDeviceReceiverSubsystem::AttachReceiver(cReceiver *receiver)
{
  assert(receiver);

  if (receiver->DeviceAttached(Device()))
  {
    dsyslog("receiver %p is already attached to %p", receiver, this);
    return true;
  }

  PLATFORM::CLockObject lock(m_mutexReceiver);

  if (!receiver->AddToPIDSubsystem(PID()))
  {
    dsyslog("receiver %p cannot be added to the pid subsys", receiver);
    return false;
  }

  receiver->Activate(true);
  Device()->Lock();
  receiver->AttachDevice(Device());
  m_receivers.push_back(receiver);
  Device()->Unlock();

  if (CommonInterface()->m_camSlot)
  {
    CommonInterface()->m_camSlot->StartDecrypting();
    CommonInterface()->m_startScrambleDetection = time(NULL);
  }

  Device()->CreateThread();
  dsyslog("receiver %p attached to %p", receiver, this);
  return true;
}

void cDeviceReceiverSubsystem::Detach(cReceiver *receiver)
{
  assert(receiver);

  if (!receiver->DeviceAttached(Device()))
  {
    dsyslog("receiver %p is not attached to %p", receiver, this);
    return;
  }

  PLATFORM::CLockObject lock(m_mutexReceiver);
  for (std::list<cReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
  {
    if (*it == receiver)
    {
      Device()->Lock();
      receiver->DetachDevice();
      Device()->Unlock();
      receiver->Activate(false);
      receiver->RemoveFromPIDSubsystem(PID());
      m_receivers.erase(it);
      break;
    }
  }
  if (CommonInterface()->m_camSlot)
    CommonInterface()->m_camSlot->StartDecrypting();
  if (m_receivers.empty())
    Device()->StopThread(0);
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

}
