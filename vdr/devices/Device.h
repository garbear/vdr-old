/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

/*
 * Device.h: The basic device interface
 */

#include "../../thread.h" // for cThread
#include "../../tools.h" // for cListObject

#include <list>
#include <string>

class cDeviceAudioSubsystem;
class cDeviceChannelSubsystem;
class cDeviceCommonInterfaceSubsystem;
class cDeviceImageGrabSubsystem;
class cDevicePIDSubsystem;
class cDevicePlayerSubsystem;
class cDeviceReceiverSubsystem;
class cDeviceSectionFilterSubsystem;
class cDeviceSPUSubsystem;
class cDeviceTrackSubsystem;
class cDeviceVideoFormatSubsystem;

struct cSubsystems
{
  cDeviceAudioSubsystem           *Audio;
  cDeviceChannelSubsystem         *Channel;
  cDeviceCommonInterfaceSubsystem *CommonInterface;
  cDeviceImageGrabSubsystem       *ImageGrab;
  cDevicePIDSubsystem             *PID;
  cDevicePlayerSubsystem          *Player;
  cDeviceReceiverSubsystem        *Receiver;
  cDeviceSectionFilterSubsystem   *SectionFilter;
  cDeviceSPUSubsystem             *SPU;
  cDeviceTrackSubsystem           *Track;
  cDeviceVideoFormatSubsystem     *VideoFormat;
};

#define FREE_SUBSYSTEMS(s) \
  do \
  { \
    delete s.Audio;           s.Audio = NULL; \
    delete s.Channel;         s.Channel = NULL; \
    delete s.CommonInterface; s.CommonInterface = NULL; \
    delete s.ImageGrab;       s.ImageGrab = NULL; \
    delete s.PID;             s.PID = NULL; \
    delete s.Player;          s.Player = NULL; \
    delete s.Receiver;        s.Receiver = NULL; \
    delete s.SectionFilter;   s.SectionFilter = NULL; \
    delete s.SPU;             s.SPU = NULL; \
    delete s.Track;           s.Track = NULL; \
    delete s.VideoFormat;     s.VideoFormat = NULL; \
  } \
  while (0)

class cDevice;
class cChannel;

class cDeviceHook : public cListObject
{
public:
  /*!
   * \brief Creates a new device hook object
   *
   * Do not delete this object - it will be automatically deleted when the
   * program ends. YOU MUST PUT THE FOLLOWING COMMENT BEFORE ALL UNASSIGNED
   * 'new cDeviceHook()' STATEMENTS LIKE SO:
   *
   * // Unassigned: ownership is assumed by persistent list
   * new cDeviceHook();
   */
  cDeviceHook();

  /*!
   * \brief Returns true if the given Device can provide the given Channel's transponder
   */
  virtual bool DeviceProvidesTransponder(const cDevice &device, const cChannel &channel) const { return true; }
};

/// Derived cDevice classes that can receive channels will have to provide
/// Transport Stream (TS) packets one at a time. cTSBuffer implements a
/// simple buffer that allows the device to read a larger amount of data
/// from the driver with each call to Read(), thus avoiding the overhead
/// of getting each TS packet separately from the driver. It also makes
/// sure the returned data points to a TS packet and automatically
/// re-synchronizes after broken packets.

class cRingBufferLinear;

class cTSBuffer : public cThread
{
public:
  cTSBuffer(int file, unsigned int size, int cardIndex);
  ~cTSBuffer();

  uchar *Get();

private:
  virtual void Action();

  int                m_file;
  int                m_cardIndex;
  bool               m_bDelivered;
  cRingBufferLinear *m_ringBuffer;
};

/// The cDevice class is the base from which actual devices can be derived.

class cPlayer;
class cReceiver;
class cLiveSubtitle;

class cDevice : public cThread
{
  //friend class cLiveSubtitle;
  //friend class cDeviceHook;
  //friend class cDeviceSubtitle;

protected:
  /*!
   * \brief
   * \param subsystems
   * \param index
   */
  cDevice(const cSubsystems &subsystems, unsigned int index);

public:
  virtual ~cDevice() { }

  bool IsPrimaryDevice() const;

  /*!
   * \brief Returns the card index of this device
   * \return The card index in the range 0..MAXDEVICES-1
   */
  int CardIndex() const { return m_cardIndex; }

  /*!
   * \brief Returns the number of this device (starting at 1)
   */
  unsigned int DeviceNumber() const { return m_number; }

  /*!
   * \brief Returns a string identifying the type of this device (like "DVB-S")
   *
   * If this device can receive different delivery systems, the returned string
   * shall be that of the currently used system. The length of the returned
   * string should not exceed 6 characters.
   */
  virtual cString DeviceType() const { return ""; }

  /*!
   * \brief Returns a string identifying the name of this device
   */
  virtual cString DeviceName() const { return ""; }

  /*!
   * \brief Tells whether this device has an MPEG decoder
   */
  virtual bool HasDecoder() const { return false; }

  /*!
   * \brief Returns true if this device should only be used for recording if no
   *        other device is available
   */
  virtual bool AvoidRecording() const { return false; }

protected:
  /*!
   * \brief Returns true if this device is ready
   *
   * Devices with conditional access hardware may need some time until they are
   * up and running. This function is called in a loop at startup until all
   * devices are ready (see WaitForAllDevicesReady()).
   */
  virtual bool Ready() { return true; }

  /*!
   * \brief Informs a device that it will be the primary device
   *
   * If there is anything the device needs to set up when it becomes the primary
   * device (bOn = true) or to shut down when it no longer is the primary device
   * (bOn = false), it should do so in this function. A derived class must call
   * the MakePrimaryDevice() function of its base class.
   */
  virtual void MakePrimaryDevice(bool bOn);


public:
  cDeviceAudioSubsystem           *Audio()           const { return m_subsystems.Audio; }
  cDeviceChannelSubsystem         *Channel()         const { return m_subsystems.Channel; }
  cDeviceCommonInterfaceSubsystem *CommonInterface() const { return m_subsystems.CommonInterface; }
  cDeviceImageGrabSubsystem       *ImageGrab()       const { return m_subsystems.ImageGrab; }
  cDevicePIDSubsystem             *PID()             const { return m_subsystems.PID; }
  cDevicePlayerSubsystem          *Player()          const { return m_subsystems.Player; }
  cDeviceReceiverSubsystem        *Receiver()        const { return m_subsystems.Receiver; }
  cDeviceSectionFilterSubsystem   *SectionFilter()   const { return m_subsystems.SectionFilter; }
  cDeviceSPUSubsystem             *SPU()             const { return m_subsystems.SPU; }
  cDeviceTrackSubsystem           *Track()           const { return m_subsystems.Track; }
  cDeviceVideoFormatSubsystem     *VideoFormat()     const { return m_subsystems.VideoFormat; }

private:
  virtual void Action();

  cSubsystems m_subsystems;

  unsigned int m_number; // Strictly positive
  unsigned int m_cardIndex;
};
