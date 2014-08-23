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

#include "Player.h"
#include "devices/DeviceManager.h"
#include "utils/I18N.h"
#include "utils/log/Log.h"

using namespace std;
using namespace PLATFORM;

namespace VDR
{

// --- cPlayer ---------------------------------------------------------------

cPlayer::cPlayer(ePlayMode PlayMode)
{
  device = NULL;
  playMode = PlayMode;
}

cPlayer::~cPlayer()
{
  Detach();
}

int cPlayer::PlayPes(const uint8_t *Data, int Length, bool VideoOnly /* = false */)
{
  vector<uint8_t> data;
  if (Data && Length)
  {
    data.reserve(Length);
    for (int ptr = 0; ptr < Length; ptr++)
      data.push_back(Data[ptr]);
  }
  return PlayPes(data, VideoOnly);
}

int cPlayer::PlayPes(const vector<uint8_t> &data, bool VideoOnly /* = false */)
{
  if (device)
     return device->Player()->PlayPes(data, VideoOnly);
  esyslog("ERROR: attempt to use cPlayer::PlayPes() without attaching to a cDevice!");
  return -1;
}

int cPlayer::PlayTs(const uint8_t *Data, int Length, bool VideoOnly /* = false */)
{
  if (Data && Length)
  {
    vector<uint8_t> data(Data, Data + Length);
    return PlayTs(data, VideoOnly);
  }
  else
  {
    vector<uint8_t> empty;
    return PlayTs(empty, VideoOnly);
  }
}

int cPlayer::PlayTs(const vector<uint8_t> &data, bool VideoOnly /* = false */)
{
  return device ? device->Player()->PlayTs(data, VideoOnly) : -1;
}

void cPlayer::Detach(void)
{
  if (device)
     device->Player()->Detach(this);
}

void cPlayer::DeviceClrAvailableTracks(bool DescriptionsOnly /* = false */)
{
  if (device) device->Track()->ClrAvailableTracks(DescriptionsOnly);
}

bool cPlayer::DeviceSetAvailableTrack(eTrackType Type, int Index, uint16_t Id, const char *Language /* = NULL */, const char *Description /* = NULL*/)
{
  if (device)
    return device->Track()->SetAvailableTrack(Type, Index, Id, Language, Description);
  return false;
}

bool cPlayer::DeviceSetCurrentAudioTrack(eTrackType Type)
{
  return device ? device->Track()->SetCurrentAudioTrack(Type) : false;
}

bool cPlayer::DeviceSetCurrentSubtitleTrack(eTrackType Type)
{
  return device ? device->Track()->SetCurrentSubtitleTrack(Type) : false;
}

bool cPlayer::DevicePoll(cPoller &Poller, int TimeoutMs /* = 0 */)
{
  return device ? device->Player()->Poll(Poller, TimeoutMs) : false;
}

bool cPlayer::DeviceFlush(int TimeoutMs /* = 0 */)
{
  return device ? device->Player()->Flush(TimeoutMs) : true;
}

bool cPlayer::DeviceHasIBPTrickSpeed(void)
{
  return device ? device->Player()->HasIBPTrickSpeed() : false;
}

bool cPlayer::DeviceIsPlayingVideo(void)
{
  return device ? device->Player()->IsPlayingVideo() : false;
}

void cPlayer::DeviceTrickSpeed(int Speed)
{
  if (device) device->Player()->TrickSpeed(Speed);
}

void cPlayer::DeviceClear(void)
{
  if (device) device->Player()->Clear();
}

void cPlayer::DevicePlay(void)
{
  if (device) device->Player()->Play();
}

void cPlayer::DeviceFreeze(void)
{
  if (device) device->Player()->Freeze();
}
void cPlayer::DeviceMute(void)
{
  if (device) device->Player()->Mute();
}

void cPlayer::DeviceSetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat)
{
  if (device) device->VideoFormat()->SetVideoDisplayFormat(VideoDisplayFormat);
}

void cPlayer::DeviceStillPicture(const vector<uint8_t> &data)
{
  if (device) device->Player()->StillPicture(data);
}

uint64_t cPlayer::DeviceGetSTC(void)
{
  return device ? device->Player()->GetSTC() : -1;
}

}
