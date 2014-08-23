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

#include "Streamer.h"
#include "VideoBuffer.h"
#include "vnsi/net/VNSICommand.h"
#include "vnsi/net/ResponsePacket.h"
#include "channels/ChannelManager.h"
#include "devices/DeviceManager.h"
#include "recordings/Recordings.h"
#include "settings/Settings.h"
#include "timers/Timers.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/XSocket.h"
#include "Config.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>

namespace VDR
{

// --- cLiveStreamer -------------------------------------------------

cLiveStreamer::cLiveStreamer(int clientID, uint8_t timeshift, uint32_t timeout)
 : m_ClientID(clientID)
 , m_scanTimeout(timeout)
{
  m_Channel         = cChannel::EmptyChannel;
  m_Socket          = NULL;
  m_IsAudioOnly     = false;
  m_IsMPEGPS        = false;
  m_startup         = true;
  m_SignalLost      = false;
  m_IFrameSeen      = false;
  m_VideoBuffer     = NULL;
  m_Timeshift       = timeshift;

  if(m_scanTimeout == 0)
    m_scanTimeout = cSettings::Get().m_StreamTimeout;
}

cLiveStreamer::~cLiveStreamer()
{
  dsyslog("Started to delete live streamer");

  StopThread(5000);
  Close();

  dsyslog("Finished to delete live streamer");
}

bool cLiveStreamer::Open(int serial)
{
  Close();

  bool recording = false;
  if (0) // test harness
  {
    recording = true;
    m_VideoBuffer = cVideoBuffer::Create("/home/xbmc/test.ts");
  }
  else if (serial == -1)
  {
    cRecording* rec = cTimers::Get().GetActiveRecording(m_Channel);
    if (rec)
    {
      recording = true;
      m_VideoBuffer = cVideoBuffer::Create(rec);
    }
  }

  if (!recording)
  {
    m_VideoBuffer = cVideoBuffer::Create(m_ClientID, m_Timeshift);
  }

  if (!m_VideoBuffer)
    return false;

  if (!recording)
  {
    if (!cDeviceManager::Get().OpenVideoInput(m_VideoBuffer, m_Channel))
    {
      esyslog("Can't switch to channel %i - %s", m_Channel->Number(), m_Channel->Name().c_str());
      return false;
    }
  }

  m_Demuxer.Open(m_Channel, m_VideoBuffer);
  if (serial >= 0)
    m_Demuxer.SetSerial(serial);

  return true;
}

void cLiveStreamer::Close(void)
{
  isyslog("LiveStreamer::Close - close");
  cDeviceManager::Get().CloseVideoInput(m_VideoBuffer);
  m_Demuxer.Close();

  if (m_VideoBuffer)
  {
    delete m_VideoBuffer;
    m_VideoBuffer = NULL;
  }
}

void* cLiveStreamer::Process(void)
{
  int ret;
  sStreamPacket pkt;
  memset(&pkt, 0, sizeof(sStreamPacket));
  bool requestStreamChange = false;
  cTimeMs last_info(1000);
  cTimeMs bufferStatsTimer(1000);

  while (!IsStopped())
  {
    ret = m_Demuxer.Read(&pkt);
    if (ret > 0)
    {
      if (pkt.pmtChange)
      {
        requestStreamChange = true;
      }
      if (pkt.data)
      {
        if (pkt.streamChange || requestStreamChange)
          sendStreamChange();
        requestStreamChange = false;
        if (pkt.reftime)
        {
          sendRefTime(&pkt);
          pkt.reftime = 0;
        }
        sendStreamPacket(&pkt);
      }

      // send signal info every 10 sec.
      if(last_info.Elapsed() >= 10*1000)
      {
        last_info.Set(0);
        sendSignalInfo();
      }

      // send buffer stats
      if(bufferStatsTimer.TimedOut())
      {
        sendBufferStatus();
        bufferStatsTimer.Set(1000);
      }
    }
    else if (ret == VIDEOBUFFER_NO_DATA)
    {
      // no data
      usleep(10000);
      if(m_last_tick.Elapsed() >= (uint64_t)(m_scanTimeout*1000))
      {
        sendStreamStatus();
        m_last_tick.Set(0);
        m_SignalLost = true;
      }
    }
    else if (ret == VIDEOBUFFER_EOF)
    {
      if (!Open(m_Demuxer.GetSerial()))
      {
        m_Socket->Shutdown();
        break;
      }
    }
  }
  isyslog("exit streamer thread");
  return NULL;
}

bool cLiveStreamer::StreamChannel(ChannelPtr channel, cxSocket *Socket, cResponsePacket *resp)
{
  if (channel == cChannel::EmptyChannel)
  {
    esyslog("cannot start streaming without a valid channel");
    return false;
  }

  m_Channel   = channel;
  m_Socket    = Socket;

  if (!Open())
    return false;

  // Send the OK response here, that it is before the Stream end message
  resp->add_U32(VNSI_RET_OK);
  resp->finalise();
  m_Socket->write(resp->getPtr(), resp->getLen());

  Activate(true);

  isyslog("Successfully switched to channel %i - %s", m_Channel->Number(), m_Channel->Name().c_str());
  return true;
}

inline void cLiveStreamer::Activate(bool On)
{
  if (On)
  {
    dsyslog("VDR active, sending stream start message");
    CreateThread();
  }
  else
  {
    dsyslog("VDR inactive, sending stream end message");
    StopThread(5000);
  }
}

void cLiveStreamer::sendStreamPacket(sStreamPacket *pkt)
{
  if(pkt == NULL)
    return;

  if(pkt->size == 0)
    return;

  if (!m_streamHeader.initStream(VNSI_STREAM_MUXPKT, pkt->id, pkt->duration, pkt->pts, pkt->dts, pkt->serial))
  {
    esyslog("stream response packet init fail");
    return;
  }
  m_streamHeader.setLen(m_streamHeader.getStreamHeaderLength() + pkt->size);
  m_streamHeader.finaliseStream();

  m_Socket->LockWrite();
  m_Socket->write(m_streamHeader.getPtr(), m_streamHeader.getStreamHeaderLength(), -1, true);
  m_Socket->write(pkt->data, pkt->size);
  m_Socket->UnlockWrite();

  m_last_tick.Set(0);
  m_SignalLost = false;
}

void cLiveStreamer::sendStreamChange()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_CHANGE, 0, 0, 0, 0, 0))
  {
    esyslog("stream response packet init fail");
    delete resp;
    return;
  }

  uint32_t FpsScale, FpsRate, Height, Width;
  double Aspect;
  uint32_t Channels, SampleRate, BitRate, BitsPerSample, BlockAlign;
  for (cTSStream* stream = m_Demuxer.GetFirstStream(); stream; stream = m_Demuxer.GetNextStream())
  {
    resp->add_U32(stream->GetPID());
    if (stream->Type() == stMPEG2AUDIO)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("MPEG2AUDIO");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stMPEG2VIDEO)
    {
      stream->GetVideoInformation(FpsScale, FpsRate, Height, Width, Aspect);
      resp->add_String("MPEG2VIDEO");
      resp->add_U32(FpsScale);
      resp->add_U32(FpsRate);
      resp->add_U32(Height);
      resp->add_U32(Width);
      resp->add_double(Aspect);
    }
    else if (stream->Type() == stAC3)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("AC3");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stH264)
    {
      stream->GetVideoInformation(FpsScale, FpsRate, Height, Width, Aspect);
      resp->add_String("H264");
      resp->add_U32(FpsScale);
      resp->add_U32(FpsRate);
      resp->add_U32(Height);
      resp->add_U32(Width);
      resp->add_double(Aspect);
    }
    else if (stream->Type() == stDVBSUB)
    {
      resp->add_String("DVBSUB");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(stream->CompositionPageId());
      resp->add_U32(stream->AncillaryPageId());
    }
    else if (stream->Type() == stTELETEXT)
    {
      resp->add_String("TELETEXT");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(stream->CompositionPageId());
      resp->add_U32(stream->AncillaryPageId());
    }
    else if (stream->Type() == stAACADTS)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("AAC");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stAACLATM)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("AAC_LATM");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stEAC3)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("EAC3");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stDTS)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("DTS");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

