/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *
 */

#include "VideoInput.h"
#include "VideoBuffer.h"

#include "LiveReceiver.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceSectionFilterSubsystem.h"
#include "devices/Remux.h"
#include "dvb/filters/PAT.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/Receiver.h"
#include "settings/Settings.h"

using namespace PLATFORM;

namespace VDR
{

cVideoInput::cVideoInput()
{
  m_Receiver  = NULL;
  ResetMembers();
}

cVideoInput::~cVideoInput()
{
  Close();
}

void cVideoInput::ResetMembers(void)
{
  DELETENULL(m_Receiver);
  if (m_Channel)
    m_Channel->UnregisterObserver(this);
  m_Channel     = cChannel::EmptyChannel;
  m_VideoBuffer = NULL;
  m_Priority    = 0;
  m_PmtChange   = false;
  m_Device      = cDevice::EmptyDevice;
  m_SeenPmt     = false;
}

bool cVideoInput::Open(ChannelPtr channel, int priority, cVideoBuffer *videoBuffer)
{
  CLockObject lock(m_mutex);

  m_Device = cDeviceManager::Get().GetDevice(*channel, priority, true);
  if (m_Device)
  {
    dsyslog("found device: '%s' (%d) for channel '%s'",
        m_Device->DeviceName().c_str(), m_Device->CardIndex(), channel->Name().c_str());

    m_VideoBuffer = videoBuffer;
    m_Priority    = priority;
    m_Channel     = channel;
    m_Channel->RegisterObserver(this);

    if (m_Device->Channel()->SwitchChannel(m_Channel))
    {
      dsyslog("Creating new live Receiver");
      m_SeenPmt   = false;
      m_Receiver  = new cLiveReceiver(m_Device, this, m_Channel, m_Priority);
      m_Device->Receiver()->AttachReceiver(m_Receiver);
      CreateThread();
      return true;
    }
  }
  return false;
}

void cVideoInput::CancelPMTThread(void)
{
  StopThread(-1);
  {
    CLockObject lock(m_mutex);
    m_SeenPmt = true;
  }
  StopThread(0);
}

void cVideoInput::Close()
{
  CancelPMTThread();

  DevicePtr device;
  cLiveReceiver* receiver;
  {
    CLockObject lock(m_mutex);
    device   = m_Device;
    m_Device = cDevice::EmptyDevice;

    receiver   = m_Receiver;
    m_Receiver = NULL;

    if (m_Channel)
      m_Channel->UnregisterObserver(this);
    m_Channel = cChannel::EmptyChannel;
  }

  if (device)
  {
    if (receiver)
    {
      dsyslog("Detaching Live Receiver");
      device->Receiver()->Detach(receiver);
    }
    else
    {
      dsyslog("No live receiver present");
    }

    delete receiver;
  }

  ResetMembers();
}

void cVideoInput::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageChannelHasPMT)
    PmtChange();
}

void cVideoInput::PmtChange(void)
{
  isyslog("VideoInput - new pmt, attaching receiver");

  if (m_Receiver)
    m_Receiver->SetPids(*m_Channel);

  CLockObject lock(m_mutex);
  m_PmtChange = true;
  m_SeenPmt   = true;
}

void cVideoInput::Receive(uchar *data, int length)
{
  CLockObject lock(m_mutex);
  if (!m_Device)
    return;

  if (m_PmtChange)
  {
     // generate pat/pmt so we can configure parsers later
     cPatPmtGenerator patPmtGenerator(m_Channel);
     m_VideoBuffer->Put(patPmtGenerator.GetPat(), TS_SIZE);
     int Index = 0;
     while (uchar *pmt = patPmtGenerator.GetPmt(Index))
       m_VideoBuffer->Put(pmt, TS_SIZE);
     m_PmtChange = false;
  }
  m_VideoBuffer->Put(data, length);
}

void cVideoInput::Attach(bool on)
{
  CLockObject lock(m_mutex);
  m_VideoBuffer->AttachInput(on);
}

void* cVideoInput::Process()
{
  cPat pat(m_Device.get());

  ChannelVector channels;
  while (!IsStopped() && channels.empty())
    channels = pat.GetChannels();

  if (IsStopped())
    return NULL;

  CLockObject lock(m_mutex);
  if (m_Channel)
  {
    for (ChannelVector::const_iterator it = channels.begin(); it != channels.end(); ++it)
    {
      if ((*it)->GetSid() == m_Channel->GetSid())
      {
        // Clear modification flag
        m_Channel->Modification();

        m_Channel->SetStreams((*it)->GetVideoStream(),
                              (*it)->GetAudioStreams(),
                              (*it)->GetDataStreams(),
                              (*it)->GetSubtitleStreams(),
                              (*it)->GetTeletextStream());

        m_Channel->NotifyObservers(ObservableMessageChannelHasPMT);
        if (m_Channel->Modification(CHANNELMOD_PIDS))
          m_Channel->NotifyObservers(ObservableMessageChannelPMTChanged);

        break;
      }
    }
  }

  if (!m_SeenPmt)
  {
    isyslog("VideoInput: no pat/pmt within timeout, falling back to channel pids");
    PmtChange();
  }

  return NULL;
}

}
