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
#include "dvb/PsiBuffer.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"

#include <algorithm>

using namespace PLATFORM;
using namespace std;

#define TS_SCRAMBLING_TIMEOUT     3 // seconds to wait until a TS becomes unscrambled
#define TS_SCRAMBLING_TIME_OK    10 // seconds before a Channel/CAM combination is marked as known to decrypt

#define MAX_IDLE_DELAY_MS      100

namespace VDR
{

// --- cDeviceReceiverSubsystem::cReceiverHandle -------------------------------

cDeviceReceiverSubsystem::cReceiverHandle::cReceiverHandle(iReceiver* rcvr)
 : receiver(rcvr)
{
}

bool cDeviceReceiverSubsystem::cReceiverHandle::Start(void)
{
  return receiver->Start();
}

cDeviceReceiverSubsystem::cReceiverHandle::~cReceiverHandle(void)
{
  receiver->Stop();
}

// --- cDeviceReceiverSubsystem ------------------------------------------------

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

  TsPacket packet;
  uint16_t pid;
  const uint8_t* psidata;
  size_t psidatalen;
  bool validpsi;
  PidResourcePtr resource;

  while (!IsStopped())
  {
    if (!Receiving())
    {
      CThread::Sleep(100);
      continue;
    }

    /** wait for new data */
    switch (Poll(resource))
    {
      case POLL_RESULT_STREAMING_READY:
      if (!IsStopped() && resource->Read(&psidata, &psidatalen))
      {
        CLockObject lock(m_mutex);

        /** distribute the packet to all receivers */
        std::set<iReceiver*> receivers = GetReceivers(resource);
        for (std::set<iReceiver*>::iterator receiverit = receivers.begin(); receiverit != receivers.end(); ++receiverit)
          (*receiverit)->Receive(resource->Pid(), psidata, psidatalen);
      }
      break;

      case POLL_RESULT_MULTIPLEXED_READY:
      if (!IsStopped() && (packet = ReadMultiplexed()) != NULL)
      {
        CLockObject lock(m_mutex);

        /** find resources attached to the PID that we received */
        pid = TsPid(packet);
        PidResourcePtr pidPtr = GetMultiplexedResource(pid);
        if (pidPtr)
        {
          validpsi = pidPtr->Buffer()->AddTsData(packet, TS_SIZE, &psidata, &psidatalen);

          /** distribute the packet to all receivers */
          std::set<iReceiver*> receivers = GetReceivers();
          for (std::set<iReceiver*>::iterator receiverit = receivers.begin(); receiverit != receivers.end(); ++receiverit)
          {
            if ((*receiverit)->IsPsiReceiver())
            {
              /** only send full data */
              if (validpsi)
                (*receiverit)->Receive(pid, psidata, psidatalen);
            }
            else
            {
              (*receiverit)->Receive(pid, packet, TS_SIZE);
            }
          }
        }
        Consumed();
      }
      break;

      case POLL_RESULT_NOT_READY:
      default:
        break;
    }
  }

  Deinitialise();

  return NULL;
}

std::set<cDeviceReceiverSubsystem::PidResourcePtr> cDeviceReceiverSubsystem::GetResources(void) const
{
  std::set<PidResourcePtr> resources;
  for (ReceiverPidTable::const_iterator it = m_receiverPidTable.begin(); it != m_receiverPidTable.end(); ++it)
    resources.insert(it->second);
  return resources;
}

cDeviceReceiverSubsystem::ReceiverHandlePtr cDeviceReceiverSubsystem::GetReceiverHandle(iReceiver* receiver) const
{
  for (ReceiverPidTable::const_iterator it = m_receiverPidTable.begin(); it != m_receiverPidTable.end(); ++it)
  {
    if (it->first->receiver == receiver)
      return it->first;
  }
  return ReceiverHandlePtr();
}

std::set<iReceiver*> cDeviceReceiverSubsystem::GetReceivers(void) const
{
  std::set<iReceiver*> receivers;
  for (ReceiverPidTable::const_iterator it = m_receiverPidTable.begin(); it != m_receiverPidTable.end(); ++it)
    receivers.insert(it->first->receiver);
  return receivers;
}

std::set<iReceiver*> cDeviceReceiverSubsystem::GetReceivers(const PidResourcePtr& resource) const
{
  std::set<iReceiver*> receivers;
  for (ReceiverPidTable::const_iterator it = m_receiverPidTable.begin(); it != m_receiverPidTable.end(); ++it)
    if (resource == it->second)
      receivers.insert(it->first->receiver);
  return receivers;
}

cDeviceReceiverSubsystem::PidResourcePtr cDeviceReceiverSubsystem::GetResource(const PidResourcePtr& needle) const
{
  for (ReceiverPidTable::const_iterator it = m_receiverPidTable.begin(); it != m_receiverPidTable.end(); ++it)
  {
    if (it->second->Equals(needle.get()))
      return it->second;
  }
  return PidResourcePtr();
}