bool IsFrontendFound(void) { return false; } // TODO

void cLiveStreamer::sendSignalInfo()
{
  // If no frontend is found, return an empty signalinfo package
  bool bFrontendFound = false; // TODO
  if (!bFrontendFound)
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStream(VNSI_STREAM_SIGNALINFO, 0, 0, 0, 0, 0))
    {
      esyslog("stream response packet init fail");
      delete resp;
      return;
    }

    resp->add_String(StringUtils::Format("Unknown"));
    resp->add_String(StringUtils::Format("Unknown"));
    resp->add_U32(0);
    resp->add_U32(0);
    resp->add_U32(0);
    resp->add_U32(0);

    resp->finaliseStream();
    m_Socket->write(resp->getPtr(), resp->getLen());
    delete resp;
  }
  /* TODO
  else
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStream(VNSI_STREAM_SIGNALINFO, 0, 0, 0, 0, 0))
    {
      esyslog("stream response packet init fail");
      delete resp;
      return;
    }

    fe_status_t status;
    uint16_t fe_snr;
    uint16_t fe_signal;
    uint32_t fe_ber;
    uint32_t fe_unc;

    if (ioctl(m_Frontend, FE_READ_SIGNAL_STRENGTH, &fe_signal) == -1)
      fe_signal = -2;
    if (ioctl(m_Frontend, FE_READ_SNR, &fe_snr) == -1)
      fe_snr = -2;
    if (ioctl(m_Frontend, FE_READ_BER, &fe_ber) == -1)
      fe_ber = -2;
    if (ioctl(m_Frontend, FE_READ_UNCORRECTED_BLOCKS, &fe_unc) == -1)
      fe_unc = -2;

    if (m_Channel->GetTransponder().IsSatellite())
      resp->add_String(StringUtils::Format("DVB-S%s #%d - %s", (m_FrontendInfo.caps & 0x10000000) ? "2" : "",  m_Device->Index(), m_FrontendInfo.name));
    else if (m_Channel->GetTransponder().IsCable())
      resp->add_String(StringUtils::Format("DVB-C #%d - %s", m_Device->Index(), m_FrontendInfo.name));
    else if (m_Channel->GetTransponder().IsTerrestrial())
      resp->add_String(StringUtils::Format("DVB-T #%d - %s", m_Device->Index(), m_FrontendInfo.name));

    resp->add_String(StringUtils::Format("%s:%s:%s:%s:%s", (status & FE_HAS_LOCK) ? "LOCKED" : "-", (status & FE_HAS_SIGNAL) ? "SIGNAL" : "-", (status & FE_HAS_CARRIER) ? "CARRIER" : "-", (status & FE_HAS_VITERBI) ? "VITERBI" : "-", (status & FE_HAS_SYNC) ? "SYNC" : "-"));
    resp->add_U32(fe_snr);
    resp->add_U32(fe_signal);
    resp->add_U32(fe_ber);
    resp->add_U32(fe_unc);

    resp->finaliseStream();
    m_Socket->write(resp->getPtr(), resp->getLen());
    delete resp;
  }
  */
}

