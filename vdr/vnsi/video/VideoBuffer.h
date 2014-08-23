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

#include "devices/Receiver.h"
#include "utils/Timer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <time.h>

namespace VDR
{

#define VIDEOBUFFER_NO_DATA (-1)
#define VIDEOBUFFER_EOF     (-2)

class cRecording;

class cVideoBuffer : public iReceiver
{
public:
  virtual ~cVideoBuffer(void) { }

  virtual void Start(void);
  virtual void Stop(void);
  virtual void Receive(const std::vector<uint8_t>& data) = 0;

  virtual int ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime) = 0;
  virtual off_t GetPosMin() { return 0; };
  virtual off_t GetPosMax() { return 0; };
  virtual off_t GetPosCur() { return 0; };
  virtual void GetPositions(off_t *cur, off_t *min, off_t *max) {};
  virtual void SetPos(off_t pos) {};
  virtual void SetCache(bool on) {};
  virtual bool HasBuffer() { return false; };
  virtual time_t GetRefTime();
  int Read(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime);

  /*!
   * Factory methods
   */
  static cVideoBuffer* Create(int clientID, uint8_t timeshift);
  static cVideoBuffer* Create(const std::string& filename);
  static cVideoBuffer* Create(cRecording *rec);

protected:
  cVideoBuffer(void);

  cTimeMs m_Timer;
  bool    m_CheckEof;
  bool    m_InputAttached;
  time_t  m_bufferEndTime;
  time_t  m_bufferWrapTime;
};

}