cDeviceReceiverSubsystem::PidResourcePtr cDeviceReceiverSubsystem::GetMultiplexedResource(uint16_t pid) const
{
  for (ReceiverPidTable::const_iterator it = m_receiverPidTable.begin(); it != m_receiverPidTable.end(); ++it)
  {
    if (it->second->Equals(pid))
      return it->second;
  }
  return PidResourcePtr();
}

bool cDeviceReceiverSubsystem::AttachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask)
{
  CLockObject lock(m_mutex);

  ReceiverHandlePtr receiverHandle = GetReceiverHandle(receiver);
  if (!receiverHandle)
  {
    receiverHandle = ReceiverHandlePtr(new cReceiverHandle(receiver));
    if (!receiverHandle->Start())
      return false;
  }

  PidResourcePtr resource = CreateStreamingResource(pid, tid, mask);
  if (GetResource(resource) || !resource->Open())
    return false;

  m_receiverPidTable.insert(ReceiverPidEdge(receiverHandle, resource));

  return true;
}

bool cDeviceReceiverSubsystem::AttachMultiplexedReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type /* = STREAM_TYPE_UNDEFINED */)
{
  CLockObject lock(m_mutex);

  ReceiverHandlePtr receiverHandle = GetReceiverHandle(receiver);
  if (!receiverHandle)
  {
    receiverHandle = ReceiverHandlePtr(new cReceiverHandle(receiver));
    if (!receiverHandle->Start())
      return false;
  }

  PidResourcePtr resource = CreateMultiplexedResource(pid, type);
  if (GetResource(resource) || !resource->Open())
    return false;

  m_receiverPidTable.insert(ReceiverPidEdge(receiverHandle, resource));

  return true;
}

bool cDeviceReceiverSubsystem::AttachMultiplexedReceiver(iReceiver* receiver, const ChannelPtr& channel)
{
  CLockObject lock(m_mutex);

  bool bAllOpened(true);

  if (bAllOpened && channel->GetVideoStream().vpid)
    bAllOpened &= AttachMultiplexedReceiver(receiver, channel->GetVideoStream().vpid, channel->GetVideoStream().vtype);

  if (bAllOpened && channel->GetVideoStream().ppid != channel->GetVideoStream().vpid)
    bAllOpened &= AttachMultiplexedReceiver(receiver, channel->GetVideoStream().ppid);

  for (vector<AudioStream>::const_iterator it = channel->GetAudioStreams().begin(); bAllOpened && it != channel->GetAudioStreams().end(); ++it)
    bAllOpened &= AttachMultiplexedReceiver(receiver, it->apid, it->atype);

  for (vector<DataStream>::const_iterator it = channel->GetDataStreams().begin(); bAllOpened && it != channel->GetDataStreams().end(); ++it)
    bAllOpened &= AttachMultiplexedReceiver(receiver, it->dpid, it->dtype);

  for (vector<SubtitleStream>::const_iterator it = channel->GetSubtitleStreams().begin(); bAllOpened && it != channel->GetSubtitleStreams().end(); ++it)
    bAllOpened &= AttachMultiplexedReceiver(receiver, it->spid);

  if (bAllOpened && channel->GetTeletextStream().tpid)
    bAllOpened &= AttachMultiplexedReceiver(receiver, channel->GetTeletextStream().tpid);

  if (bAllOpened)
  {
    if (CommonInterface()->m_camSlot)
    {
      CommonInterface()->m_camSlot->StartDecrypting();
      CommonInterface()->m_startScrambleDetection = time(NULL);
    }
  }
  else
  {
    DetachReceiver(receiver);
  }

  return bAllOpened;
}

void cDeviceReceiverSubsystem::DetachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask)
{
  CLockObject lock(m_mutex);

  const ReceiverPidEdge needle(GetReceiverHandle(receiver), GetResource(CreateStreamingResource(pid, tid, mask)));
  ReceiverPidTable::iterator it = m_receiverPidTable.find(needle);
  if (it != m_receiverPidTable.end())
    m_receiverPidTable.erase(it);
}

void cDeviceReceiverSubsystem::DetachMultiplexedReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type /* = STREAM_TYPE_UNDEFINED */)
{
  CLockObject lock(m_mutex);

  const ReceiverPidEdge needle(GetReceiverHandle(receiver), CreateMultiplexedResource(pid, type));
  ReceiverPidTable::iterator it = m_receiverPidTable.find(needle);
  if (it != m_receiverPidTable.end())
    m_receiverPidTable.erase(it);
}

void cDeviceReceiverSubsystem::DetachReceiver(iReceiver* receiver)
{
  CLockObject lock(m_mutex);

  for (ReceiverPidTable::iterator it = m_receiverPidTable.begin(); it != m_receiverPidTable.end(); )
  {
    if (it->first->receiver == receiver)
      m_receiverPidTable.erase(it++);
    else
      ++it;
  }
}

void cDeviceReceiverSubsystem::DetachAllReceivers(void)
{
  CLockObject lock(m_mutex);

  m_receiverPidTable.clear();
}

bool cDeviceReceiverSubsystem::Receiving(void) const
{
  CLockObject lock(m_mutex);

  return !m_receiverPidTable.empty();
}

}