void cLiveStreamer::sendStreamStatus()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_STATUS, 0, 0, 0, 0, 0))
  {
    esyslog("stream response packet init fail");
    delete resp;
    return;
  }
  uint16_t error = m_Demuxer.GetError();
  if (error & ERROR_PES_SCRAMBLE)
  {
    isyslog("Channel: scrambled %d", error);
    resp->add_String(StringUtils::Format("Channel: scrambled (%d)", error));
  }
  else if (error & ERROR_PES_STARTCODE)
  {
    isyslog("Channel: startcode %d", error);
    resp->add_String(StringUtils::Format("Channel: encrypted? (%d)", error));
  }
  else if (error & ERROR_DEMUX_NODATA)
  {
    isyslog("Channel: no data %d", error);
    resp->add_String(StringUtils::Format("Channel: no data"));
  }
  else
  {
    isyslog("Channel: unknown error %d", error);
    resp->add_String(StringUtils::Format("Channel: unknown error (%d)", error));
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cLiveStreamer::sendBufferStatus()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_BUFFERSTATS, 0, 0, 0, 0, 0))
  {
    esyslog("stream response packet init fail");
    delete resp;
    return;
  }
  uint32_t start, end;
  bool timeshift;
  m_Demuxer.BufferStatus(timeshift, start, end);
  resp->add_U8(timeshift);
  resp->add_U32(start);
  resp->add_U32(end);
  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cLiveStreamer::sendRefTime(sStreamPacket *pkt)
{
  if(pkt == NULL)
    return;

  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_REFTIME, 0, 0, 0, 0, 0))
  {
    esyslog("stream response packet init fail");
    delete resp;
    return;
  }

  resp->add_U32(pkt->reftime);
  resp->add_U64(pkt->pts);
  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

bool cLiveStreamer::SeekTime(int64_t time, uint32_t &serial)
{
  bool ret = m_Demuxer.SeekTime(time);
  serial = m_Demuxer.GetSerial();
  return ret;
}

}
