/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "DevicePlayerSubsystem.h"
#include "DeviceReceiverSubsystem.h"
#include "DeviceSPUSubsystem.h"
#include "DeviceTrackSubsystem.h"
#include "DeviceVideoFormatSubsystem.h"
#include "devices/Device.h"
#include "devices/Transfer.h"
#include "Player.h"
#include "utils/log/Log.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) \
  do \
  { \
    delete p; p = NULL; \
  } while (0)
#endif

using namespace std;

namespace VDR
{

// The minimum number of unknown PS1 packets to consider this a "pre 1.3.19 private stream":
#define MIN_PRE_1_3_19_PRIVATESTREAM  10

#define MAGIC_NUMBER 0x47

cDevicePlayerSubsystem::cDevicePlayerSubsystem(cDevice *device)
 : cDeviceSubsystem(device),
   m_player(NULL),
   m_bIsPlayingVideo(false)
{
}

cDevicePlayerSubsystem::~cDevicePlayerSubsystem()
{
  Detach(m_player);
  //Receiver()->DetachAllReceivers(); TODO: Must not call subsystems in destructor!!!!!!!!!!!
}

void cDevicePlayerSubsystem::Clear()
{
}

void cDevicePlayerSubsystem::Play()
{
}

void cDevicePlayerSubsystem::Freeze()
{
}

void cDevicePlayerSubsystem::Mute()
{
}

void cDevicePlayerSubsystem::StillPicture(const vector<uint8_t> &data)
{
  assert(!data.empty());

  if (data[0] == MAGIC_NUMBER)
  {
    // TS data
    cTsToPes TsToPes;
    uint8_t *buf = NULL;
    const uint8_t *dataptr = data.data();
    size_t length = data.size();
    int Size = 0;
    while (length >= TS_SIZE)
    {
      int Pid = TsPid(dataptr);
      if (Pid == PATPID)
        m_patPmtParser.ParsePat(dataptr, TS_SIZE);
      else if (m_patPmtParser.IsPmtPid(Pid))
        m_patPmtParser.ParsePmt(dataptr, TS_SIZE);
      else if (Pid == m_patPmtParser.Vpid())
      {
        if (TsPayloadStart(dataptr))
        {
          int l;
          while (const uint8_t *p = TsToPes.GetPes(l))
          {
            int Offset = Size;
            int NewSize = Size + l;
            if (uint8_t *NewBuffer = (uint8_t*)realloc(buf, NewSize))
            {
              Size = NewSize;
              buf = NewBuffer;
              memcpy(buf + Offset, p, l);
            }
            else
            {
              LOG_ERROR_STR("out of memory");
              free(buf);
              return;
            }
          }
          TsToPes.Reset();
        }
        TsToPes.PutTs(dataptr, TS_SIZE);
      }
      length -= TS_SIZE;
      dataptr += TS_SIZE;
    }
    int l;
    while (const uint8_t *p = TsToPes.GetPes(l))
    {
      int Offset = Size;
      int NewSize = Size + l;
      if (uint8_t *NewBuffer = (uint8_t*)realloc(buf, NewSize))
      {
        Size = NewSize;
        buf = NewBuffer;
        memcpy(buf + Offset, p, l);
      }
      else
      {
        esyslog("ERROR: out of memory");
        free(buf);
        return;
      }
    }
    if (buf)
    {
      vector<uint8_t> vec(buf, buf + Size);
      free(buf);
      StillPicture(vec);
    }
  }
}

int cDevicePlayerSubsystem::PlayPes(const vector<uint8_t> &data, bool bVideoOnly /* = false */)
{
  if (data.empty())
  {
    return 0;
  }
  int i = 0;
  while (i <= data.size() - 6)
  {
    if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01)
    {
      int length = PesLength(data.data() + i);
      if (i + length > data.size())
      {
        esyslog("ERROR: incomplete PES packet!");
        return data.size();
      }
      vector<uint8_t> vecData(data.data() + i, data.data() + i + length);
      int w = PlayPesPacket(vecData, bVideoOnly);
      if (w > 0)
        i += length;
      else
        return i == 0 ? w : i;
    }
    else
      i++;
  }
  if (i < data.size())
    esyslog("ERROR: leftover PES data!");
  return data.size();
}

