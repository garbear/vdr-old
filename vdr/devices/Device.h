/*
 * device.h: The basic device interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: device.h 2.47.1.1 2013/08/22 12:01:48 kls Exp $
 */

#ifndef __DEVICE_H
#define __DEVICE_H

#include "channels.h"
#include "ci.h"
#include "dvbsubtitle.h"
#include "eit.h"
#include "filter.h"
#include "nit.h"
#include "pat.h"
#include "remux.h"
#include "ringbuffer.h"
#include "sdt.h"
#include "sections.h"
#include "spu.h"
#include "thread.h"
#include "tools.h"

#define MAXDEVICES         16 // the maximum number of devices in the system
#define MAXPIDHANDLES      64 // the maximum number of different PIDs per device
#define MAXOCCUPIEDTIMEOUT 99 // max. time (in seconds) a device may be occupied

enum eVideoSystem { vsPAL,
                    vsNTSC
                  };

enum eVideoDisplayFormat { vdfPanAndScan,
                           vdfLetterBox,
                           vdfCenterCutOut
                         };

enum eTrackType { ttNone,
                  ttAudio,
                  ttAudioFirst = ttAudio,
                  ttAudioLast  = ttAudioFirst + 31, // MAXAPIDS - 1
                  ttDolby,
                  ttDolbyFirst = ttDolby,
                  ttDolbyLast  = ttDolbyFirst + 15, // MAXDPIDS - 1
                  ttSubtitle,
                  ttSubtitleFirst = ttSubtitle,
                  ttSubtitleLast  = ttSubtitleFirst + 31, // MAXSPIDS - 1
                  ttMaxTrackTypes
                };

#define IS_AUDIO_TRACK(t) (ttAudioFirst <= (t) && (t) <= ttAudioLast)
#define IS_DOLBY_TRACK(t) (ttDolbyFirst <= (t) && (t) <= ttDolbyLast)
#define IS_SUBTITLE_TRACK(t) (ttSubtitleFirst <= (t) && (t) <= ttSubtitleLast)

struct tTrackId {
  uint16_t id;                  // The PES packet id or the PID.
  char language[MAXLANGCODE2];  // something like either "eng" or "deu+eng"
  char description[32];         // something like "Dolby Digital 5.1"
  };

class cPlayer;
class cReceiver;
class cLiveSubtitle;

class cDeviceHook : public cListObject {
public:
  cDeviceHook(void);
          ///< Creates a new device hook object.
          ///< Do not delete this object - it will be automatically deleted when the
          ///< program ends.
  virtual bool DeviceProvidesTransponder(const cDevice *Device, const cChannel *Channel) const;
          ///< Returns true if the given Device can provide the given Channel's transponder.
  };

/// The cDevice class is the base from which actual devices can be derived.

