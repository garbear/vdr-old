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
      bHasReceivers = !m_resources.empty();
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
        PidResourceMap::iterator it = m_resources.find(pid);
        if (it != m_resources.end())
        {
          for (std::set<iReceiver*>::iterator receiverit = it->second->receivers.begin(); receiverit != it->second->receivers.end(); ++receiverit)
            (*receiverit)->Receive(packet);
        }
      }
    }
  }

  Deinitialise();

  return NULL;
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, uint16_t pid)
{
  return OpenResourceForReceiver(pid, STREAM_TYPE_UNDEFINED, receiver);
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, const ChannelPtr& channel)
{
  bool bAllOpened(true);

  if (channel->GetVideoStream().vpid)
    bAllOpened &= OpenResourceForReceiver(channel->GetVideoStream().vpid, channel->GetVideoStream().vtype, receiver);

  if (channel->GetVideoStream().ppid != channel->GetVideoStream().vpid)
    bAllOpened &= OpenResourceForReceiver(channel->GetVideoStream().ppid, STREAM_TYPE_UNDEFINED, receiver);

  for (vector<AudioStream>::const_iterator it = channel->GetAudioStreams().begin(); it != channel->GetAudioStreams().end(); ++it)
    bAllOpened &= OpenResourceForReceiver(it->apid, it->atype, receiver);

  for (vector<DataStream>::const_iterator it = channel->GetDataStreams().begin(); it != channel->GetDataStreams().end(); ++it)
    bAllOpened &= OpenResourceForReceiver(it->dpid, it->dtype, receiver);

  for (vector<SubtitleStream>::const_iterator it = channel->GetSubtitleStreams().begin(); it != channel->GetSubtitleStreams().end(); ++it)
    bAllOpened &= OpenResourceForReceiver(it->spid, STREAM_TYPE_UNDEFINED, receiver);

  if (channel->GetTeletextStream().tpid)
    bAllOpened &= OpenResourceForReceiver(channel->GetTeletextStream().tpid, STREAM_TYPE_UNDEFINED, receiver);

  // TODO: Or should we bail if any stream fails to open? (this was VDR's behavior)

  if (CommonInterface()->m_camSlot)
  {
    CommonInterface()->m_camSlot->StartDecrypting();
    CommonInterface()->m_startScrambleDetection = time(NULL);
  }

  return bAllOpened;
}

void cDeviceReceiverSubsystem::DetachReceiverPid(iReceiver* receiver, uint16_t pid)
{
  CloseResourceForReceiver(pid, receiver);
}

void cDeviceReceiverSubsystem::DetachReceiver(iReceiver* receiver)
{
  CloseResourceForReceiver(receiver);

  //TODO
  if (CommonInterface()->m_camSlot)
    CommonInterface()->m_camSlot->StopDecrypting();
}

void cDeviceReceiverSubsystem::DetachAllReceivers(void)
{
  CLockObject lock(m_mutexReceiver);
  while (!m_resources.empty())
    DetachReceiver(*m_resources.begin()->second->receivers.begin());
}

bool cDeviceReceiverSubsystem::Receiving(void) const
{
  CLockObject lock(m_mutexReceiver);
  return !m_resources.empty();
}

bool cDeviceReceiverSubsystem::OpenResourceForReceiver(uint16_t pid, STREAM_TYPE streamType, iReceiver* receiver)
{
  PLATFORM::CLockObject lock(m_mutexReceiver);
  PidReceivers* receivers = GetReceivers(pid, streamType);
  if (receivers)
  {
    receivers->receivers.insert(receiver);
    return true;
  }
  return false;
}

void cDeviceReceiverSubsystem::CloseResourceForReceiver(uint16_t pid, iReceiver* receiver)
{
  PLATFORM::CLockObject lock(m_mutexReceiver);
  PidResourceMap::iterator it = m_resources.find(pid);
  if (it != m_resources.end())
  {
    PidReceivers* receivers = it->second;
    std::set<iReceiver*>::iterator receiverit = receivers->receivers.find(receiver);
    if (receiverit != receivers->receivers.end())
      receivers->receivers.erase(receiverit);
    if (receivers->receivers.empty())
    {
      delete it->second;
      m_resources.erase(it);
    }
  }
}

void cDeviceReceiverSubsystem::CloseResourceForReceiver(iReceiver* receiver)
{
  PLATFORM::CLockObject lock(m_mutexReceiver);
  for (PidResourceMap::iterator it = m_resources.begin(); it != m_resources.end(); ++it)
  {
    PidReceivers* receivers = it->second;
    std::set<iReceiver*>::iterator receiverit = receivers->receivers.find(receiver);
    if (receiverit != receivers->receivers.end())
      receivers->receivers.erase(receiverit);
    if (receivers->receivers.empty())
    {
      delete it->second;
      m_resources.erase(it);
    }
  }
}

struct PidReceivers* cDeviceReceiverSubsystem::GetReceivers(uint16_t pid, STREAM_TYPE streamType)
{
  PidReceivers* receivers;
  PLATFORM::CLockObject lock(m_mutexReceiver);
  PidResourceMap::iterator it = m_resources.find(pid);
  if (it == m_resources.end())
  {
    receivers = new PidReceivers;
    if (!receivers)
      return NULL;
    receivers->resource = CreateResource(pid, streamType);
    if (!receivers->resource->Open())
    {
      delete receivers;
      receivers = NULL;
    }
    else
    {
      m_resources[pid] = receivers;
    }
  }
  else
  {
    receivers = it->second;
  }

  return receivers;
}

}
