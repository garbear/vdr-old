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
#pragma once

#include "channels/ChannelTypes.h"
#include "devices/DeviceTypes.h"
#include "dvb/filters/Filter.h"
#include "lib/platform/threads/threads.h"

#include <stdint.h>

namespace VDR
{

class cLiveReceiver;
class cVideoBuffer;

class cVideoInput : public PLATFORM::CThread, public iFilterCallback
{
friend class cLiveReceiver;
public:
  cVideoInput();
  virtual ~cVideoInput();
  bool Open(ChannelPtr channel, cVideoBuffer *videoBuffer);
  void Close();

  virtual void OnChannelParamsScanned(const ChannelPtr& channel) { m_channels.push_back(channel); } // TODO

protected:
  void ResetMembers(void);
  virtual void* Process(void);
  void PmtChange(void);
  void Receive(uint8_t *data, int length);
  void Attach(bool on);
  void CancelPMTThread(void);
  DevicePtr                  m_Device;
  cLiveReceiver*             m_Receiver;
  ChannelPtr                 m_Channel;
  cVideoBuffer*              m_VideoBuffer;
  PLATFORM::CMutex           m_mutex;
  bool                       m_PmtChange;
  bool                       m_SeenPmt;
  ChannelVector              m_channels;
};

}
