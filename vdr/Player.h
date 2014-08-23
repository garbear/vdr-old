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
#pragma once

#include "devices/subsystems/DevicePlayerSubsystem.h"      // for ePlayMode
#include "devices/subsystems/DeviceTrackSubsystem.h"       // for eTrackType
#include "devices/subsystems/DeviceVideoFormatSubsystem.h" // for eVideoDisplayFormat
#include "recordings/Recording.h"
#include "recordings/marks/Mark.h"

#include <stdint.h>
#include <string>

namespace VDR
{

class cDevice;

class cPlayer {
  friend class cDevice;
private:
public: // TODO
  cDevice *device;
  ePlayMode playMode;
protected:
  void DeviceClrAvailableTracks(bool DescriptionsOnly = false);
  bool DeviceSetAvailableTrack(eTrackType Type, int Index, uint16_t Id, const char *Language = NULL, const char *Description = NULL);
  bool DeviceSetCurrentAudioTrack(eTrackType Type);
  bool DeviceSetCurrentSubtitleTrack(eTrackType Type);
  bool DevicePoll(cPoller &Poller, int TimeoutMs = 0);
  bool DeviceFlush(int TimeoutMs = 0);
  bool DeviceHasIBPTrickSpeed(void);
  bool DeviceIsPlayingVideo(void);
  void DeviceTrickSpeed(int Speed);
  void DeviceClear(void);
  void DevicePlay(void);
  void DeviceFreeze(void);
  void DeviceMute(void);
  void DeviceSetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat);
  void DeviceStillPicture(const std::vector<uint8_t> &data);
  uint64_t DeviceGetSTC(void);
  void Detach(void);
public: // TODO
  virtual void Activate(bool On) {}
       // This function is called right after the cPlayer has been attached to
       // (On == true) or before it gets detached from (On == false) a cDevice.
       // It can be used to do things like starting/stopping a thread.
protected:
  // XXX probably convert all of these back from vectors to POD types to reduce the overhead
  int PlayPes(const uint8_t *Data, int Length, bool VideoOnly = false);
  int PlayPes(const std::vector<uint8_t> &data, bool VideoOnly = false);
       // Sends the given PES Data to the device and returns the number of
       // bytes that have actually been accepted by the device (or a
       // negative value in case of an error).
  int PlayTs(const uint8_t *Data, int Length, bool VideoOnly = false);
  int PlayTs(const std::vector<uint8_t> &data, bool VideoOnly = false);
       // Sends the given TS packet to the device and returns a positive number
       // if the packet has been accepted by the device, or a negative value in
       // case of an error.
public:
  cPlayer(ePlayMode PlayMode = pmAudioVideo);
  virtual ~cPlayer();
  bool IsAttached(void) { return device != NULL; }
  virtual double FramesPerSecond(void) { return DEFAULTFRAMESPERSECOND; }
       // Returns the number of frames per second of the currently played material.
  virtual bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false) { return false; }
       // Returns the current and total frame index, optionally snapped to the
       // nearest I-frame.
  virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed) { return false; }
       // Returns the current replay mode (if applicable).
       // 'Play' tells whether we are playing or pausing, 'Forward' tells whether
       // we are going forward or backward and 'Speed' is -1 if this is normal
       // play/pause mode, 0 if it is single speed fast/slow forward/back mode
       // and >0 if this is multi speed mode.
  virtual void SetAudioTrack(eTrackType Type, const tTrackId *TrackId) {}
       // Sets the current audio track to the given value.
       // This is just a virtual hook for players that need to do special things
       // in order to switch audio tracks.
  virtual void SetSubtitleTrack(eTrackType Type, const tTrackId *TrackId) {}
       // Sets the current subtitle track to the given value.
       // This is just a virtual hook for players that need to do special things
       // in order to switch subtitle tracks.
  };

}
