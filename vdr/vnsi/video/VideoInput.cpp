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
#include "channels/Channel.h"
#include "devices/DeviceManager.h"
#include "devices/Remux.h"
#include "utils/log/Log.h"

#include <assert.h>

using namespace PLATFORM;
using namespace std;

namespace VDR
{

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
  m_Channel     = cChannel::EmptyChannel;
  m_VideoBuffer = NULL;
  m_PmtChange   = false;
}

bool cVideoInput::Open(const ChannelPtr& channel, cVideoBuffer* videoBuffer)
{
  assert(videoBuffer);

  CLockObject lock(m_mutex);

  // TODO: Must set before calling AttachReceiver() because AttachReceiver()
  // invokes m_VideoBuffer via Activate()
  m_VideoBuffer = videoBuffer;

  if (cDeviceManager::Get().AttachReceiver(this, channel))
  {
    m_Channel = channel;
    PmtChange();
    return true;
  }

  ResetMembers(); // TODO: Only reset members if AttachReceiver() succeeds

  return false;
}

void cVideoInput::Close()
{
  CLockObject lock(m_mutex);

  cDeviceManager::Get().DetachReceiver(this);
  ResetMembers();
}

void cVideoInput::PmtChange(void)
{
  SetPids(*m_Channel);

  CLockObject lock(m_mutex);
  m_PmtChange = true;
}

void cVideoInput::SetChannel(const ChannelPtr& channel)
{
  CLockObject lock(m_mutex);
  m_Channel = channel;
}

void cVideoInput::SetVideoBuffer(cVideoBuffer* videoBuffer)
{
  CLockObject lock(m_mutex);
  m_VideoBuffer = videoBuffer;
}

void cVideoInput::Receive(const std::vector<uint8_t>& data)
{
  CLockObject lock(m_mutex);

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
  m_VideoBuffer->Put(data.data(), data.size());
}

void cVideoInput::Activate(bool bOn)
{
  CLockObject lock(m_mutex);
  m_VideoBuffer->AttachInput(bOn);
  dsyslog("%s live receiver", bOn ? "Activate" : "Deactivate");
}

}
