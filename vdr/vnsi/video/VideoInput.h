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
 */

#pragma once

#include "lib/platform/threads/threads.h"
#include "channels/Channel.h"
#include "devices/Device.h"

class cLivePatFilter;
class cLiveReceiver;
class cVideoBuffer;

class cVideoInput : public PLATFORM::CThread
{
friend class cLivePatFilter;
friend class cLiveReceiver;
public:
  cVideoInput();
  virtual ~cVideoInput();
  bool Open(ChannelPtr channel, int priority, cVideoBuffer *videoBuffer);
  void Close();

protected:
  virtual void* Process(void);
  void PmtChange(int pidChange);
  cChannel *PmtChannel();
  void Receive(uchar *data, int length);
  void Attach(bool on);
  DevicePtr         m_Device;
  cLivePatFilter   *m_PatFilter;
  cLiveReceiver    *m_Receiver;
  cLiveReceiver    *m_Receiver0;
  ChannelPtr        m_Channel;
  cVideoBuffer     *m_VideoBuffer;
  int               m_Priority;
  bool              m_PmtChange;
  bool              m_SeenPmt;
};
