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

#include "parser/Parser.h"
#include "channels/ChannelTypes.h"
#include "devices/Remux.h"
#include "utils/Observer.h"

#include <list>
#include <stdint.h>
#include <set>

namespace VDR
{

struct sStreamPacket;
class cTSStream;
class cVideoBuffer;

struct sStreamInfo
{
  int pID;
  eStreamType type;
  eStreamContent content;
  char language[MAXLANGCODE2];
  int subtitlingType;
  int compositionPageId;
  int ancillaryPageId;
  void SetLanguage(const char* lang)
  {
    language[0] = lang[0];
    language[1] = lang[1];
    language[2] = lang[2];
    language[3] = 0;
  }
};

class cVNSIDemuxer : public Observer
{
public:
  cVNSIDemuxer(int clientID, uint8_t timeshift);
  virtual ~cVNSIDemuxer();

  bool Open(const ChannelPtr& channel, int serial);
  void Close();

  int Read(sStreamPacket *packet);
  cTSStream *GetFirstStream();
  cTSStream *GetNextStream();

  bool SeekTime(int64_t time);

  uint32_t GetSerial() { return m_MuxPacketSerial; }
  void SetSerial(uint32_t serial) { m_MuxPacketSerial = serial; }

  void BufferStatus(bool &timeshift, uint32_t &start, uint32_t &end);

  uint16_t GetError();

  virtual void Notify(const Observable &obs, const ObservableMessage msg);

protected:
  bool EnsureParsers(const ChannelPtr& channel);
  void ResetParsers();

  std::vector<sStreamInfo> GetStreamsFromChannel(const ChannelPtr& channel);
  cTSStream *FindStream(int Pid);

  bool GetTimeAtPos(off_t *pos, int64_t *time);

  bool PidsChanged(void);

  int m_clientID;
  std::list<cTSStream*> m_Streams;
  std::list<cTSStream*>::iterator m_StreamsIterator;
  ChannelPtr m_CurrentChannel;
  std::set<uint16_t> m_pids;
  bool m_WaitIFrame;
  int64_t m_FirstFramePTS;
  cVideoBuffer *m_VideoBuffer;
  PLATFORM::CMutex m_Mutex;
  uint32_t m_MuxPacketSerial;
  sPtsWrap m_PtsWrap;
  uint16_t m_Error;
  bool m_SetRefTime;
  time_t m_refTime, m_endTime, m_wrapTime;
  uint8_t m_timeshift;
  TunerHandlePtr m_tunerHandle;
};

}
