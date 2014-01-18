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

cReceiver::cReceiver(const cChannel *Channel, int Priority)
{
  m_device = NULL;
  m_priority = constrain(Priority, MINPRIORITY, MAXPRIORITY);
  m_numPids = 0;
  SetPids(Channel);
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
  if (Pid && !WantsPid(Pid))
  {
    if (m_numPids < MAXRECEIVEPIDS)
    {
      m_pids[m_numPids++] = Pid;
      esyslog("adding PID %d to receiver '%s' (%p)", Pid, m_device ? m_device->DeviceName().c_str() : "<nil>", this);
    }
    else
    {
      esyslog("too many PIDs in cReceiver (Pid = %d)", Pid);
      return false;
    }
  }
  return true;
}

bool cReceiver::AddPids(const int *Pids)
{
  if (Pids)
  {
    while (*Pids)
    {
      if (!AddPid(*Pids++))
        return false;
    }
  }
  return true;
}

bool cReceiver::AddPids(int Pid1, int Pid2, int Pid3, int Pid4, int Pid5, int Pid6, int Pid7, int Pid8, int Pid9)
{
  return AddPid(Pid1) && AddPid(Pid2) && AddPid(Pid3) && AddPid(Pid4) && AddPid(Pid5) && AddPid(Pid6) && AddPid(Pid7) && AddPid(Pid8) && AddPid(Pid9);
}

bool cReceiver::SetPids(const cChannel *Channel)
{
  m_numPids = 0;
  dsyslog("reset PIDs for channel '%s'", Channel ? Channel->Name().c_str() : "<nil>");
  if (Channel)
  {
    channelID = Channel->GetChannelID();
    return AddPid(Channel->Vpid())
        && (Channel->Ppid() == Channel->Vpid() || AddPid(Channel->Ppid()))
        && AddPids(Channel->Apids()) && AddPids(Channel->Dpids())
        && AddPids(Channel->Spids());
  }
  return true;
}

bool cReceiver::WantsPid(int Pid)
{
  if (Pid)
  {
    for (int i = 0; i < m_numPids; i++)
    {
      if (m_pids[i] == Pid)
        return true;
    }
  }
  return false;
}

void cReceiver::Detach(void)
{
  if (m_device)
  {
    dsyslog("detaching receiver");
    m_device->Receiver()->Detach(this);
  }
}

bool cReceiver::DeviceAttached(cDevice* device) const
{
  return m_device == device;
}

void cReceiver::AttachDevice(cDevice* device)
{
  dsyslog("attaching device '%s' to receiver '%p'", device ? device->DeviceName().c_str() : "<nil>", this);
  m_device = device;
}

void cReceiver::DetachDevice(void)
{
  m_device = NULL;
}

bool cReceiver::AddToPIDSubsystem(cDevicePIDSubsystem* pidSys)
{
  assert(pidSys);

  for (size_t n = 0; n < m_numPids; n++)
  {
    if (!pidSys->AddPid(m_pids[n]))
    {
      for ( ; n-- > 0; )
        pidSys->DelPid(m_pids[n]);
      return false;
    }
  }

  return true;
}

void cReceiver::RemoveFromPIDSubsystem(cDevicePIDSubsystem* pidSys)
{
  assert(pidSys);

  for (int n = 0; n < m_numPids; n++)
    pidSys->DelPid(m_pids[n]);
}
