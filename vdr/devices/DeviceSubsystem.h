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

#include <assert.h>
#include <typeinfo>

class cDevice;
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

class cDeviceSubsystem
{
public:
  virtual ~cDeviceSubsystem() { }

protected:
  /*!
   * \brief Construct the subsystem with a pointer to the device it belongs to
   */
  cDeviceSubsystem(cDevice *device) : m_device(device) { assert(m_device); }

  /*!
   * \brief Provide access to the device for calling functions in other subsystems
   *
   * This allows subsystems of derived devices to access their device as the
   * derived class. RTTI is used to assert on invalid pointer cast, ensuring the
   * validity of the returned pointer.
   */
  template <class DeviceType>
  DeviceType *GetDevice() const
  {
    DeviceType *device = dynamic_cast<DeviceType*>(m_device);
    assert(device);
    return device;
  }

  /*!
   * \brief Provide a non-templated default (assumes cDevice) because member
   *        functions don't support default function template arguments
   */
  cDevice *Device() const { return m_device; }

  cDeviceAudioSubsystem           *Audio() const;
  cDeviceChannelSubsystem         *Channel() const;
  cDeviceCommonInterfaceSubsystem *CommonInterface() const;
  cDeviceImageGrabSubsystem       *ImageGrab() const;
  cDevicePIDSubsystem             *PID() const;
  cDevicePlayerSubsystem          *Player() const;
  cDeviceReceiverSubsystem        *Receiver() const;
  cDeviceSectionFilterSubsystem   *SectionFilter() const;
  cDeviceSPUSubsystem             *SPU() const;
  cDeviceTrackSubsystem           *Track() const;
  cDeviceVideoFormatSubsystem     *VideoFormat() const;

private:
  cDeviceSubsystem(const cDeviceSubsystem &other); // non copy

  cDevice *m_device;
};
