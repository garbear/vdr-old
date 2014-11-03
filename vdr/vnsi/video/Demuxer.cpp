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

#include "Demuxer.h"
#include "VideoBuffer.h"
#include "Config.h"
#include "devices/DeviceManager.h"
#include "Streamer.h"
#include "timers/TimerManager.h"
#include "utils/log/Log.h"

#include <assert.h>
#include <libsi/si.h>

using namespace PLATFORM;

namespace VDR
{

cVNSIDemuxer::cVNSIDemuxer(int clientID, uint8_t timeshift) :
    m_clientID(clientID),
    m_timeshift(timeshift),
    m_WaitIFrame(true),
    m_FirstFramePTS(0),
    m_VideoBuffer(NULL),
    m_MuxPacketSerial(0),
    m_Error(0),
    m_SetRefTime(true),
    m_refTime(0),
    m_endTime(0),
    m_wrapTime(0)
{
  memset(&m_PtsWrap, 0, sizeof(sPtsWrap));
}

cVNSIDemuxer::~cVNSIDemuxer()
{
}

bool cVNSIDemuxer::Open(const ChannelPtr& channel, int serial)
{
  assert(channel.get());

  CLockObject lock(m_Mutex);

  bool recording = false;
  if (0) // test harness
  {
    recording = true;
    m_VideoBuffer = cVideoBuffer::Create("/home/xbmc/test.ts");
  }
  else if (serial == -1)
  {
    /* TODO
    cRecording* rec = cTimerManager::Get().GetActiveRecording(channel);
    if (rec)
    {
      recording = true;
      m_VideoBuffer = cVideoBuffer::Create(rec);
    }
    */
  }

  if (!recording)
  {
    m_VideoBuffer = cVideoBuffer::Create(m_clientID, m_timeshift);
  }

  if (!m_VideoBuffer)
    return false;

  if (!recording)
  {
    if (!(m_tunerHandle = cDeviceManager::Get().OpenVideoInput(m_VideoBuffer, channel)))
    {
      esyslog("Can't switch to channel %i - %s", channel->Number(), channel->Name().c_str());
      return false;
    }
  }

  m_CurrentChannel = channel;
  m_CurrentChannel->RegisterObserver(this);

  PidsChanged();

  return true;
}

void cVNSIDemuxer::Close()
{
  CLockObject lock(m_Mutex);

  if (m_VideoBuffer)
  {
    cDeviceManager::Get().CloseVideoInput(m_VideoBuffer);
    delete m_VideoBuffer;
    m_VideoBuffer = NULL;
    if (m_tunerHandle)
    {
      m_tunerHandle->Release(false);
      m_tunerHandle = cTunerHandle::EmptyHandle;
    }
  }

  if (m_CurrentChannel)
  {
    m_CurrentChannel->UnregisterObserver(this);
    m_CurrentChannel.reset();
  }
  m_WaitIFrame      = true;
  m_FirstFramePTS   = 0;
  m_MuxPacketSerial = 0;
  m_Error           = ERROR_DEMUX_NODATA;
  m_SetRefTime      = true;
  memset(&m_PtsWrap, 0, sizeof(sPtsWrap));

  for (std::list<cTSStream*>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
  {
    dsyslog("Deleting stream parser for pid=%i and type=%i", (*it)->GetPID(), (*it)->Type());
    delete (*it);
  }

  m_Streams.clear();
}

void cVNSIDemuxer::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageChannelChanged)
  {
    PidsChanged();
  }
}

bool cVNSIDemuxer::PidsChanged(void)
{
  std::set<uint16_t> pids = m_CurrentChannel->GetPids();
  if (pids != m_pids)
  {
    m_pids = pids;
    return EnsureParsers(m_CurrentChannel);
  }

  return false;
}

