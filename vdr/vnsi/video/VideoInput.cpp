/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2007 Chris Tallon
 *      Portions Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Portions Copyright (C) 2010, 2011 Alexander Pipelka
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "VideoInput.h"
#include "VideoBuffer.h"

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
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"

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
  m_Channel     = cChannel::EmptyChannel;
  m_VideoBuffer = NULL;
  m_PmtChange   = false;
  m_Device      = cDevice::EmptyDevice;
}

bool cVideoInput::Open(ChannelPtr channel, cVideoBuffer *videoBuffer)
{
  CLockObject lock(m_mutex);

  m_Device = cDeviceManager::Get().GetDevice(0); // TODO
  if (m_Device)
  {
    m_VideoBuffer = videoBuffer;
    m_Channel     = channel;

    if (m_Device->Channel()->SwitchChannel(m_Channel))
    {
      dsyslog("Creating new live Receiver");
      m_Receiver  = new cLiveReceiver(m_Device, this, m_Channel);
      m_Device->Receiver()->AttachReceiver(m_Receiver);

      PmtChange();

      return true;
    }
  }
  return false;
}

void cVideoInput::Close()
{
  DevicePtr device;
  cLiveReceiver* receiver;
  {
    CLockObject lock(m_mutex);
    device   = m_Device;
    m_Device = cDevice::EmptyDevice;

    receiver   = m_Receiver;
    m_Receiver = NULL;

    m_Channel.reset();
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

void cVideoInput::PmtChange(void)
{
  if (m_Receiver)
    m_Receiver->SetPids(*m_Channel);

  CLockObject lock(m_mutex);
  m_PmtChange = true;
}

void cVideoInput::Receive(uint8_t *data, int length)
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
     while (uint8_t *pmt = patPmtGenerator.GetPmt(Index))
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

}
