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
    if (!SyncResources())
      continue;

    if (Poll() && !IsStopped())
    {
      vector<uint8_t> packet;
      if (Read(packet))
      {
        CLockObject lock(m_mutexReceiverRead);
        if (!SyncResources())
          continue;

        uint16_t pid = TsPid(packet.data());
        PidResourceMap::iterator it = m_resourcesActive.find(pid);
        if (it != m_resourcesActive.end())
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

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type /* = STREAM_TYPE_UNDEFINED */)
{
  addedReceiver newcrv;
  newcrv.receiver = receiver;
  newcrv.type     = type;
  newcrv.pid      = pid;
  CLockObject lock(m_mutexReceiverRead);
  CLockObject lock2(m_mutexReceiverWrite);
  m_resourcesAdded.push_back(newcrv);
  return true;
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, const ChannelPtr& channel)
{
  bool bAllOpened(true);

  if (channel->GetVideoStream().vpid)
    bAllOpened &= AttachReceiver(receiver, channel->GetVideoStream().vpid, channel->GetVideoStream().vtype);

  if (channel->GetVideoStream().ppid != channel->GetVideoStream().vpid)
    bAllOpened &= AttachReceiver(receiver, channel->GetVideoStream().ppid);

  for (vector<AudioStream>::const_iterator it = channel->GetAudioStreams().begin(); it != channel->GetAudioStreams().end(); ++it)
    bAllOpened &= AttachReceiver(receiver, it->apid, it->atype);

  for (vector<DataStream>::const_iterator it = channel->GetDataStreams().begin(); it != channel->GetDataStreams().end(); ++it)
    bAllOpened &= AttachReceiver(receiver, it->dpid, it->dtype);

  for (vector<SubtitleStream>::const_iterator it = channel->GetSubtitleStreams().begin(); it != channel->GetSubtitleStreams().end(); ++it)
    bAllOpened &= AttachReceiver(receiver, it->spid);

  if (channel->GetTeletextStream().tpid)
    bAllOpened &= AttachReceiver(receiver, channel->GetTeletextStream().tpid);

  // TODO: Or should we bail if any stream fails to open? (this was VDR's behavior)

  if (CommonInterface()->m_camSlot)
  {
    CommonInterface()->m_camSlot->StartDecrypting();
    CommonInterface()->m_startScrambleDetection = time(NULL);
  }

  return bAllOpened;
}

bool cDeviceReceiverSubsystem::HasReceiver(iReceiver* receiver) const
{
  PLATFORM::CLockObject lock(m_mutexReceiverRead);
  for (PidResourceMap::const_iterator it = m_resourcesActive.begin(); it != m_resourcesActive.end(); ++it)
    if (it->second->receivers.find(receiver) != it->second->receivers.end())
      return true;
  return false;
}

bool cDeviceReceiverSubsystem::HasReceiverPid(iReceiver* receiver, uint16_t pid) const
{
  PLATFORM::CLockObject lock(m_mutexReceiverRead);
  PidResourceMap::const_iterator it = m_resourcesActive.find(pid);
  return (it == m_resourcesActive.end()) ? false : it->second->receivers.find(receiver) != it->second->receivers.end();
}

void cDeviceReceiverSubsystem::DetachReceiverPid(iReceiver* receiver, uint16_t pid)
{
  PLATFORM::CLockObject lock(m_mutexReceiverRead);
  PLATFORM::CLockObject lock2(m_mutexReceiverWrite);
  if (HasReceiverPid(receiver, pid))
    m_resourcesRemoved[pid] = receiver;
}

void cDeviceReceiverSubsystem::DetachReceiver(iReceiver* receiver)
{
  PLATFORM::CLockObject lock(m_mutexReceiverRead);
  PLATFORM::CLockObject lock2(m_mutexReceiverWrite);
  if (HasReceiver(receiver))
    m_receiversRemoved.insert(receiver);
}

void cDeviceReceiverSubsystem::DetachAllReceivers(void)
{
  CLockObject lock(m_mutexReceiverRead);
  if (!m_resourcesActive.empty())
  {
    while (SyncResources())
      DetachReceiver(*m_resourcesActive.begin()->second->receivers.begin());
  }
}

bool cDeviceReceiverSubsystem::Receiving(void) const
{
  CLockObject lock(m_mutexReceiverRead);
  return !m_resourcesActive.empty();
}

bool cDeviceReceiverSubsystem::OpenResourceForReceiver(uint16_t pid, STREAM_TYPE streamType, iReceiver* receiver)
{
  PLATFORM::CLockObject lock(m_mutexReceiverRead);
  PidReceivers* receivers = GetReceivers(pid, streamType);
  if (receivers)
  {
    receivers->receivers.insert(receiver);
    return true;
  }
  return false;
}

bool cDeviceReceiverSubsystem::SyncResources(void)
{
  PLATFORM::CLockObject lock(m_mutexReceiverRead);
  PLATFORM::CLockObject lock2(m_mutexReceiverWrite);

  for (std::map<uint16_t, iReceiver*>::iterator it = m_resourcesRemoved.begin(); it != m_resourcesRemoved.end(); ++it)
    CloseResourceForReceiver(it->first, it->second);
  m_resourcesRemoved.clear();

  for (std::set<iReceiver*>::iterator it = m_receiversRemoved.begin(); it != m_receiversRemoved.end(); ++it)
    CloseResourceForReceiver(*it);
  m_receiversRemoved.clear();

  for (std::vector<addedReceiver>::iterator it = m_resourcesAdded.begin(); it != m_resourcesAdded.end(); ++it)
    OpenResourceForReceiver((*it).pid, (*it).type, (*it).receiver);
  m_resourcesAdded.clear();

  if (m_resourcesActive.empty())
  {
    if (CommonInterface()->m_camSlot)
      CommonInterface()->m_camSlot->StopDecrypting();
    usleep(MAX_IDLE_DELAY_MS * 1000);
    return false;
  }
  return true;
}

void cDeviceReceiverSubsystem::CloseResourceForReceiver(uint16_t pid, iReceiver* receiver)
{
  PidResourceMap::iterator it = m_resourcesActive.find(pid);
  if (it != m_resourcesActive.end())
  {
    PidReceivers* receivers = it->second;
    std::set<iReceiver*>::iterator receiverit = receivers->receivers.find(receiver);
    if (receiverit != receivers->receivers.end())
      receivers->receivers.erase(receiverit);
    if (receivers->receivers.empty())
    {
      delete it->second;
      m_resourcesActive.erase(it);
    }
  }
}

void cDeviceReceiverSubsystem::CloseResourceForReceiver(iReceiver* receiver)
{
  for (PidResourceMap::iterator it = m_resourcesActive.begin(); it != m_resourcesActive.end(); ++it)
  {
    PidReceivers* receivers = it->second;
    std::set<iReceiver*>::iterator receiverit = receivers->receivers.find(receiver);
    if (receiverit != receivers->receivers.end())
      receivers->receivers.erase(receiverit);
    if (receivers->receivers.empty())
    {
      delete it->second;
      m_resourcesActive.erase(it);
    }
  }
}

struct PidReceivers* cDeviceReceiverSubsystem::GetReceivers(uint16_t pid, STREAM_TYPE streamType)
{
  PidReceivers* receivers;
  PLATFORM::CLockObject lock(m_mutexReceiverRead);
  PLATFORM::CLockObject lock2(m_mutexReceiverWrite);
  PidResourceMap::iterator it = m_resourcesActive.find(pid);
  if (it == m_resourcesActive.end())
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
      m_resourcesActive[pid] = receivers;
    }
  }
  else
  {
    receivers = it->second;
  }

  return receivers;
}

}
