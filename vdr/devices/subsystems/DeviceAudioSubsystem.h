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

#include "devices/DeviceSubsystem.h"

#define MAXVOLUME         255
#define VOLUMEDELTA         5 // used to increase/decrease the volume

class cDevice;

class cDeviceAudioSubsystem : protected cDeviceSubsystem
{
public:
  cDeviceAudioSubsystem(cDevice *device);
  virtual ~cDeviceAudioSubsystem() { }

  bool IsMute() const { return m_bMute; }

  /*!
   * \brief Turns the volume off or on
   * \return The new mute state
   */
  bool ToggleMute();

  /*!
   * \brief Gets the current audio channel
   * \return The current channel: stereo (0), mono left (1) or mono right (2)
   */
  int GetAudioChannel(void);

  /*!
   * \brief Sets the audio channel
   * \param audioChannel The audio channel: stereo (0), mono left (1) or mono right (2)
   */
  void SetAudioChannel(int AudioChannel);

  /*!
   * \brief Sets the volume to the given value
   * \param volume The volume
   * \param bAbsolute True if absolute volume, false if relative to the current volume
   */
  void SetVolume(int volume, bool bAbsolute = false);

  //static int CurrentVolume(void) { return m_primaryDevice ? m_primaryDevice->volume : 0; }//XXX???

protected:
  /*!
   * \brief Gets the current audio channel
   * \return The audio channel: stereo (0), mono left (1) or mono right (2)
   */
  virtual int GetAudioChannelDevice() { return 0; }

  /*!
   * \brief Sets the audio channel
   * \param audioChannel Either stereo (0), mono left (1) or mono right (2)
   */
  virtual void SetAudioChannelDevice(int audioChannel);

  /*!
   * \brief Sets the audio volume on this device
   * \param volume The volume in the range 0..255
   */
public: // TODO
  virtual void SetVolumeDevice(unsigned int volume) { }

  /*!
   * \brief Tells the actual device that digital audio output shall be switched on or off
   * \param bOn True for on, false for off
   */
  virtual void SetDigitalAudioDevice(bool bOn) { }

//private:
  bool          m_bMute;
private:
  unsigned int  m_volume; // In the range 0..255
};
