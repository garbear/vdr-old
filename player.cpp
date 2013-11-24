/*
 * player.c: The basic player interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: player.c 2.2 2012/04/28 11:52:50 kls Exp $
 */

#include "player.h"
#include "i18n.h"

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

int cPlayer::PlayPes(const uchar *Data, int Length, bool VideoOnly)
{
  if (device)
     return device->PlayPes(Data, Length, VideoOnly);
  esyslog("ERROR: attempt to use cPlayer::PlayPes() without attaching to a cDevice!");
  return -1;
}

int cPlayer::PlayTs(const uchar *Data, int Length, bool VideoOnly /* = false */)
{
  return device ? device->PlayTs(Data, Length, VideoOnly) : -1;
}

void cPlayer::Detach(void)
{
  if (device)
     device->Detach(this);
}

// --- cControl --------------------------------------------------------------

cControl *cControl::control = NULL;
cMutex cControl::mutex;

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

cString cControl::GetHeader(void)
{
  return "";
}

cControl *cControl::Control(bool Hidden)
{
  cMutexLock MutexLock(&mutex);
  return (control && (!control->hidden || Hidden)) ? control : NULL;
}

void cControl::Launch(cControl *Control)
{
  cMutexLock MutexLock(&mutex);
  cControl *c = control; // keeps control from pointing to uninitialized memory
  control = Control;
  delete c;
}

void cControl::Attach(void)
{
  cMutexLock MutexLock(&mutex);
  if (control && !control->attached && control->player && !control->player->IsAttached()) {
     if (cDevice::PrimaryDevice()->AttachPlayer(control->player))
        control->attached = true;
     else {
        Skins.Message(mtError, tr("Channel locked (recording)!"));
        Shutdown();
        }
     }
}

void cControl::Shutdown(void)
{
  cMutexLock MutexLock(&mutex);
  cControl *c = control; // avoids recursions
  control = NULL;
  delete c;
}

void cPlayer::DeviceClrAvailableTracks(bool DescriptionsOnly /* = false */)
{
  if (device) device->ClrAvailableTracks(DescriptionsOnly);
}

bool cPlayer::DeviceSetAvailableTrack(eTrackType Type, int Index, uint16_t Id, const char *Language /* = NULL */, const char *Description /* = NULL*/)
{
  if (device)
    return device->SetAvailableTrack(Type, Index, Id, Language, Description);
  return false;
}

bool cPlayer::DeviceSetCurrentAudioTrack(eTrackType Type)
{
  return device ? device->SetCurrentAudioTrack(Type) : false;
}

bool cPlayer::DeviceSetCurrentSubtitleTrack(eTrackType Type)
{
  return device ? device->SetCurrentSubtitleTrack(Type) : false;
}

bool cPlayer::DevicePoll(cPoller &Poller, int TimeoutMs = 0)
{
  return device ? device->Poll(Poller, TimeoutMs) : false;
}

bool cPlayer::DeviceFlush(int TimeoutMs = 0)
{
  return device ? device->Flush(TimeoutMs) : true;
}

bool cPlayer::DeviceHasIBPTrickSpeed(void)
{
  return device ? device->HasIBPTrickSpeed() : false;
}

bool cPlayer::DeviceIsPlayingVideo(void)
{
  return device ? device->IsPlayingVideo() : false;
}

void cPlayer::DeviceTrickSpeed(int Speed)
{
  if (device) device->TrickSpeed(Speed);
}

void cPlayer::DeviceClear(void)
{
  if (device) device->Clear();
}

void cPlayer::DevicePlay(void)
{
  if (device) device->Play();
}

void cPlayer::DeviceFreeze(void)
{
  if (device) device->Freeze();
}
void cPlayer::DeviceMute(void)
{
  if (device) device->Mute();
}

void cPlayer::DeviceSetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat)
{
  if (device) device->SetVideoDisplayFormat(VideoDisplayFormat);
}

void cPlayer::DeviceStillPicture(const uchar *Data, int Length)
{
  if (device) device->StillPicture(Data, Length);
}

uint64_t cPlayer::DeviceGetSTC(void)
{
  return device ? device->GetSTC() : -1;
}