class cDevice : public cThread {
  friend class cLiveSubtitle;
  friend class cDeviceHook;
private:
  static int numDevices;
  static int useDevice;
  static cDevice *device[MAXDEVICES];
  static cDevice *primaryDevice;
public:
  static int NumDevices(void) { return numDevices; }
         ///< Returns the total number of devices.
  static bool WaitForAllDevicesReady(int Timeout = 0);
         ///< Waits until all devices have become ready, or the given Timeout
         ///< (seconds) has expired. While waiting, the Ready() function of each
         ///< device is called in turn, until they all return true.
         ///< Returns true if all devices have become ready within the given
         ///< timeout.
  static void SetUseDevice(int n);
         ///< Sets the 'useDevice' flag of the given device.
         ///< If this function is not called before initializing, all devices
         ///< will be used.
  static bool UseDevice(int n) { return useDevice == 0 || (useDevice & (1 << n)) != 0; }
         ///< Tells whether the device with the given card index shall be used in
         ///< this instance of VDR.
  static bool SetPrimaryDevice(int n);
         ///< Sets the primary device to 'n'.
         ///< n must be in the range 1...numDevices.
         ///< Returns true if this was possible.
  static cDevice *PrimaryDevice(void) { return primaryDevice; }
         ///< Returns the primary device.
  static cDevice *ActualDevice(void);
         ///< Returns the actual receiving device in case of Transfer Mode, or the
         ///< primary device otherwise.
  static cDevice *GetDevice(int Index);
         ///< Gets the device with the given Index.
         ///< Index must be in the range 0..numDevices-1.
         ///< Returns a pointer to the device, or NULL if the Index was invalid.
  static cDevice *GetDevice(const cChannel *Channel, int Priority, bool LiveView, bool Query = false);
         ///< Returns a device that is able to receive the given Channel at the
         ///< given Priority, with the least impact on active recordings and
         ///< live viewing. The LiveView parameter tells whether the device will
         ///< be used for live viewing or a recording.
         ///< If the Channel is encrypted, a CAM slot that claims to be able to
         ///< decrypt the channel is automatically selected and assigned to the
         ///< returned device. Whether or not this combination of device and CAM
         ///< slot is actually able to decrypt the channel can only be determined
         ///< by checking the "scrambling control" bits of the received TS packets.
         ///< The Action() function automatically does this and takes care that
         ///< after detaching any receivers because the channel can't be decrypted,
         ///< this device/CAM combination will be skipped in the next call to
         ///< GetDevice().
         ///< If Query is true, no actual CAM assignments or receiver detachments will
         ///< be done, so that this function can be called without any side effects
         ///< in order to just determine whether a device is available for the given
         ///< Channel.
         ///< See also ProvidesChannel().
  static cDevice *GetDeviceForTransponder(const cChannel *Channel, int Priority);
         ///< Returns a device that is not currently "occupied" and can be tuned to
         ///< the transponder of the given Channel, without disturbing any receiver
         ///< at priorities higher or equal to Priority.
         ///< If no such device is currently available, NULL will be returned.
  static void Shutdown(void);
         ///< Closes down all devices.
         ///< Must be called at the end of the program.
private:
  static int nextCardIndex;
  int cardIndex;
protected:
  cDevice(void);
  virtual ~cDevice();
  virtual bool Ready(void);
         ///< Returns true if this device is ready. Devices with conditional
         ///< access hardware may need some time until they are up and running.
         ///< This function is called in a loop at startup until all devices
         ///< are ready (see WaitForAllDevicesReady()).
  static int NextCardIndex(int n = 0);
         ///< Calculates the next card index.
         ///< Each device in a given machine must have a unique card index, which
         ///< will be used to identify the device for assigning Ca parameters and
         ///< deciding whether to actually use that device in this particular
         ///< instance of VDR. Every time a new cDevice is created, it will be
         ///< given the current nextCardIndex, and then nextCardIndex will be
         ///< automatically incremented by 1. A derived class can determine whether
         ///< a given device shall be used by checking UseDevice(NextCardIndex()).
         ///< If a device is skipped, or if there are possible device indexes left
         ///< after a derived class has set up all its devices, NextCardIndex(n)
         ///< must be called, where n is the number of card indexes to skip.
  virtual void MakePrimaryDevice(bool On);
         ///< Informs a device that it will be the primary device. If there is
         ///< anything the device needs to set up when it becomes the primary
         ///< device (On = true) or to shut down when it no longer is the primary
         ///< device (On = false), it should do so in this function.
         ///< A derived class must call the MakePrimaryDevice() function of its
         ///< base class.
public:
  bool IsPrimaryDevice(void) const { return this == primaryDevice && HasDecoder(); }
  int CardIndex(void) const { return cardIndex; }
         ///< Returns the card index of this device (0 ... MAXDEVICES - 1).
  int DeviceNumber(void) const;
         ///< Returns the number of this device (0 ... numDevices).
  virtual cString DeviceType(void) const;
         ///< Returns a string identifying the type of this device (like "DVB-S").
         ///< If this device can receive different delivery systems, the returned
         ///< string shall be that of the currently used system.
         ///< The length of the returned string should not exceed 6 characters.
         ///< The default implementation returns an empty string.
  virtual cString DeviceName(void) const;
         ///< Returns a string identifying the name of this device.
         ///< The default implementation returns an empty string.
  virtual bool HasDecoder(void) const;
         ///< Tells whether this device has an MPEG decoder.
  virtual bool AvoidRecording(void) const { return false; }
         ///< Returns true if this device should only be used for recording
         ///< if no other device is available.

// Device hooks

private:
  static cList<cDeviceHook> deviceHooks;
protected:
  bool DeviceHooksProvidesTransponder(const cChannel *Channel) const;

private:
  virtual void Action(void);

// Video format facilities

public:
  virtual void SetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat);
         ///< Sets the video display format to the given one (only useful
         ///< if this device has an MPEG decoder).
         ///< A derived class must first call the base class function!
  virtual void SetVideoFormat(bool VideoFormat16_9);
         ///< Sets the output video format to either 16:9 or 4:3 (only useful
         ///< if this device has an MPEG decoder).
  virtual eVideoSystem GetVideoSystem(void);
         ///< Returns the video system of the currently displayed material
         ///< (default is PAL).
  virtual void GetVideoSize(int &Width, int &Height, double &VideoAspect);
         ///< Returns the Width, Height and VideoAspect ratio of the currently
         ///< displayed video material. Width and Height are given in pixel
         ///< (e.g. 720x576) and VideoAspect is e.g. 1.33333 for a 4:3 broadcast,
         ///< or 1.77778 for 16:9.
         ///< The default implementation returns 0 for Width and Height
         ///< and 1.0 for VideoAspect.
  virtual void GetOsdSize(int &Width, int &Height, double &PixelAspect);
         ///< Returns the Width, Height and PixelAspect ratio the OSD should use
         ///< to best fit the resolution of the output device. If PixelAspect
         ///< is not 1.0, the OSD may take this as a hint to scale its
         ///< graphics in a way that, e.g., a circle will actually
         ///< show up as a circle on the screen, and not as an ellipse.
         ///< Values greater than 1.0 mean to stretch the graphics in the
         ///< vertical direction (or shrink it in the horizontal direction,
         ///< depending on which dimension shall be fixed). Values less than
         ///< 1.0 work the other way round. Note that the OSD is not guaranteed
         ///< to actually use this hint.

