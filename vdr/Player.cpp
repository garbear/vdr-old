/*
 * player.c: The basic player interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: player.c 2.2 2012/04/28 11:52:50 kls Exp $
 */

#include "Player.h"
#include "devices/DeviceManager.h"
#include "utils/I18N.h"

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

// --- cControl --------------------------------------------------------------

cControl *cControl::control = NULL;
CMutex cControl::mutex;

cControl::cControl(cPlayer *Player, bool Hidden)
{
  attached = false;
  hidden = Hidden;
  player = Player;
}

cControl::~cControl()
{
  if (this == control)
     control = NULL;
}

cOsdObject *cControl::GetInfo(void)
{
  return NULL;
}

const cRecording *cControl::GetRecording(void)
{
  return NULL;
}

string cControl::GetHeader(void)
{
  return "";
}

cControl *cControl::Control(bool Hidden)
{
  CLockObject lock(mutex);
  return (control && (!control->hidden || Hidden)) ? control : NULL;
}

void cControl::Launch(cControl *Control)
{
  CLockObject lock(mutex);
  cControl *c = control; // keeps control from pointing to uninitialized memory
  control = Control;
  delete c;
}

//void cControl::Attach(void)
//{
//  CLockObject lock(mutex);
//  if (control && !control->attached && control->player && !control->player->IsAttached()) {
//     if (cDeviceManager::Get().PrimaryDevice()->Player()->AttachPlayer(control->player))
//        control->attached = true;
//     else {
//       esyslog(tr("Channel locked (recording)!"));
//        Shutdown();
//        }
//     }
//}

void cControl::Shutdown(void)
{
  CLockObject lock(mutex);
  cControl *c = control; // avoids recursions
  control = NULL;
  delete c;
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
