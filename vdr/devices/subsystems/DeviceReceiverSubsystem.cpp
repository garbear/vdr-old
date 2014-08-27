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
#include "utils/Ringbuffer.h"

#include <algorithm>
#include <assert.h>

using namespace PLATFORM;
using namespace std;

#define TS_SCRAMBLING_TIMEOUT     3 // seconds to wait until a TS becomes unscrambled
#define TS_SCRAMBLING_TIME_OK    10 // seconds before a Channel/CAM combination is marked as known to decrypt

#define STREAM_TYPE_UNKNOWN  0x00 // TODO: Find VDR code that determines stream type for these cases

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

void *cDeviceReceiverSubsystem::Process()
{
  if (!OpenDvr())
    return NULL;

  cRingBufferLinear ringBuffer(MEGABYTE(5), TS_SIZE, true, "TS");

  while (!IsStopped())
  {
    std::vector<uint8_t> packet;
    if (!GetTSPacket(ringBuffer, packet))
      continue;

    uint16_t pid = TsPid(packet.data());

    // Distribute the packet to all attached receivers:
    CLockObject lock(m_mutexReceiver);
    for (ReceiverResourceMap::iterator itPair = m_receiverResources.begin(); itPair != m_receiverResources.end(); ++itPair)
    {
      iReceiver* receiver = itPair->first;
      const PidResourceSet& pids = itPair->second;

      for (PidResourceSet::const_iterator itPid = pids.begin(); itPid != pids.end(); ++itPid)
      {
        if (pid == (*itPid)->Pid())
        {
          receiver->Receive(packet);
          break;
        }
      }
    }
  }

  CloseDvr();

  return NULL;
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, const ChannelPtr& channel)
{
  CLockObject lock(m_mutexReceiver);

  if (m_receiverResources.find(receiver) != m_receiverResources.end())
  {
    dsyslog("Receiver already attached, skipping");
    return false;
  }

  // Receivers are mapped receiver -> pids
  // Generate list of pids to attach to this filter
  PidResourceSet pidHandles;
  if (!OpenResources(channel, pidHandles))
    return false;
  m_receiverResources[receiver] = pidHandles;

  receiver->Start();

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

  ReceiverResourceMap::iterator it = m_receiverResources.find(receiver);
  if (it == m_receiverResources.end())
  {
    dsyslog("Receiver is not attached?");
    return;
  }

  receiver->Stop();

  m_receiverResources.erase(it);

  if (CommonInterface()->m_camSlot)
    CommonInterface()->m_camSlot->StartDecrypting();

  if (m_receiverResources.empty())
    StopThread(0);

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

bool cDeviceReceiverSubsystem::HasPid(uint16_t pid) const
{
  CLockObject lock(m_mutexReceiver);

  for (ReceiverResourceMap::const_iterator itPair = m_receiverResources.begin(); itPair != m_receiverResources.end(); ++itPair)
  {
    const PidResourceSet& haystack = itPair->second;
    for (PidResourceSet::const_iterator itPidHandle = haystack.begin(); itPidHandle != haystack.end(); ++itPidHandle)
    {
      if (pid == (*itPidHandle)->Pid())
        return true;
    }
  }

  return false;
}

PidResourcePtr cDeviceReceiverSubsystem::GetOpenResource(uint16_t pid)
{
  CLockObject lock(m_mutexReceiver);

  for (ReceiverResourceMap::const_iterator itPair = m_receiverResources.begin(); itPair != m_receiverResources.end(); ++itPair)
  {
    const PidResourceSet& haystack = itPair->second;
    for (PidResourceSet::const_iterator itPidHandle = haystack.begin(); itPidHandle != haystack.end(); ++itPidHandle)
    {
      if (pid == (*itPidHandle)->Pid())
        return *itPidHandle;
    }
  }

  return PidResourcePtr();
}

bool cDeviceReceiverSubsystem::OpenResources(const ChannelPtr& channel, PidResourceSet& openResources)
{
  if (channel->GetVideoStream().vpid)
    OpenResourceInternal(channel->GetVideoStream().vpid, channel->GetVideoStream().vtype, openResources);

  if (channel->GetVideoStream().ppid != channel->GetVideoStream().vpid)
    OpenResourceInternal(channel->GetVideoStream().ppid, STREAM_TYPE_UNKNOWN, openResources);

  for (vector<AudioStream>::const_iterator it = channel->GetAudioStreams().begin(); it != channel->GetAudioStreams().end(); ++it)
    OpenResourceInternal(it->apid, it->atype, openResources);

  for (vector<DataStream>::const_iterator it = channel->GetDataStreams().begin(); it != channel->GetDataStreams().end(); ++it)
    OpenResourceInternal(it->dpid, it->dtype, openResources);

  for (vector<SubtitleStream>::const_iterator it = channel->GetSubtitleStreams().begin(); it != channel->GetSubtitleStreams().end(); ++it)
    OpenResourceInternal(it->spid, STREAM_TYPE_UNKNOWN, openResources);

  if (channel->GetTeletextStream().tpid)
    OpenResourceInternal(channel->GetTeletextStream().tpid, STREAM_TYPE_UNKNOWN, openResources);

  // TODO: Or should we bail if any stream fails to open? (this was VDR's behavior)
  return !openResources.empty();
}

bool cDeviceReceiverSubsystem::OpenResourceInternal(uint16_t pid, uint8_t streamType, PidResourceSet& pidHandles)
{
  // Try to get a resource that is already open
  PidResourcePtr pidHandle = GetOpenResource(pid);
  if (pidHandle)
  {
    pidHandles.insert(pidHandle);
    return true;
  }

  // Open a new resource and add it to the set
  pidHandle = OpenResource(pid, streamType);
  if (pidHandle)
  {
    pidHandles.insert(pidHandle);
    return true;
  }

  return false;
}

bool cDeviceReceiverSubsystem::GetTSPacket(cRingBufferLinear& ringBuffer, std::vector<uint8_t>& data)
{
  Read(ringBuffer);
  int count = 0;
  uint8_t* p = ringBuffer.Get(count);

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

      ringBuffer.Del(count);
      esyslog("ERROR: skipped %d bytes to sync on TS packet on device %d", count, Device()->Index());
      return false;
    }

    ringBuffer.Del(TS_SIZE);
    data.assign(p, p + TS_SIZE);
    assert(data.size() == TS_SIZE);
    return true;
  }
  return false;
}

}
