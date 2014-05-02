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

#include <list>

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

class cVNSIDemuxer
{
public:
  cVNSIDemuxer();
  virtual ~cVNSIDemuxer();
  int Read(sStreamPacket *packet);
  cTSStream *GetFirstStream();
  cTSStream *GetNextStream();
  void Open(ChannelPtr channel, cVideoBuffer *videoBuffer);
  void Close();
  bool SeekTime(int64_t time);
  uint32_t GetSerial() { return m_MuxPacketSerial; }
  void SetSerial(uint32_t serial) { m_MuxPacketSerial = serial; }
  void BufferStatus(bool &timeshift, uint32_t &start, uint32_t &end);
  uint16_t GetError();

protected:
  bool EnsureParsers();
  void ResetParsers();
  void UpdateStreamsFromChannel(void);
  void UpdateChannelPIDsFromPMT(void);
  cTSStream *FindStream(int Pid);
  void AddStreamInfo(sStreamInfo &stream);
  bool GetTimeAtPos(off_t *pos, int64_t *time);
  std::list<cTSStream*> m_Streams;
  std::list<cTSStream*>::iterator m_StreamsIterator;
  std::list<sStreamInfo> m_StreamInfos;
  ChannelPtr m_CurrentChannel;
  cPatPmtParser m_PatPmtParser;
  int m_OldPmtVersion;
  bool m_WaitIFrame;
  int64_t m_FirstFramePTS;
  cVideoBuffer *m_VideoBuffer;
  PLATFORM::CMutex m_Mutex;
  uint32_t m_MuxPacketSerial;
  sPtsWrap m_PtsWrap;
  uint16_t m_Error;
  bool m_SetRefTime;
  time_t m_refTime, m_endTime, m_wrapTime;
};

}
