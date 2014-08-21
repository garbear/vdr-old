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
#include "lib/platform/threads/mutex.h"

#include <stdint.h>

namespace VDR
{

class cLiveReceiver;
class cVideoBuffer;

class cVideoInput
{
friend class cLiveReceiver;
public:
  cVideoInput();
  virtual ~cVideoInput();
  bool Open(ChannelPtr channel, cVideoBuffer *videoBuffer);
  void Close();

protected:
  void ResetMembers(void);
  void PmtChange(void);
  void Receive(const std::vector<uint8_t>& data);
  void Attach(bool on);

  DevicePtr                  m_Device;
  cLiveReceiver*             m_Receiver;
  ChannelPtr                 m_Channel;
  cVideoBuffer*              m_VideoBuffer;
  PLATFORM::CMutex           m_mutex;
  bool                       m_PmtChange;
};

}