int cVNSIDemuxer::Read(sStreamPacket *packet)
{
  uint8_t *buf;
  int len;
  cTSStream *stream;

  assert(packet);
  assert(m_VideoBuffer);

  CLockObject lock(m_Mutex);

  // clear packet
  memset(packet, 0, sizeof(sStreamPacket));

  // read TS Packet from buffer
  len = m_VideoBuffer->Read(&buf, TS_SIZE, m_endTime, m_wrapTime);

  // eof
  if (len == VIDEOBUFFER_EOF)
    return VIDEOBUFFER_EOF;
  else if (len != TS_SIZE)
    return VIDEOBUFFER_NO_DATA;

  m_Error &= ~ERROR_DEMUX_NODATA;

  int ts_pid = TsPid(buf);

  /* TODO
  if (PidsChanged())
    packet->pmtChange = true;
  */

  if ((stream = FindStream(ts_pid)))
  {
    // pass to demux
    int error = stream->ProcessTSPacket(buf, packet, m_WaitIFrame);
    if (error == 0)
    {
      if (m_WaitIFrame)
      {
        if (packet->pts != DVD_NOPTS_VALUE)
          m_FirstFramePTS = packet->pts;
        m_WaitIFrame = false;
      }

      if (packet->pts < m_FirstFramePTS)
        return 0;


      packet->serial = m_MuxPacketSerial;
      if (m_SetRefTime)
      {
        m_refTime = m_VideoBuffer->GetRefTime();
        packet->reftime = m_refTime;
        m_SetRefTime = false;
      }
      return 1;
    }
    else if (error < 0)
    {
      m_Error |= abs(error);
    }
  }

  return 0;
}

bool cVNSIDemuxer::SeekTime(int64_t time)
{
  off_t pos, pos_min, pos_max, pos_limit, start_pos;
  int64_t ts, ts_min, ts_max, last_ts;
  int no_change;

  if (!m_VideoBuffer->HasBuffer())
    return false;

  CLockObject lock(m_Mutex);

//  isyslog("----- seek to time: %ld", time);

  // rescale to 90khz
  time = cTSStream::Rescale(time, 90000, DVD_TIME_BASE);

  m_VideoBuffer->GetPositions(&pos, &pos_min, &pos_max);

//  isyslog("----- seek to time: %ld", time);
//  isyslog("------pos: %ld, pos min: %ld, pos max: %ld", pos, pos_min, pos_max);

  if (!GetTimeAtPos(&pos_min, &ts_min))
  {
    ResetParsers();
    m_WaitIFrame = true;
    return false;
  }

//  isyslog("----time at min: %ld", ts_min);

  if (ts_min >= time)
  {
    m_VideoBuffer->SetPos(pos_min);
    ResetParsers();
    m_WaitIFrame = true;
    m_MuxPacketSerial++;
    return true;
  }

  int64_t timecur;
  GetTimeAtPos(&pos, &timecur);

  // get time at end of buffer
  unsigned int step= 1024;
  bool gotTime;
  do
  {
    pos_max -= step;
    gotTime = GetTimeAtPos(&pos_max, &ts_max);
    step += step;
  } while (!gotTime && pos_max >= step);

  if (!gotTime)
  {
    ResetParsers();
    m_WaitIFrame = true;
    return false;
  }

  if (ts_max <= time)
  {
    ResetParsers();
    m_WaitIFrame = true;
    m_MuxPacketSerial++;
    return true;
  }

//  isyslog(" - time in buffer: %ld", cTSStream::Rescale(ts_max-ts_min, DVD_TIME_BASE, 90000)/1000000);

  // bisect seek
  if(ts_min > ts_max)
  {
    ResetParsers();
    m_WaitIFrame = true;
    return false;
  }
  else if (ts_min == ts_max)
  {
    pos_limit = pos_min;
  }
  else
    pos_limit = pos_max;

  no_change = 0;
  ts = time;
  last_ts = 0;
  while (pos_min < pos_limit)
  {
    if (no_change==0)
    {
      // interpolate position
      pos = cTSStream::Rescale(time - ts_min, pos_max - pos_min, ts_max - ts_min)
          + pos_min - (pos_max - pos_limit);
    }
    else if (no_change==1)
    {
      // bisection, if interpolation failed to change min or max pos last time
      pos = (pos_min + pos_limit) >> 1;
    }
    else
    {
      // linear search if bisection failed
      pos = pos_min;
    }

    // clamp calculated pos into boundaries
    if( pos <= pos_min)
      pos = pos_min + 1;
    else if (pos > pos_limit)
      pos = pos_limit;
    start_pos = pos;

    // get time stamp at pos
    if (!GetTimeAtPos(&pos, &ts))
    {
      ResetParsers();
      m_WaitIFrame = true;
      return false;
    }
    pos = m_VideoBuffer->GetPosCur();

    // determine method for next calculation of pos
    if ((last_ts == ts) || (pos >= pos_max))
      no_change++;
    else
      no_change=0;

//    isyslog("--- pos: %ld, \t time: %ld, diff time: %ld", pos, ts, time-ts);

    // 0.4 sec is close enough
    if (abs(time - ts) <= 36000)
    {
      break;
    }
    // target is to the left
    else if (time <= ts)
    {
      pos_limit = start_pos - 1;
      pos_max = pos;
      ts_max = ts;
    }
    // target is to the right
    if (time >= ts)
    {
      pos_min = pos;
      ts_min = ts;
    }
    last_ts = ts;
  }

//  isyslog("----pos found: %ld", pos);
//  isyslog("----time at pos: %ld, diff time: %ld", ts, cTSStream::Rescale(timecur-ts, DVD_TIME_BASE, 90000));

  m_VideoBuffer->SetPos(pos);

  ResetParsers();
  m_WaitIFrame = true;
  m_MuxPacketSerial++;
  return true;
}

