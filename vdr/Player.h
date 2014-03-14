/*
 * player.h: The basic player interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: player.h 2.6 2012/04/28 13:04:17 kls Exp $
 */

#ifndef __PLAYER_H
#define __PLAYER_H

#include "Types.h"
#include "devices/subsystems/DevicePlayerSubsystem.h"
#include "devices/subsystems/DeviceTrackSubsystem.h"
#include "devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "recordings/Recording.h"
#include "recordings/marks/Mark.h"

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
  void DeviceStillPicture(const std::vector<uchar> &data);
  uint64_t DeviceGetSTC(void);
  void Detach(void);
public: // TODO
  virtual void Activate(bool On) {}
       // This function is called right after the cPlayer has been attached to
       // (On == true) or before it gets detached from (On == false) a cDevice.
       // It can be used to do things like starting/stopping a thread.
protected:
  // XXX probably convert all of these back from vectors to POD types to reduce the overhead
  int PlayPes(const uchar *Data, int Length, bool VideoOnly = false);
  int PlayPes(const std::vector<uchar> &data, bool VideoOnly = false);
       // Sends the given PES Data to the device and returns the number of
       // bytes that have actually been accepted by the device (or a
       // negative value in case of an error).
  int PlayTs(const uchar *Data, int Length, bool VideoOnly = false);
  int PlayTs(const std::vector<uchar> &data, bool VideoOnly = false);
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

class cOsdObject { ///XXX
  friend class cOsdMenu;
private:
  bool isMenu;
  bool needsFastResponse;
protected:
  void SetNeedsFastResponse(bool NeedsFastResponse) { needsFastResponse = NeedsFastResponse; }
public:
  cOsdObject(bool FastResponse = false) { isMenu = false; needsFastResponse = FastResponse; }
  virtual ~cOsdObject() {}
  virtual bool NeedsFastResponse(void) { return needsFastResponse; }
  bool IsMenu(void) const { return isMenu; }
  virtual void Show(void) { /* XXX */ }
// virtual eOSState ProcessKey(eKeys Key) { return osUnknown; }
  };

class cControl : public cOsdObject {
private:
  static cControl *control;
  static PLATFORM::CMutex mutex;
  bool attached;
  bool hidden;
protected:
  cPlayer *player;
public:
  cControl(cPlayer *Player, bool Hidden = false);
  virtual ~cControl();
  virtual void Hide(void) = 0;
  virtual cOsdObject *GetInfo(void);
         ///< Returns an OSD object that displays information about the currently
         ///< played programme. If no such information is available, NULL will be
         ///< returned.
  virtual const cRecording *GetRecording(void);
         ///< Returns the cRecording that is currently being replayed, or NULL if
         ///< this player is not playing a cRecording.
  virtual cString GetHeader(void);
         ///< This can be used by players that don't play a cRecording, but rather
         ///< do something completely different. The resulting string may be used by
         ///< skins as a last resort, in case they want to display the state of the
         ///< current player. The return value is expected to be a short, single line
         ///< string. The default implementation returns an empty string.
  double FramesPerSecond(void) { return player->FramesPerSecond(); }
  bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false) { return player->GetIndex(Current, Total, SnapToIFrame); }
  bool GetReplayMode(bool &Play, bool &Forward, int &Speed) { return player->GetReplayMode(Play, Forward, Speed); }
  static void Launch(cControl *Control);
//  static void Attach(void);
  static void Shutdown(void);
  static cControl *Control(bool Hidden = false);
         ///< Returns the current replay control (if any) in case it is currently
         ///< visible. If Hidden is true, the control will be returned even if it is
         ///< currently hidden.
  };

#endif //__PLAYER_H
