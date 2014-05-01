/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "platform/threads/threads.h"

#include <stdint.h>

namespace VDR
{

// Derived cDevice classes that can receive channels will have to provide
// Transport Stream (TS) packets one at a time. cTSBuffer implements a
// simple buffer that allows the device to read a larger amount of data
// from the driver with each call to Read(), thus avoiding the overhead
// of getting each TS packet separately from the driver. It also makes
// sure the returned data points to a TS packet and automatically
// re-synchronizes after broken packets.

class cRingBufferLinear;

class cTSBuffer : public PLATFORM::CThread
{
public:
  cTSBuffer(int file, unsigned int size, int cardIndex);
  virtual ~cTSBuffer();

  uint8_t* Get();

private:
  virtual void *Process();

  int                m_file;
  int                m_cardIndex;
  bool               m_bDelivered;
  cRingBufferLinear *m_ringBuffer;
};

}