void cVNSIDemuxer::BufferStatus(bool &timeshift, uint32_t &start, uint32_t &end)
{
  timeshift = m_VideoBuffer->HasBuffer();

  if (timeshift)
  {
    if (!m_wrapTime)
    {
      start = m_refTime;
    }
    else
    {
      start = m_endTime - (m_wrapTime - m_refTime);
    }
    end = m_endTime;
  }
  else
  {
    start = 0;
    end = 0;
  }
}

cTSStream *cVNSIDemuxer::GetFirstStream()
{
  m_StreamsIterator = m_Streams.begin();
  if (m_StreamsIterator != m_Streams.end())
    return *m_StreamsIterator;
  else
    return NULL;
}

cTSStream *cVNSIDemuxer::GetNextStream()
{
  ++m_StreamsIterator;
  if (m_StreamsIterator != m_Streams.end())
    return *m_StreamsIterator;
  else
    return NULL;
}

cTSStream *cVNSIDemuxer::FindStream(int Pid)
{
  for (std::list<cTSStream*>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
  {
    if (Pid == (*it)->GetPID())
      return *it;
  }
  return NULL;
}

void cVNSIDemuxer::ResetParsers()
{
  for (std::list<cTSStream*>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
  {
    (*it)->ResetParser();
  }
}

bool cVNSIDemuxer::EnsureParsers(const ChannelPtr& channel)
{
  std::vector<sStreamInfo> streamInfos = GetStreamsFromChannel(m_CurrentChannel);

  bool streamChange = false;

  std::list<cTSStream*>::iterator it = m_Streams.begin();
  while (it != m_Streams.end())
  {
    std::vector<sStreamInfo>::iterator its;
    for (its = streamInfos.begin(); its != streamInfos.end(); ++its)
    {
      if ((its->pID == (*it)->GetPID()) && (its->type == (*it)->Type()))
      {
        break;
      }
    }
    if (its == streamInfos.end())
    {
      isyslog("Deleting stream for pid=%i and type=%i", (*it)->GetPID(), (*it)->Type());
      m_Streams.erase(it);
      it = m_Streams.begin();
      streamChange = true;
    }
    else
      ++it;
  }

  for (std::vector<sStreamInfo>::const_iterator it = streamInfos.begin(); it != streamInfos.end(); ++it)
  {
    cTSStream *stream = FindStream(it->pID);
    if (stream)
    {
      // TODO: check for change in lang
      stream->SetLanguage(it->language);
      continue;
    }

    if (it->type == stH264)
    {
      stream = new cTSStream(stH264, it->pID, &m_PtsWrap);
    }
    else if (it->type == stMPEG2VIDEO)
    {
      stream = new cTSStream(stMPEG2VIDEO, it->pID, &m_PtsWrap);
    }
    else if (it->type == stMPEG2AUDIO)
    {
      stream = new cTSStream(stMPEG2AUDIO, it->pID, &m_PtsWrap);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stAACADTS)
    {
      stream = new cTSStream(stAACADTS, it->pID, &m_PtsWrap);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stAACLATM)
    {
      stream = new cTSStream(stAACLATM, it->pID, &m_PtsWrap);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stAC3)
    {
      stream = new cTSStream(stAC3, it->pID, &m_PtsWrap);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stEAC3)
    {
      stream = new cTSStream(stEAC3, it->pID, &m_PtsWrap);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stDVBSUB)
    {
      stream = new cTSStream(stDVBSUB, it->pID, &m_PtsWrap);
      stream->SetLanguage(it->language);
      stream->SetSubtitlingDescriptor(it->subtitlingType, it->compositionPageId, it->ancillaryPageId);
    }
    else if (it->type == stTELETEXT)
    {
      stream = new cTSStream(stTELETEXT, it->pID, &m_PtsWrap);
    }
    else
      continue;

    m_Streams.push_back(stream);
    isyslog("Created stream for pid=%i and type=%i", stream->GetPID(), stream->Type());
    streamChange = true;
  }

  return streamChange;
}

std::vector<sStreamInfo> cVNSIDemuxer::GetStreamsFromChannel(const ChannelPtr& channel)
{
  std::vector<sStreamInfo> streamInfos;

  if (channel->GetVideoStream().vpid)
  {
    sStreamInfo newStream = { };
    newStream.pID = channel->GetVideoStream().vpid;
    if (channel->GetVideoStream().vtype == STREAM_TYPE_14496_H264_VIDEO)
      newStream.type = stH264;
    else
      newStream.type = stMPEG2VIDEO;

    streamInfos.push_back(newStream);
  }

  const std::vector<DataStream>& dataStreams = channel->GetDataStreams();
  for (std::vector<DataStream>::const_iterator it = dataStreams.begin(); it != dataStreams.end(); ++it)
  {
    if (!FindStream(it->dpid))
    {
      sStreamInfo newStream = { };
      newStream.pID = it->dpid;
      newStream.type = stAC3;
      if (it->dtype == (STREAM_TYPE)SI::EnhancedAC3DescriptorTag) // TODO: STREAM_TYPE or DESCRIPTOR_TAG?
        newStream.type = stEAC3;
      newStream.SetLanguage(it->dlang.c_str());
      streamInfos.push_back(newStream);
    }
  }

  const std::vector<AudioStream>& audioStreams = channel->GetAudioStreams();
  for (std::vector<AudioStream>::const_iterator it = audioStreams.begin(); it != audioStreams.end(); ++it)
  {
    if (!FindStream(it->apid))
    {
      sStreamInfo newStream = { };
      newStream.pID = it->apid;
      newStream.type = stMPEG2AUDIO;
      if (it->atype == STREAM_TYPE_13818_AUDIO_ADTS)
        newStream.type = stAACADTS;
      else if (it->atype == STREAM_TYPE_14496_AUDIO_LATM)
        newStream.type = stAACLATM;
      newStream.SetLanguage(it->alang.c_str());
      streamInfos.push_back(newStream);
    }
  }

  const std::vector<SubtitleStream>& subtitleStreams = channel->GetSubtitleStreams();
  for (std::vector<SubtitleStream>::const_iterator it = subtitleStreams.begin(); it != subtitleStreams.end(); ++it)
  {
    if (!FindStream(it->spid))
    {
      sStreamInfo newStream = { };
      newStream.pID = it->spid;
      newStream.type = stDVBSUB;
      newStream.SetLanguage(it->slang.c_str());
      newStream.subtitlingType = it->subtitlingType;
      newStream.compositionPageId = it->compositionPageId;
      newStream.ancillaryPageId = it->ancillaryPageId;
      streamInfos.push_back(newStream);
    }
  }

  if (channel->GetTeletextStream().tpid)
  {
    sStreamInfo newStream = { };
    newStream.pID = channel->GetTeletextStream().tpid;
    newStream.type = stTELETEXT;
    streamInfos.push_back(newStream);
  }

  return streamInfos;
}

bool cVNSIDemuxer::GetTimeAtPos(off_t *pos, int64_t *time)
{
  uint8_t *buf;
  int len;
  cTSStream *stream;
  int ts_pid;

  m_VideoBuffer->SetPos(*pos);
  ResetParsers();
  while ((len = m_VideoBuffer->Read(&buf, TS_SIZE, m_endTime, m_wrapTime)) == TS_SIZE)
  {
    ts_pid = TsPid(buf);
    if ((stream = FindStream(ts_pid)))
    {
      // only consider video or audio streams
      if ((stream->Content() == scVIDEO || stream->Content() == scAUDIO) &&
          stream->ReadTime(buf, time))
      {
        return true;
      }
    }
  }
  return false;
}

uint16_t cVNSIDemuxer::GetError()
{
  uint16_t ret = m_Error;
  m_Error = ERROR_DEMUX_NODATA;
  return ret;
}

}
