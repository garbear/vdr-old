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

#include "Receiver.h"
#include "Device.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DevicePIDSubsystem.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"

#include <stdio.h>

using namespace std;
using namespace PLATFORM;

namespace VDR
{

cReceiver::cReceiver(const ChannelPtr& Channel /* = cChannel::EmptyChannel */)
{
  m_device = NULL;
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

void cReceiver::AddPid(uint16_t pid)
{
  CLockObject lock(m_mutex);

  set<uint16_t>::const_iterator it = m_pids.find(pid);
  if (it == m_pids.end())
  {
    dsyslog("adding PID %u", pid);
    m_pids.insert(pid);
  }
}

void cReceiver::AddPids(const set<uint16_t>& pids)
{
  CLockObject lock(m_mutex);

  for (set<uint16_t>::const_iterator it = pids.begin(); it != pids.end(); ++it)
    AddPid(*it);
}

void cReceiver::UpdatePids(const set<uint16_t>& pids)
{
  CLockObject lock(m_mutex);

  for (set<uint16_t>::const_iterator it = pids.begin(); it != pids.end(); ++it)
  {
    if (m_pids.find(*it) == m_pids.end())
      dsyslog("adding PID %u", *it);
  }

  for (set<uint16_t>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
  {
    if (pids.find(*it) == pids.end())
      dsyslog("removing PID %u", *it);
  }

  m_pids = pids;
}

cChannelID cReceiver::ChannelID(void) const
{
  CLockObject lock(m_mutex);

  return m_channelID;
}

void cReceiver::SetPids(const cChannel& Channel)
{
  set<uint16_t> newPids;
  newPids.insert(Channel.GetVideoStream().vpid);
  if (Channel.GetVideoStream().ppid != Channel.GetVideoStream().vpid)
    newPids.insert(Channel.GetVideoStream().ppid);
  for (vector<AudioStream>::const_iterator it = Channel.GetAudioStreams().begin(); it != Channel.GetAudioStreams().end(); ++it)
    newPids.insert(it->apid);
  for (vector<DataStream>::const_iterator it = Channel.GetDataStreams().begin(); it != Channel.GetDataStreams().end(); ++it)
    newPids.insert(it->dpid);
  for (vector<SubtitleStream>::const_iterator it = Channel.GetSubtitleStreams().begin(); it != Channel.GetSubtitleStreams().end(); ++it)
    newPids.insert(it->spid);
  if (Channel.GetTeletextStream().tpid)
    newPids.insert(Channel.GetTeletextStream().tpid);

  CLockObject lock(m_mutex);

  if (m_channelID != Channel.GetChannelID())
  {
    dsyslog("set PIDs for channel '%s'", Channel.Name().c_str());
    m_channelID = Channel.GetChannelID();
    m_pids.clear();
    AddPids(newPids);
  }
  else
  {
    dsyslog("updating PIDs for channel '%s'", Channel.Name().c_str());
    UpdatePids(newPids);
  }
}

bool cReceiver::WantsPid(int Pid)
{
  set<uint16_t>::const_iterator it = m_pids.find(Pid);
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

  for (set<uint16_t>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
  {
    if (!pidSys->AddPid(*it))
    {
      for (set<uint16_t>::const_iterator it2 = m_pids.begin(); *it2 != *it; ++it2)
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

  for (set<uint16_t>::const_iterator it = m_pids.begin(); it != m_pids.end(); ++it)
    pidSys->DelPid(*it);
}

}
