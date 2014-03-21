/*
 * receiver.c: The basic receiver interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: receiver.c 2.7 2012/06/02 13:20:38 kls Exp $
 */

#include "Device.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DevicePIDSubsystem.h"
#include "Receiver.h"
#include <stdio.h>
#include "utils/Tools.h"

using namespace std;
using namespace PLATFORM;

namespace VDR
{

cReceiver::cReceiver(ChannelPtr Channel /* = cChannel::EmptyChannel */, int Priority)
{
  m_device = NULL;
  m_priority = constrain(Priority, MINPRIORITY, MAXPRIORITY);
  if (Channel)
    SetPids(*Channel);
}

cReceiver::~cReceiver()
{
  if (m_device)
  {
    const char *msg =
        "ERROR: cReceiver has not been detached yet! This is a design fault and VDR will segfault now!";
    esyslog("%s", msg);
    fprintf(stderr, "%s\n", msg);
    *(char *) 0 = 0; // XXX dafuq... cause a segfault
  }
}

bool cReceiver::AddPid(int Pid)
{
  CLockObject lock(m_mutex);

  set<int>::const_iterator it = m_pids.find(Pid);
  if (it == m_pids.end())
  {
    dsyslog("adding PID %d", Pid);
    m_pids.insert(Pid);
  }

  return true;
}

bool cReceiver::AddPids(set<int> pids)
{
  CLockObject lock(m_mutex);

  for (set<int>::const_iterator it = pids.begin(); it != pids.end(); ++it)
    if (!AddPid(*it))
      return false;
  return true;
}

bool cReceiver::UpdatePids(set<int> pids)
{
  CLockObject lock(m_mutex);

  for (set<int>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
  {
    if (pids.find(*it) == pids.end())
    {
      dsyslog("removing PID %d", *it);
      m_pids.erase(it);
      it = m_pids.begin();
    }
  }

  return AddPids(pids);
}

bool cReceiver::AddPids(const int *Pids)
{
  if (Pids)
  {
    CLockObject lock(m_mutex);

    while (*Pids)
    {
      if (!AddPid(*Pids++))
        return false;
    }
  }
  return true;
}

tChannelID cReceiver::ChannelID(void) const
{
  CLockObject lock(m_mutex);

  return m_channelID;
}

inline void AppendToSet(set<int>& newPids, const int* pids)
{
  while (*pids)
    newPids.insert(*pids++);
}

bool cReceiver::SetPids(const cChannel& Channel)
{
  set<int> newPids;
  newPids.insert(Channel.Vpid());
  if (Channel.Ppid() != Channel.Vpid())
    newPids.insert(Channel.Ppid());
  AppendToSet(newPids, Channel.Apids());
  AppendToSet(newPids, Channel.Dpids());
  AppendToSet(newPids, Channel.Spids());
  if (Channel.Tpid())
    newPids.insert(Channel.Tpid());

  CLockObject lock(m_mutex);

  if (m_channelID != Channel.GetChannelID())
  {
    dsyslog("set PIDs for channel '%s'", Channel.Name().c_str());
    m_channelID = Channel.GetChannelID();
    m_pids.clear();
    return AddPids(newPids);
  }
  else
  {
    dsyslog("updating PIDs for channel '%s'", Channel.Name().c_str());
    return UpdatePids(newPids);
  }

  return true;
}

bool cReceiver::WantsPid(int Pid)
{
  set<int>::const_iterator it = m_pids.find(Pid);
  return it != m_pids.end();
}

void cReceiver::Detach(void)
{
  CLockObject lock(m_mutex);

  if (m_device)
  {
    dsyslog("detaching receiver");
    m_device->Receiver()->Detach(this);
  }
}

bool cReceiver::DeviceAttached(cDevice* device) const
{
  CLockObject lock(m_mutex);

  return m_device == device;
}

bool cReceiver::IsAttached(void) const
{
  CLockObject lock(m_mutex);

  return m_device != NULL;
}

void cReceiver::AttachDevice(cDevice* device)
{
  CLockObject lock(m_mutex);

  dsyslog("attaching device '%s' to receiver '%p'", device ? device->DeviceName().c_str() : "<nil>", this);
  m_device = device;
}

void cReceiver::DetachDevice(void)
{
  CLockObject lock(m_mutex);

  dsyslog("detaching device '%s' from receiver '%p'", m_device ? m_device->DeviceName().c_str() : "<nil>", this);
  m_device = NULL;
}

bool cReceiver::AddToPIDSubsystem(cDevicePIDSubsystem* pidSys) const
{
  assert(pidSys);

  CLockObject lock(m_mutex);

  for (set<int>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
  {
    if (!pidSys->AddPid(*it))
    {
      for (set<int>::const_iterator it2 = m_pids.begin(); *it2 != *it; ++it2)
        pidSys->DelPid(*it2);
      return false;
    }
  }

  return true;
}

void cReceiver::RemoveFromPIDSubsystem(cDevicePIDSubsystem* pidSys) const
{
  assert(pidSys);

  CLockObject lock(m_mutex);

  for (set<int>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
    pidSys->DelPid(*it);
}

}
