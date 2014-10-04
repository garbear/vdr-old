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
#include <unistd.h> // for usleep()

using namespace PLATFORM;
using namespace std;

#define TS_SCRAMBLING_TIMEOUT     3 // seconds to wait until a TS becomes unscrambled
#define TS_SCRAMBLING_TIME_OK    10 // seconds before a Channel/CAM combination is marked as known to decrypt

#define MAX_IDLE_DELAY_MS      100

namespace VDR
{

cDeviceReceiverSubsystem::cDeviceReceiverSubsystem(cDevice *device)
 : cDeviceSubsystem(device)
{
}

cDeviceReceiverSubsystem::~cDeviceReceiverSubsystem(void)
{
  DetachAllReceivers();
}

void cDeviceReceiverSubsystem::Start(void)
{
  if (!IsRunning())
    CreateThread(true);
}

void cDeviceReceiverSubsystem::Stop(void)
{
  StopThread(0);
}

void *cDeviceReceiverSubsystem::Process()
{
  if (!Initialise())
    return NULL;

  while (!IsStopped())
  {
    bool bHasReceivers;
    {
      CLockObject lock(m_mutexReceiver);
      bHasReceivers = !m_receiverResources.empty();
    }

    if (!bHasReceivers)
    {
      usleep(MAX_IDLE_DELAY_MS * 1000);
      continue;
    }

    if (Poll() && !IsStopped())
    {
      vector<uint8_t> packet;
      if (Read(packet))
      {
        uint16_t pid = TsPid(packet.data());

        CLockObject lock(m_mutexReceiver);

        // Distribute the packet to all attached receivers:
        for (ReceiverResourceMap::iterator itPair = m_receiverResources.begin(); itPair != m_receiverResources.end(); ++itPair)
        {
          iReceiver* receiver = itPair->first;
          const PidResourceSet& pids = itPair->second;

          for (PidResourceSet::const_iterator itPid = pids.begin(); itPid != pids.end(); ++itPid)
          {
            if (pid == (*itPid)->Pid())
            {
              //(*itPid)->Receive(packet)
              receiver->Receive(packet);
            }
          }
        }
      }
    }
  }

  Deinitialise();

  return NULL;
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, PidResourceSet pids)
{
  bool retval = true;
  CLockObject lock(m_mutexReceiver);

  if (m_receiverResources.find(receiver) != m_receiverResources.end())
  {
    bool haspid;
    bool added = false;
    for (std::set<PidResourcePtr>::iterator it = pids.begin(); it != pids.end(); ++it)
    {
      haspid = false;
      for (std::set<PidResourcePtr>::iterator it2 = m_receiverResources[receiver].begin(); it2 != m_receiverResources[receiver].end(); ++it2)
      {
        if ((*it2)->Pid() == (*it)->Pid())
        {
          haspid = true;
          break;
        }
      }

      if (!haspid)
      {
        PidResourcePtr res = OpenResourceInternal((*it)->Pid(), STREAM_TYPE_UNDEFINED);
        if (res)
        {
          added = true;
          m_receiverResources[receiver].insert(res);
        }
        else
        {
          retval = false;
        }
      }
    }

    if (!added)
    {
      dsyslog("Receiver already attached, skipping");
      return false;
    }
    return retval;
  }

  m_receiverResources[receiver] = pids;
  receiver->Start();

  dsyslog("receiver %p attached to %p", receiver, this);

  return true;
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, uint16_t pid)
{
  PidResourceSet pidHandles;
  CLockObject lock(m_mutexReceiver);
  PidResourcePtr res = OpenResourceInternal(pid, STREAM_TYPE_UNDEFINED);
  if (!res)
    return false;

  pidHandles.insert(res);
  return AttachReceiver(receiver, pidHandles);
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, const ChannelPtr& channel)
{
  CLockObject lock(m_mutexReceiver);
  // Receivers are mapped receiver -> pids
  // Generate list of pids to attach to this filter
  PidResourceSet pidHandles;
  if (!OpenResources(channel, pidHandles))
    return false;

  if (!AttachReceiver(receiver, pidHandles))
    return false;

  if (CommonInterface()->m_camSlot)
  {
    CommonInterface()->m_camSlot->StartDecrypting();
    CommonInterface()->m_startScrambleDetection = time(NULL);
  }

  return true;
}

void cDeviceReceiverSubsystem::DetachReceiverPid(iReceiver* receiver, uint16_t pid)
{
  CLockObject lock(m_mutexReceiver);

  ReceiverResourceMap::iterator it = m_receiverResources.find(receiver);
  if (it == m_receiverResources.end())
  {
    dsyslog("Receiver is not attached?");
    return;
  }

  for (std::set<PidResourcePtr>::iterator it2 = m_receiverResources[receiver].begin(); it2 != m_receiverResources[receiver].end(); ++it2)
  {
    if ((*it2)->Pid() == pid)
    {
      m_receiverResources[receiver].erase(it2);
      break;
    }
  }

  if (m_receiverResources[receiver].empty())
  {
    receiver->Stop();
    m_receiverResources.erase(it);

    if (CommonInterface()->m_camSlot)
      CommonInterface()->m_camSlot->StartDecrypting();
    dsyslog("receiver %p detached from %p", receiver, this);
  }
  else
  {
    dsyslog("PID %u detached from receiver %p attached to %p", pid, receiver, this);
  }
}

void cDeviceReceiverSubsystem::DetachReceiver(iReceiver* receiver)
{
  CLockObject lock(m_mutexReceiver);

  ReceiverResourceMap::iterator it = m_receiverResources.find(receiver);
  if (it == m_receiverResources.end())
  {
    dsyslog("Receiver is not attached?");
    return;
  }

  receiver->Stop();

  m_receiverResources.erase(it);

  if (CommonInterface()->m_camSlot)
    CommonInterface()->m_camSlot->StopDecrypting();

  dsyslog("receiver %p detached from %p", receiver, this);
}

void cDeviceReceiverSubsystem::DetachAllReceivers(void)
{
  CLockObject lock(m_mutexReceiver);
  while (!m_receiverResources.empty())
    DetachReceiver(m_receiverResources.begin()->first);
}

bool cDeviceReceiverSubsystem::Receiving(void) const
{
  CLockObject lock(m_mutexReceiver);
  return !m_receiverResources.empty();
}

PidResourcePtr cDeviceReceiverSubsystem::GetOpenResource(const PidResourcePtr& needle)
{
  CLockObject lock(m_mutexReceiver);

  for (ReceiverResourceMap::const_iterator itPair = m_receiverResources.begin(); itPair != m_receiverResources.end(); ++itPair)
  {
    const PidResourceSet& haystack = itPair->second;
    for (PidResourceSet::const_iterator itPidHandle = haystack.begin(); itPidHandle != haystack.end(); ++itPidHandle)
    {
      if ((*itPidHandle)->Equals(needle.get()))
        return *itPidHandle;
    }
  }

  return PidResourcePtr();
}

bool cDeviceReceiverSubsystem::OpenResources(const ChannelPtr& channel, PidResourceSet& openResources)
{
  PidResourcePtr resource;

  if (channel->GetVideoStream().vpid)
  {
    if ((resource = OpenResourceInternal(channel->GetVideoStream().vpid, channel->GetVideoStream().vtype)))
      openResources.insert(resource);
  }

  if (channel->GetVideoStream().ppid != channel->GetVideoStream().vpid)
  {
    if ((resource = OpenResourceInternal(channel->GetVideoStream().ppid, STREAM_TYPE_UNDEFINED)))
      openResources.insert(resource);
  }

  for (vector<AudioStream>::const_iterator it = channel->GetAudioStreams().begin(); it != channel->GetAudioStreams().end(); ++it)
  {
    if ((resource = OpenResourceInternal(it->apid, it->atype)))
      openResources.insert(resource);
  }

  for (vector<DataStream>::const_iterator it = channel->GetDataStreams().begin(); it != channel->GetDataStreams().end(); ++it)
  {
    if ((resource = OpenResourceInternal(it->dpid, it->dtype)))
      openResources.insert(resource);
  }

  for (vector<SubtitleStream>::const_iterator it = channel->GetSubtitleStreams().begin(); it != channel->GetSubtitleStreams().end(); ++it)
  {
    if ((resource = OpenResourceInternal(it->spid, STREAM_TYPE_UNDEFINED)))
      openResources.insert(resource);
  }

  if (channel->GetTeletextStream().tpid)
  {
    if ((resource = OpenResourceInternal(channel->GetTeletextStream().tpid, STREAM_TYPE_UNDEFINED)))
      openResources.insert(resource);
  }

  // TODO: Or should we bail if any stream fails to open? (this was VDR's behavior)
  return !openResources.empty();
}

PidResourcePtr cDeviceReceiverSubsystem::OpenResourceInternal(uint16_t pid, STREAM_TYPE streamType)
{
  PidResourcePtr newResource = CreateResource(pid, streamType);
  PidResourcePtr existingResource = GetOpenResource(newResource);

  if (existingResource)
    return existingResource;

  if (newResource && newResource->Open())
    return newResource;

  return PidResourcePtr();
}

}