//TODO detect and report continuity errors?
int cDevicePlayerSubsystem::PlayTs(const vector<uint8_t> &data, bool bVideoOnly /* = false */)
{
  unsigned int Played = 0;
  if (data.empty())
  {
    m_tsToPesVideo.Reset();
    m_tsToPesAudio.Reset();
    m_tsToPesSubtitle.Reset();
  }
  else if (data.size() < TS_SIZE)
  {
    esyslog("ERROR: skipped %d bytes of TS fragment", (int)data.size());
    return data.size();
  }
  else
  {
    size_t length = data.size();
    const uint8_t *dataptr = data.data();
    while (length >= TS_SIZE)
    {
      vector<uint8_t> vecData(dataptr, dataptr + TS_SIZE);

      if (vecData[0] != TS_SYNC_BYTE)
      {
        unsigned int Skipped = 1;
        while (Skipped < length && (vecData[Skipped] != TS_SYNC_BYTE || (length - Skipped > TS_SIZE && vecData[Skipped + TS_SIZE] != TS_SYNC_BYTE)))
          Skipped++;
        esyslog("ERROR: skipped %d bytes to sync on start of TS packet", Skipped);
        return Played + Skipped;
      }

      int Pid = TsPid(vecData.data());

      if (TsHasPayload(dataptr))
      { // silently ignore TS packets w/o payload
        int PayloadOffset = TsPayloadOffset(vecData.data());
        if (PayloadOffset < TS_SIZE)
        {
          if (Pid == PATPID)
            m_patPmtParser.ParsePat(vecData.data(), vecData.size());
          else if (m_patPmtParser.IsPmtPid(Pid))
            m_patPmtParser.ParsePmt(vecData.data(), vecData.size());
          else if (Pid == m_patPmtParser.Vpid())
          {
            m_bIsPlayingVideo = true;
            int w = PlayTsVideo(vecData);
            if (w < 0)
              return Played ? Played : w;
            if (w == 0)
              break;
          }
          else if (Pid == Track()->m_availableTracks[Track()->m_currentAudioTrack].id)
          {
            if (!bVideoOnly || HasIBPTrickSpeed())
            {
              int w = PlayTsAudio(vecData);
              if (w < 0)
                return Played ? Played : w;
              if (w == 0)
                break;
            }
          }
          else if (Pid == Track()->m_availableTracks[Track()->m_currentSubtitleTrack].id)
          {
            if (!bVideoOnly || HasIBPTrickSpeed())
              PlayTsSubtitle(vecData);
          }
        }
      }
      else if (Pid == m_patPmtParser.Ppid())
      {
        int w = PlayTsVideo(vecData);
        if (w < 0)
          return Played ? Played : w;
        if (w == 0)
          break;
      }
      Played += TS_SIZE;
      length -= TS_SIZE;
      dataptr += TS_SIZE;
    }
  }
  return Played;
}

//bool cDevicePlayerSubsystem::Transferring() const
//{
//  return cTransferControl::ReceiverDevice() != NULL;
//}

//void cDevicePlayerSubsystem::StopReplay()
//{
//  if (m_player)
//  {
//    Detach(m_player);
//    if (Device()->IsPrimaryDevice())
//      cControl::Shutdown();
//  }
//}

//bool cDevicePlayerSubsystem::AttachPlayer(cPlayer *player)
//{
//  if (CanReplay())
//  {
//    if (m_player)
//      Detach(m_player);
//    m_patPmtParser.Reset();
//    m_player = player;
//    if (!Transferring())
//      Track()->ClrAvailableTracks(false, true);
//    SetPlayMode(m_player->playMode);
//    m_player->device = Device();
//    m_player->Activate(true);
//    return true;
//  }
//  return false;
//}

void cDevicePlayerSubsystem::Detach(cPlayer *player)
{
  if (player && m_player == player)
  {
    cPlayer *p = m_player;
    m_player = NULL; // avoids recursive calls to Detach()
    p->Activate(false);
    p->device = NULL;
    PLATFORM::CLockObject lock(Track()->m_mutexCurrentSubtitleTrack);
    SetPlayMode(pmNone);
//XXX    VideoFormat()->SetVideoDisplayFormat(cSettings::Get().m_VideoDisplayFormat);
    PlayTs(vector<uint8_t>());
    m_patPmtParser.Reset();
    m_bIsPlayingVideo = false;
  }
}

bool cDevicePlayerSubsystem::CanReplay() const
{
  return Device()->HasDecoder();
}

int cDevicePlayerSubsystem::PlaySubtitle(const vector<uint8_t> &data)
{
  return -1; //XXX remove this method
}

