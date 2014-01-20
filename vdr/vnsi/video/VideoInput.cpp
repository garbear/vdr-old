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

#include "PATFilter.h"
#include "LiveReceiver.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceSectionFilterSubsystem.h"
#include "devices/Remux.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/Receiver.h"
#include "settings/Settings.h"

using namespace PLATFORM;

cVideoInput::cVideoInput()
{
  ResetMembers();
}

cVideoInput::~cVideoInput()
{
  Close();
}

void cVideoInput::ResetMembers(void)
{
  DELETENULL(m_PatFilter);
  DELETENULL(m_Receiver);
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
    dsyslog("found device: '%s' (%d) for channel '%s'", m_Device->DeviceName().c_str(), m_Device->CardIndex(), channel->Name().c_str());

    m_VideoBuffer = videoBuffer;
    m_Priority    = priority;
    m_Channel     = channel;
    m_Channel->RegisterObserver(this);

    if (m_Device->Channel()->SwitchChannel(m_Channel))
    {
      dsyslog("Creating new live Receiver");
      m_SeenPmt   = false;
      m_PatFilter = new cLivePatFilter(this, m_Channel);
      m_Receiver  = new cLiveReceiver(this, m_Channel, m_Priority);
      m_Device->Receiver()->AttachReceiver(m_Receiver);
      m_Device->SectionFilter()->AttachFilter(m_PatFilter);
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
    m_pmtCondition.Broadcast();
  }
  StopThread(0);
}

void cVideoInput::Close()
{
  CancelPMTThread();

  DevicePtr device;
  cLiveReceiver* receiver;
  cLivePatFilter* patFilter;
  {
    CLockObject lock(m_mutex);
    device   = m_Device;
    m_Device = cDevice::EmptyDevice;

    receiver   = m_Receiver;
    m_Receiver = NULL;

    patFilter   = m_PatFilter;
    m_PatFilter = NULL;

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

    if (patFilter)
    {
      dsyslog("Detaching Live Filter");
      device->SectionFilter()->Detach(patFilter);
    }
    else
    {
      dsyslog("No live filter present");
    }

    delete receiver;
    delete patFilter;
  }

  ResetMembers();
}

void cVideoInput::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageChannelPMTChanged)
    PmtChange();
}

void cVideoInput::PmtChange(void)
{
  isyslog("Video Input - new pmt, attaching receiver");

  CLockObject lock(m_mutex);
  assert(m_Receiver->m_PmtChannel.get());

  m_Device->Receiver()->Detach(m_Receiver);
  m_Receiver->SetPids(*m_Receiver->m_PmtChannel);
  m_Device->Receiver()->AttachReceiver(m_Receiver);
  m_PmtChange = true;
  m_SeenPmt   = true;
  m_pmtCondition.Signal();
}

void cVideoInput::Receive(uchar *data, int length)
{
  CLockObject lock(m_mutex);
  if (!m_Device)
    return;

  if (m_PmtChange)
  {
     // generate pat/pmt so we can configure parsers later
     cPatPmtGenerator patPmtGenerator(m_Receiver->m_PmtChannel);
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
  CLockObject lock(m_mutex);
  if (!m_pmtCondition.Wait(m_mutex, m_SeenPmt, (unsigned int)cSettings::Get().m_PmtTimeout*1000))
  {
    if (!IsStopped())
    {
      isyslog("VideoInput: no pat/pmt within timeout, falling back to channel pids");
      m_Receiver->m_PmtChannel = m_Channel;
      PmtChange();
    }
  }

  return NULL;
}
