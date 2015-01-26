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

using namespace PLATFORM;
using namespace std;

#define TS_SCRAMBLING_TIMEOUT     3 // seconds to wait until a TS becomes unscrambled
#define TS_SCRAMBLING_TIME_OK    10 // seconds before a Channel/CAM combination is marked as known to decrypt

#define MAX_IDLE_DELAY_MS      100

#define DEBUG_RCV_CHANGES (1)

#if DEBUG_RCV_CHANGES
#define DEBUG_RCV_CHANGE(x...) dsyslog(x)
#else
#define DEBUG_RCV_CHANGE(x...)
#endif

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
 : cDeviceSubsystem(device),
   m_changed(false),
   m_changeProcessed(false)
{
}

cDeviceReceiverSubsystem::~cDeviceReceiverSubsystem(void)
{
  DetachAllReceivers(false);
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

void cDeviceReceiverSubsystem::ProcessDetachAll(void)
{
  DEBUG_RCV_CHANGE("detaching all receivers");
  m_receiverPidTable.clear();
}

void cDeviceReceiverSubsystem::ProcessAttachMultiplexed(cDeviceReceiverSubsystem::cReceiverChange& change)
{
  DEBUG_RCV_CHANGE("ProcessChanges: attaching multiplexed receiver for pid %d", change.m_pid);
  AttachReceiver(change.m_receiver, CreateMultiplexedResource(change.m_pid, change.m_streamType));
}

void cDeviceReceiverSubsystem::ProcessAttachStreaming(cDeviceReceiverSubsystem::cReceiverChange& change)
{
  DEBUG_RCV_CHANGE("ProcessChanges: attaching streaming receiver for pid %d tid %d mask %d", change.m_pid, change.m_tid, change.m_mask);
  AttachReceiver(change.m_receiver, CreateStreamingResource(change.m_pid, change.m_tid, change.m_mask));
}

void cDeviceReceiverSubsystem::ProcessDetachReceiver(cDeviceReceiverSubsystem::cReceiverChange& change)
{
  ReceiverList receiverList;
  DEBUG_RCV_CHANGE("ProcessChanges: detaching receiver %p", change.m_receiver);
  for (ReceiverPidTable::iterator itReceiverList = m_receiverPidTable.begin(); itReceiverList != m_receiverPidTable.end(); ++itReceiverList)
  {
    receiverList = itReceiverList->second;
    for (ReceiverList::iterator itReceiver = receiverList.begin(); itReceiver != receiverList.end(); ++itReceiver)
    {
      if (itReceiver->first->receiver == change.m_receiver)
      {
        itReceiverList->second.erase(itReceiver);
        break;
      }
    }
    if (receiverList.empty())
      m_receiverPidTable.erase(itReceiverList);
  }
}

void cDeviceReceiverSubsystem::ProcessDetachMultiplexed(cDeviceReceiverSubsystem::cReceiverChange& change)
{
  ReceiverPidTable::iterator itReceiverList = m_receiverPidTable.find(change.m_pid);
  DEBUG_RCV_CHANGE("ProcessChanges: detaching multiplexed receiver %p from pid %u", change.m_receiver, change.m_pid);
  if (itReceiverList != m_receiverPidTable.end())
  {
    ReceiverList receiverList = itReceiverList->second;
    for (ReceiverList::iterator itReceiver = receiverList.begin(); itReceiver != receiverList.end(); ++itReceiver)
    {
      if (itReceiver->first->receiver == change.m_receiver)
      {
        itReceiverList->second.erase(itReceiver);
        break;
      }
    }
    if (receiverList.empty())
      m_receiverPidTable.erase(itReceiverList);
  }
}

void cDeviceReceiverSubsystem::ProcessDetachStreaming(cDeviceReceiverSubsystem::cReceiverChange& change)
{
  //TODO remove tid/mask
  ReceiverPidTable::iterator itReceiverList = m_receiverPidTable.find(change.m_pid);
  DEBUG_RCV_CHANGE("ProcessChanges: detaching streaming receiver %p from pid %u", change.m_receiver, change.m_pid);
  if (itReceiverList != m_receiverPidTable.end())
  {
    ReceiverList receiverList = itReceiverList->second;
    for (ReceiverList::iterator itReceiver = receiverList.begin(); itReceiver != receiverList.end(); ++itReceiver)
    {
      if (itReceiver->first->receiver == change.m_receiver)
      {
        itReceiverList->second.erase(itReceiver);
        break;
      }
    }
    if (receiverList.empty())
      m_receiverPidTable.erase(itReceiverList);
  }
}

void cDeviceReceiverSubsystem::ProcessChanges(void)
{
  cReceiverChange* change;
  while (!m_receiverChanges.empty())
  {
    change = m_receiverChanges.front();
    switch (change->m_type)
    {
      case RCV_CHANGE_DETACH_ALL:
        ProcessDetachAll();
        break;
      case RCV_CHANGE_ATTACH_MULTIPLEXED:
        ProcessAttachMultiplexed(*change);
        break;
      case RCV_CHANGE_ATTACH_STREAMING:
        ProcessAttachStreaming(*change);
        break;
      case RCV_CHANGE_DETACH:
        ProcessDetachReceiver(*change);
        break;
      case RCV_CHANGE_DETACH_MULTIPLEXED:
        ProcessDetachMultiplexed(*change);
        break;
      case RCV_CHANGE_DETACH_STREAMING:
        ProcessDetachStreaming(*change);
        break;
    }
    delete change;
    m_receiverChanges.pop();
  }

  m_changeProcessed = true;
  m_pidChangeProcessed.Broadcast();
}

void *cDeviceReceiverSubsystem::Process()
{
  if (!Initialise())
    return NULL;

  TsPacket packet;
  uint16_t pid;
  const uint8_t* psidata;
  size_t psidatalen;
  bool validpsi, psichecked;
  PidResourcePtr resource;
  ts_crc_check_t crcCheck;
  iReceiver* receiver;
  PidResourcePtr pidPtr;
  bool empty = true;

  while (!IsStopped())
  {
    {
      CLockObject lock(m_mutex);
      if (m_changed)
      {
        m_changed = false;
        ProcessChanges();
      }

      if (m_receiverPidTable.empty())
      {
        if (m_pidChange.Wait(m_mutex, m_changed, 1000))
        {
          m_changed = false;
          ProcessChanges();
        }
      }

      empty = m_receiverPidTable.empty();
    }

    if (empty)
      continue;

    /** wait for new data */
    switch (Poll(resource))
    {
      case POLL_RESULT_STREAMING_READY:
      if (!IsStopped() && resource->Read(&psidata, &psidatalen))
      {
        crcCheck = TS_CRC_NOT_CHECKED;
        /** distribute the packet to receivers holding this resource */
        std::set<iReceiver*> receivers = GetReceiversForPid(resource->Pid());
        for (std::set<iReceiver*>::iterator receiverit = receivers.begin(); receiverit != receivers.end(); ++receiverit)
          (*receiverit)->Receive(resource->Pid(), psidata, psidatalen, crcCheck);
      }
      break;

      case POLL_RESULT_MULTIPLEXED_READY:
      if (!IsStopped() && (packet = ReadMultiplexed()) != NULL)
      {
        /** find resources attached to the PID that we received */
        pid = TsPid(packet);
        crcCheck = TS_CRC_NOT_CHECKED;
        psichecked = false;

        /** distribute the packet to all receivers */
        std::set<iReceiver*> receivers = GetReceiversForPid(pid);
        for (std::set<iReceiver*>::iterator receiverit = receivers.begin(); receiverit != receivers.end(); ++receiverit)
        {
          receiver = *receiverit;
          if (receiver->IsPsiReceiver())
          {
            /** only send full data */
            if (!psichecked)
            {
              psichecked = true;
              pidPtr = GetMultiplexedResource(pid);
              validpsi = pidPtr ? pidPtr->AllocateBuffer()->AddTsData(packet, TS_SIZE, &psidata, &psidatalen) : false;
            }
            if (validpsi)
              receiver->Receive(pid, psidata, psidatalen, crcCheck);
          }
          else
          {
            receiver->Receive(pid, packet, TS_SIZE, crcCheck);
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

cDeviceReceiverSubsystem::ReceiverHandlePtr cDeviceReceiverSubsystem::GetReceiverHandle(iReceiver* receiver) const
{
  for (ReceiverPidTable::const_iterator it = m_receiverPidTable.begin(); it != m_receiverPidTable.end(); ++it)
  {
    for (ReceiverList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      if (it2->first->receiver == receiver)
        return it2->first;
  }
  return ReceiverHandlePtr();
}

std::set<iReceiver*> cDeviceReceiverSubsystem::GetReceiversForPid(uint16_t pid) const
{
  std::set<iReceiver*> receivers;
  ReceiverPidTable::const_iterator it = m_receiverPidTable.find(pid);
  if (it != m_receiverPidTable.end())
  {
    ReceiverList rcv = it->second;
    for (ReceiverList::const_iterator it = rcv.begin(); it != rcv.end(); ++it)
      receivers.insert(it->first->receiver);
  }
  return receivers;
}

cDeviceReceiverSubsystem::PidResourcePtr cDeviceReceiverSubsystem::GetResource(const PidResourcePtr& needle) const
{
  ReceiverPidTable::const_iterator it = m_receiverPidTable.find(needle->Pid());
  if (it != m_receiverPidTable.end())
  {
    for (ReceiverList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      if (it2->second->Equals(needle.get()))
        return it2->second;
  }
  return PidResourcePtr();
}

cDeviceReceiverSubsystem::PidResourcePtr cDeviceReceiverSubsystem::GetMultiplexedResource(uint16_t pid) const
{
  ReceiverPidTable::const_iterator it = m_receiverPidTable.find(pid);
  if (it != m_receiverPidTable.end())
  {
    for (ReceiverList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      if (it2->second->Type() == RESOURCE_TYPE_MULTIPLEXING && it2->second->Equals(pid))
        return it2->second;
  }
  return PidResourcePtr();
}

bool cDeviceReceiverSubsystem::AttachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask)
{
  CLockObject lock(m_mutex);
  m_receiverChanges.push(new cReceiverChange(RCV_CHANGE_ATTACH_STREAMING, receiver, pid, tid, mask));
  m_changed = true;
  m_pidChange.Signal();
  return true;
}

bool cDeviceReceiverSubsystem::AttachMultiplexedReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type /* = STREAM_TYPE_UNDEFINED */)
{
  CLockObject lock(m_mutex);
  m_receiverChanges.push(new cReceiverChange(RCV_CHANGE_ATTACH_MULTIPLEXED, receiver, pid, type));
  m_changed = true;
  m_pidChange.Signal();
  return true;
}

bool cDeviceReceiverSubsystem::AttachMultiplexedReceiver(iReceiver* receiver, const ChannelPtr& channel)
{
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
    DetachReceiver(receiver, false);
  }

  return bAllOpened;
}

bool cDeviceReceiverSubsystem::AttachReceiver(iReceiver* receiver, const PidResourcePtr& resource)
{
  //TODO fix when both streaming and multiplexed sources attach to the same pid
  PidResourcePtr openResource = GetResource(resource);
  if (!openResource)
  {
    // Resource hasn't been opened, try to open it now and bail on failure
    if (!resource->Open())
      return false;
    openResource = resource;
  }

  ReceiverHandlePtr receiverHandle = GetReceiverHandle(receiver);
  if (!receiverHandle)
  {
    if (!receiver->Start())
      return false;
    receiverHandle = ReceiverHandlePtr(new cReceiverHandle(receiver));
  }

  ReceiverPidTable::iterator it = m_receiverPidTable.find(resource->Pid());
  if (it != m_receiverPidTable.end())
  {
    for (ReceiverList::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (it2->first->receiver == receiverHandle->receiver)
        return true;
    }
    it->second.push_back(make_pair(receiverHandle, openResource));
  }
  else
  {
    ReceiverList rcvlist;
    rcvlist.push_back(make_pair(receiverHandle, openResource));
    m_receiverPidTable.insert(make_pair(resource->Pid(), rcvlist));
  }

  return true;
}

void cDeviceReceiverSubsystem::DetachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask, bool wait)
{
  CLockObject lock(m_mutex);
  m_receiverChanges.push(new cReceiverChange(RCV_CHANGE_DETACH_STREAMING, receiver, pid, tid, mask));
  m_changed = true;
  m_changeProcessed = false;
  m_pidChange.Signal();
  if (wait)
    m_pidChangeProcessed.Wait(m_mutex, m_changeProcessed);
}

void cDeviceReceiverSubsystem::DetachMultiplexedReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type /* = STREAM_TYPE_UNDEFINED */, bool wait /* = false */)
{
  CLockObject lock(m_mutex);
  m_receiverChanges.push(new cReceiverChange(RCV_CHANGE_DETACH_MULTIPLEXED, receiver, pid, type));
  m_changed = true;
  m_changeProcessed = false;
  m_pidChange.Signal();
  if (wait)
    m_pidChangeProcessed.Wait(m_mutex, m_changeProcessed);
}

void cDeviceReceiverSubsystem::DetachReceiver(iReceiver* receiver, bool wait)
{
  CLockObject lock(m_mutex);
  m_receiverChanges.push(new cReceiverChange(RCV_CHANGE_DETACH, receiver));
  m_changed = true;
  m_changeProcessed = false;
  m_pidChange.Signal();
  if (wait)
    m_pidChangeProcessed.Wait(m_mutex, m_changeProcessed);
}

void cDeviceReceiverSubsystem::DetachAllReceivers(bool wait)
{
  CLockObject lock(m_mutex);
  m_receiverChanges.push(new cReceiverChange(RCV_CHANGE_DETACH_ALL));
  m_changed = true;
  m_changeProcessed = false;
  if (wait)
  {
    m_pidChange.Signal();
    m_pidChangeProcessed.Wait(m_mutex, m_changeProcessed);
  }
}

}