unsigned int cDevicePlayerSubsystem::PlayPesPacket(const vector<uint8_t> &data, bool bVideoOnly /* = false */)
{
  bool FirstLoop = true;
  uint8_t c = data[3];
  const uint8_t *Start = data.data();
  const uint8_t *End = Start + data.size();
  while (Start < End)
  {
    int d = End - Start;
    int w = d;
    vector<uint8_t> vecStart(Start, Start + d);
    switch (c)
    {
    case 0xBE:          // padding stream, needed for MPEG1
    case 0xE0 ... 0xEF: // video
      m_bIsPlayingVideo = true;
      w = PlayVideo(vecStart);
      break;
    case 0xC0 ... 0xDF: // audio
      Track()->SetAvailableTrack(ttAudio, c - 0xC0, c);
      if ((!bVideoOnly || HasIBPTrickSpeed()) && c == Track()->m_availableTracks[Track()->m_currentAudioTrack].id)
      {
        w = PlayAudio(vecStart, c);
      }
      break;
    case 0xBD: { // private stream 1
      int PayloadOffset = data[8] + 9;

      // Compatibility mode for old subtitles plugin:
      if ((data[7] & 0x01) && (data[PayloadOffset - 3] & 0x81) == 0x01 && data[PayloadOffset - 2] == 0x81)
        PayloadOffset--;

      uint8_t SubStreamId = data[PayloadOffset];
      uint8_t SubStreamType = SubStreamId & 0xF0;
      uint8_t SubStreamIndex = SubStreamId & 0x1F;

      // Compatibility mode for old VDR recordings, where 0xBD was only AC3:
pre_1_3_19_PrivateStreamDetected:
      if (Track()->m_pre_1_3_19_PrivateStream > MIN_PRE_1_3_19_PRIVATESTREAM)
      {
        SubStreamId = c;
        SubStreamType = 0x80;
        SubStreamIndex = 0;
      }
      else if (Track()->m_pre_1_3_19_PrivateStream)
        Track()->m_pre_1_3_19_PrivateStream--; // every known PS1 packet counts down towards 0 to recover from glitches...
        switch (SubStreamType) {
        case 0x20: // SPU
        case 0x30: // SPU
          Track()->SetAvailableTrack(ttSubtitle, SubStreamIndex, SubStreamId);
          if ((!bVideoOnly || HasIBPTrickSpeed()) &&
              Track()->m_currentSubtitleTrack != ttNone &&
              SubStreamId == Track()->m_availableTracks[Track()->m_currentSubtitleTrack].id)
            w = PlaySubtitle(vecStart);
          break;
        case 0x80: // AC3 & DTS
          if (true /*XXX g_setup.UseDolbyDigital*/)
          {
            Track()->SetAvailableTrack(ttDolby, SubStreamIndex, SubStreamId);
            if ((!bVideoOnly || HasIBPTrickSpeed()) && SubStreamId == Track()->m_availableTracks[Track()->m_currentAudioTrack].id)
            {
              w = PlayAudio(vecStart, SubStreamId);
            }
          }
          break;
        case 0xA0: // LPCM
          Track()->SetAvailableTrack(ttAudio, SubStreamIndex, SubStreamId);
          if ((!bVideoOnly || HasIBPTrickSpeed()) && SubStreamId == Track()->m_availableTracks[Track()->m_currentAudioTrack].id)
          {
            w = PlayAudio(vecStart, SubStreamId);
          }
          break;
        default:
          // Compatibility mode for old VDR recordings, where 0xBD was only AC3:
          if (Track()->m_pre_1_3_19_PrivateStream <= MIN_PRE_1_3_19_PRIVATESTREAM)
          {
            dsyslog("unknown PS1 packet, substream id = %02X (counter is at %d)", SubStreamId, Track()->m_pre_1_3_19_PrivateStream);
            Track()->m_pre_1_3_19_PrivateStream += 2; // ...and every unknown PS1 packet counts up (the very first one counts twice, but that's ok)
            if (Track()->m_pre_1_3_19_PrivateStream > MIN_PRE_1_3_19_PRIVATESTREAM)
            {
              dsyslog("switching to pre 1.3.19 Dolby Digital compatibility mode - substream id = %02X", SubStreamId);
              Track()->ClrAvailableTracks();
              Track()->m_pre_1_3_19_PrivateStream = MIN_PRE_1_3_19_PRIVATESTREAM + 1;
              goto pre_1_3_19_PrivateStreamDetected;
            }
          }
        }
      }
      break;
    default:
      //esyslog("ERROR: unexpected packet id %02X", c);
      break;
    }
    if (w > 0)
      Start += w;
    else
    {
      if (Start != data.data())
        esyslog("ERROR: incomplete PES packet write!");
      return Start == data.data() ? w : Start - data.data();
    }
    FirstLoop = false;
  }
  return data.size();
}

int cDevicePlayerSubsystem::PlayTsVideo(const vector<uint8_t> &data)
{
  // Video PES has no explicit length, so we can only determine the end of
  // a PES packet when the next TS packet that starts a payload comes in:
  if (TsPayloadStart(data.data()))
  {
    int length;
    while (const uint8_t *p = m_tsToPesVideo.GetPes(length))
    {
      vector<uint8_t> vecP(p, p + length);
      int w = PlayVideo(vecP);
      if (w <= 0)
      {
        m_tsToPesVideo.SetRepeatLast();
        return w;
      }
    }
    m_tsToPesVideo.Reset();
  }
  m_tsToPesVideo.PutTs(data.data(), data.size());
  return data.size();
}

int cDevicePlayerSubsystem::PlayTsAudio(const vector<uint8_t> &data)
{
  // Audio PES always has an explicit length and consists of single packets:
  int length;
  if (const uint8_t *p = m_tsToPesAudio.GetPes(length))
  {
    vector<uint8_t> vecP(p, p + length);
    int w = PlayAudio(vecP, vecP[3]);
    if (w <= 0)
    {
      m_tsToPesAudio.SetRepeatLast();
      return w;
    }
    m_tsToPesAudio.Reset();
  }
  m_tsToPesAudio.PutTs(data.data(), data.size());
  return data.size();
}

int cDevicePlayerSubsystem::PlayTsSubtitle(const vector<uint8_t> &data)
{
  return -1; //XXX remove this method
}

}
