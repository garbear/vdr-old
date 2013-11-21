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
