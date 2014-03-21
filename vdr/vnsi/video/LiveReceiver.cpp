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

#include "LiveReceiver.h"
#include "VideoInput.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"

namespace VDR
{

cLiveReceiver::cLiveReceiver(DevicePtr device, cVideoInput *VideoInput, ChannelPtr Channel, int Priority) :
  cReceiver(Channel, Priority),
  m_device(device),
  m_VideoInput(VideoInput)
{
  if (Channel)
    SetPids(*Channel);
}

cLiveReceiver::~cLiveReceiver()
{
  if (m_device)
    m_device->Receiver()->Detach(this);
}

void cLiveReceiver::Receive(uchar *Data, int Length)
{
  m_VideoInput->Receive(Data, Length);
}

void cLiveReceiver::Activate(bool On)
{
  m_VideoInput->Attach(On);
  dsyslog("%s live receiver", On ? "activate" : "deactivate");
}

}