// Track facilities

private:
  tTrackId availableTracks[ttMaxTrackTypes];
  eTrackType currentAudioTrack;
  eTrackType currentSubtitleTrack;
  cMutex mutexCurrentAudioTrack;
  cMutex mutexCurrentSubtitleTrack;
  int currentAudioTrackMissingCount;
  bool autoSelectPreferredSubtitleLanguage;
  bool keepTracks;
  int pre_1_3_19_PrivateStream;
protected:
  virtual void SetAudioTrackDevice(eTrackType Type);
       ///< Sets the current audio track to the given value.
  virtual void SetSubtitleTrackDevice(eTrackType Type);
       ///< Sets the current subtitle track to the given value.
public:
  void ClrAvailableTracks(bool DescriptionsOnly = false, bool IdsOnly = false);
       ///< Clears the list of currently available tracks. If DescriptionsOnly
       ///< is true, only the track descriptions will be cleared. With IdsOnly
       ///< set to true only the ids will be cleared. IdsOnly is only taken
       ///< into account if DescriptionsOnly is false.
  bool SetAvailableTrack(eTrackType Type, int Index, uint16_t Id, const char *Language = NULL, const char *Description = NULL);
       ///< Sets the track of the given Type and Index to the given values.
       ///< Type must be one of the basic eTrackType values, like ttAudio or ttDolby.
       ///< Index tells which track of the given basic type is meant.
       ///< If Id is 0 any existing id will be left untouched and only the
       ///< given Language and Description will be set.
       ///< Returns true if the track was set correctly, false otherwise.
  const tTrackId *GetTrack(eTrackType Type);
       ///< Returns a pointer to the given track id, or NULL if Type is not
       ///< less than ttMaxTrackTypes.
  int NumTracks(eTrackType FirstTrack, eTrackType LastTrack) const;
       ///< Returns the number of tracks in the given range that are currently
       ///< available.
  int NumAudioTracks(void) const;
       ///< Returns the number of audio tracks that are currently available.
       ///< This is just for information, to quickly find out whether there
       ///< is more than one audio track.
  int NumSubtitleTracks(void) const;
       ///< Returns the number of subtitle tracks that are currently available.
  eTrackType GetCurrentAudioTrack(void) const { return currentAudioTrack; }
  bool SetCurrentAudioTrack(eTrackType Type);
       ///< Sets the current audio track to the given Type.
       ///< Returns true if Type is a valid audio track, false otherwise.
  eTrackType GetCurrentSubtitleTrack(void) const { return currentSubtitleTrack; }
  bool SetCurrentSubtitleTrack(eTrackType Type, bool Manual = false);
       ///< Sets the current subtitle track to the given Type.
       ///< IF Manual is true, no automatic preferred subtitle language selection
       ///< will be done for the rest of the current replay session, or until
       ///< the channel is changed.
       ///< Returns true if Type is a valid subtitle track, false otherwise.
  void EnsureAudioTrack(bool Force = false);
       ///< Makes sure an audio track is selected that is actually available.
       ///< If Force is true, the language and Dolby Digital settings will
       ///< be verified even if the current audio track is available.
  void EnsureSubtitleTrack(void);
       ///< Makes sure one of the preferred language subtitle tracks is selected.
       ///< Only has an effect if Setup.DisplaySubtitles is on.
  void SetKeepTracks(bool KeepTracks) { keepTracks = KeepTracks; }
       ///< Controls whether the current audio and subtitle track settings shall
       ///< be kept as they currently are, or if they shall be automatically
       ///< adjusted. This is used when pausing live video.
  };

/// Derived cDevice classes that can receive channels will have to provide
/// Transport Stream (TS) packets one at a time. cTSBuffer implements a
/// simple buffer that allows the device to read a larger amount of data
/// from the driver with each call to Read(), thus avoiding the overhead
/// of getting each TS packet separately from the driver. It also makes
/// sure the returned data points to a TS packet and automatically
/// re-synchronizes after broken packets.

class cTSBuffer : public cThread {
private:
  int f;
  int cardIndex;
  bool delivered;
  cRingBufferLinear *ringBuffer;
  virtual void Action(void);
public:
  cTSBuffer(int File, int Size, int CardIndex);
  ~cTSBuffer();
  uchar *Get(void);
  };

#endif //__DEVICE_H
