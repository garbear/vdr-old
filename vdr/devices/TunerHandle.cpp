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

#include "TunerHandle.h"
#include "epg/EPGScanner.h"
#include "scan/Scanner.h"
#include "commoninterface/CI.h"
#include "subsystems/DeviceChannelSubsystem.h"
#include "subsystems/DeviceReceiverSubsystem.h"
#include "subsystems/DeviceCommonInterfaceSubsystem.h"

using namespace VDR;

const TunerHandlePtr cTunerHandle::EmptyHandle;

cTunerHandle::cTunerHandle(device_tuning_type_t type, cDevice* tuner, iTunerHandleCallbacks* callbacks, const ChannelPtr& channel) :
    m_type(type),
    m_tuner(tuner),
    m_callbacks(callbacks),
    m_channel(channel),
    m_startEpgScan(false),
    m_startChannelScan(false)
{
}

cTunerHandle::~cTunerHandle(void)
{
  Release();
}

std::string cTunerHandle::ToString(void) const
{
  const char* type;
  switch (m_type)
  {
  case TUNING_TYPE_RECORDING:
    type = "recording";
    break;
  case TUNING_TYPE_LIVE_TV:
    type = "live tv";
    break;
  case TUNING_TYPE_CHANNEL_SCAN:
    type = "channel scan";
    break;
  case TUNING_TYPE_EPG_SCAN:
    type = "epg scan";
    break;
  default:
    type = "unknown subscription";
    break;
  }
  return StringUtils::Format("%s on frequency %u MHz", type, m_channel->GetTransponder().FrequencyMHz());
}

void cTunerHandle::LockAcquired(void)
{
  if (m_callbacks)
    m_callbacks->LockAcquired();
}

void cTunerHandle::LockLost(void)
{
  if (m_callbacks)
    m_callbacks->LockLost();
}

void cTunerHandle::LostPriority(void)
{
  if (m_callbacks)
    m_callbacks->LostPriority();
}

void cTunerHandle::DetachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask, bool wait, receiver_change_processed_t cb /* = NULL */, void* cbarg /* = NULL */)
{
  m_tuner->Receiver()->DetachStreamingReceiver(receiver, pid, tid, mask, wait, cb, cbarg);
}

void cTunerHandle::DetachReceiver(iReceiver* receiver, bool wait, receiver_change_processed_t cb /* = NULL */, void* cbarg /* = NULL */)
{
  PLATFORM::CLockObject lock(m_mutex);
  m_tuner->Receiver()->DetachReceiver(receiver, wait, cb, cbarg);
  m_receivers.erase(receiver);
}

void cTunerHandle::SyncPids(bool wait /* = true */, receiver_change_processed_t cb /* = NULL */, void* cbarg /* = NULL */)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (m_tuner)
    m_tuner->Receiver()->SyncPids(wait, cb, cbarg);
}

bool cTunerHandle::AttachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask, receiver_change_processed_t cb /* = NULL */, void* cbarg /* = NULL */)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (m_tuner->Receiver()->AttachStreamingReceiver(receiver, pid, tid, mask, cb, cbarg))
  {
    m_receivers.insert(receiver);
    return true;
  }
  return false;
}

bool cTunerHandle::AttachMultiplexedReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type /* = STREAM_TYPE_UNDEFINED */, receiver_change_processed_t cb /* = NULL */, void* cbarg /* = NULL */)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (m_tuner->Receiver()->AttachMultiplexedReceiver(receiver, pid, type, cb, cbarg))
  {
    m_receivers.insert(receiver);
    return true;
  }
  return false;
}

bool cTunerHandle::AttachMultiplexedReceiver(iReceiver* receiver, const ChannelPtr& channel, receiver_change_processed_t cb /* = NULL */, void* cbarg /* = NULL */)
{
  bool bAllOpened(true);

  if (bAllOpened && channel->GetVideoStream().vpid)
    bAllOpened &= AttachMultiplexedReceiver(receiver, channel->GetVideoStream().vpid, channel->GetVideoStream().vtype);

  if (bAllOpened && channel->GetVideoStream().ppid != channel->GetVideoStream().vpid)
    bAllOpened &= AttachMultiplexedReceiver(receiver, channel->GetVideoStream().ppid);

  for (std::vector<AudioStream>::const_iterator it = channel->GetAudioStreams().begin(); bAllOpened && it != channel->GetAudioStreams().end(); ++it)
    bAllOpened &= AttachMultiplexedReceiver(receiver, it->apid, it->atype);

  for (std::vector<DataStream>::const_iterator it = channel->GetDataStreams().begin(); bAllOpened && it != channel->GetDataStreams().end(); ++it)
    bAllOpened &= AttachMultiplexedReceiver(receiver, it->dpid, it->dtype);

  for (std::vector<SubtitleStream>::const_iterator it = channel->GetSubtitleStreams().begin(); bAllOpened && it != channel->GetSubtitleStreams().end(); ++it)
    bAllOpened &= AttachMultiplexedReceiver(receiver, it->spid);

  if (bAllOpened && channel->GetTeletextStream().tpid)
    bAllOpened &= AttachMultiplexedReceiver(receiver, channel->GetTeletextStream().tpid);

  if (bAllOpened)
  {
    if (cb)
      SyncPids(false, cb, cbarg);
    if (m_tuner->CommonInterface()->m_camSlot)
    {
      m_tuner->CommonInterface()->m_camSlot->StartDecrypting();
      m_tuner->CommonInterface()->m_startScrambleDetection = time(NULL);
    }
  }
  else
  {
    DetachReceiver(receiver, false);
  }

  return bAllOpened;
}

void cTunerHandle::Release(bool notify /* = true */)
{
  {
    if (m_tuner)
      m_tuner->Release(this, notify);

    PLATFORM::CLockObject lock(m_mutex);
    for (std::set<iReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
      m_tuner->Receiver()->DetachReceiver(*it, false);
    m_receivers.clear();
  }
  if (m_tuner)
    m_tuner->Receiver()->SyncPids();

  //TODO fix this properly later
//  if (m_startChannelScan)
//    cScanner::Get().Start();
//  else if (m_startEpgScan)
//    cEPGScanner::Get().Start();
}

bool cTunerHandle::SignalQuality(signal_quality_info_t& info) const
{
  PLATFORM::CLockObject lock(m_mutex);
  return m_tuner ? m_tuner->Channel()->SignalQuality(info) : false;
}
