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

#include <assert.h>
#include <typeinfo>

namespace VDR
{

class cDevice;
class cDeviceChannelSubsystem;
class cDeviceCommonInterfaceSubsystem;
class cDeviceImageGrabSubsystem;
class cDevicePlayerSubsystem;
class cDeviceReceiverSubsystem;
class cDeviceScanSubsystem;
class cDeviceSPUSubsystem;
class cDeviceTrackSubsystem;
class cDeviceVideoFormatSubsystem;

/*!
 * \brief cDeviceSubsystem is the base class for the various subsystems of a
 *        device (Audio, Track, etc). The class provides a way to access the
 *        device's other subsystems (as well as the device itself) in a
 *        straightforward syntax. The subsystem pointer is accessed by a member
 *        function of the same name. To call cDeviceAudioSubsystem::ToggleMute()
 *        in the same device's context, use Audio()->ToggleMute(). Subsystem
 *        class names follow the "cDevice[Name]Subsystem" pattern.
 */
class cDeviceSubsystem
{
public:
  /*!
   * \brief cDeviceSubsystem destructors must not reference m_device or other
   *        subsystems, as m_device is destructed before its subsystems
   */
  virtual ~cDeviceSubsystem() { }

protected:
  /*!
   * \brief Construct the subsystem with a pointer to the device it belongs to
   * \param device Pointer to subsystem's cDevice owner (don't dereference - not fully constructed)
   */
  cDeviceSubsystem(cDevice *device) : m_device(device) { assert(m_device); }

  /*!
   * \brief Provide access to the device for calling functions in other subsystems
   *
   * This allows subsystems of derived devices to access their device as the
   * derived class. RTTI is used to assert on invalid pointer cast, ensuring the
   * validity of the returned pointer.
   *
   * Member functions don't support default function template arguments, so the
   * default (cDevice) is explicitly specified below.
   */
  template <class DeviceType /* = cDevice */>
  DeviceType *Device() const
  {
    DeviceType *device = dynamic_cast<DeviceType*>(m_device);
    assert(device);
    return device;
  }

  cDevice *Device() const { return m_device; }

  cDeviceChannelSubsystem         *Channel() const;
  cDeviceCommonInterfaceSubsystem *CommonInterface() const;
  cDeviceImageGrabSubsystem       *ImageGrab() const;
  cDevicePlayerSubsystem          *Player() const;
  cDeviceReceiverSubsystem        *Receiver() const;
  cDeviceScanSubsystem            *Scan() const;
  cDeviceSPUSubsystem             *SPU() const;
  cDeviceTrackSubsystem           *Track() const;
  cDeviceVideoFormatSubsystem     *VideoFormat() const;

private:
  cDeviceSubsystem(const cDeviceSubsystem &other); // non copy

  cDevice* const m_device;
};
}
