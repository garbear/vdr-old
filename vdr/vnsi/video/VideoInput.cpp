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
  m_PatFilter   = NULL;
  m_Receiver    = NULL;
  m_Receiver0   = NULL;
  m_Channel     = cChannel::EmptyChannel;
  m_VideoBuffer = NULL;
  m_Priority    = 0;
  m_PmtChange   = false;
  m_Device      = cDevice::EmptyDevice;
  m_SeenPmt     = false;
}

cVideoInput::~cVideoInput()
{
  Close();
}

bool cVideoInput::Open(ChannelPtr channel, int priority, cVideoBuffer *videoBuffer)
{
  CLockObject lock(m_mutex);

  m_VideoBuffer = videoBuffer;
  m_Channel = channel;
  m_Priority = priority;
  m_Device = cDeviceManager::Get().GetDevice(*m_Channel, m_Priority, true);

  if (m_Device)
  {
    dsyslog("Successfully found following device: %s (%d) for receiving", m_Device->DeviceName().c_str(), m_Device->CardIndex());

    if (m_Device->Channel()->SwitchChannel(m_Channel))
    {
      dsyslog("Creating new live Receiver");
      m_SeenPmt   = false;
      m_PatFilter = new cLivePatFilter(this, m_Channel);
      m_Receiver0 = new cLiveReceiver(this, m_Channel, m_Priority);
      m_Receiver  = new cLiveReceiver(this, m_Channel, m_Priority);
      m_Device->Receiver()->AttachReceiver(m_Receiver0);
      m_Device->SectionFilter()->AttachFilter(m_PatFilter);
      CreateThread();
      return true;
    }
  }
  return false;
}

void cVideoInput::Close()
{
  StopThread(5000);

  CLockObject lock(m_mutex);
  if (m_Device)
  {
    if (m_Receiver)
    {
      dsyslog("Detaching Live Receiver");
      m_Device->Receiver()->Detach(m_Receiver);
    }
    else
    {
      dsyslog("No live receiver present");
    }

    if (m_Receiver0)
    {
      dsyslog("Detaching Live Receiver0");
      m_Device->Receiver()->Detach(m_Receiver0);
    }
    else
    {
      dsyslog("No live receiver present");
    }

    if (m_PatFilter)
    {
      dsyslog("Detaching Live Filter");
      m_Device->SectionFilter()->Detach(m_PatFilter);
    }
    else
    {
      dsyslog("No live filter present");
    }

    if (m_Receiver)
    {
      dsyslog("Deleting Live Receiver");
      DELETENULL(m_Receiver);
    }

    if (m_Receiver0)
    {
      dsyslog("Deleting Live Receiver0");
      DELETENULL(m_Receiver0);
    }

    if (m_PatFilter)
    {
      dsyslog("Deleting Live Filter");
      DELETENULL(m_PatFilter);
    }
  }
}

ChannelPtr cVideoInput::PmtChannel()
{
  CLockObject lock(m_mutex);
  return m_Receiver->m_PmtChannel;
}

void cVideoInput::PmtChange(int pidChange)
{
  if (pidChange)
  {
    isyslog("Video Input - new pmt, attaching receiver");
    CLockObject lock(m_mutex);
    assert(m_Receiver->m_PmtChannel.get());
    m_Receiver->SetPids(*m_Receiver->m_PmtChannel);
    m_Device->Receiver()->AttachReceiver(m_Receiver);
    m_PmtChange = true;
    m_SeenPmt = true;
  }
}

void cVideoInput::Receive(uchar *data, int length)
{
  CLockObject lock(m_mutex);
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
  cTimeMs starttime;

  while (!IsStopped())
  {
    {
      CLockObject lock(m_mutex);
      if (starttime.Elapsed() > (unsigned int)cSettings::Get().m_PmtTimeout*1000)
      {
        isyslog("VideoInput: no pat/pmt within timeout, falling back to channel pids");
        m_Receiver->m_PmtChannel = m_Channel;
        PmtChange(true);
      }
      if (m_SeenPmt)
        break;
    }

    usleep(1000);
  }
  return NULL;
}
