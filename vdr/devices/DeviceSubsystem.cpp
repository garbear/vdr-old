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

#include "DeviceSubsystem.h"
#include "Device.h"

namespace VDR
{
cDeviceChannelSubsystem         *cDeviceSubsystem::Channel() const         { return m_device->Channel(); }
cDeviceCommonInterfaceSubsystem *cDeviceSubsystem::CommonInterface() const { return m_device->CommonInterface(); }
cDeviceImageGrabSubsystem       *cDeviceSubsystem::ImageGrab() const       { return m_device->ImageGrab(); }
cDevicePlayerSubsystem          *cDeviceSubsystem::Player() const          { return m_device->Player(); }
cDeviceReceiverSubsystem        *cDeviceSubsystem::Receiver() const        { return m_device->Receiver(); }
cDeviceScanSubsystem            *cDeviceSubsystem::Scan() const            { return m_device->Scan(); }
cDeviceSPUSubsystem             *cDeviceSubsystem::SPU() const             { return m_device->SPU(); }
cDeviceTrackSubsystem           *cDeviceSubsystem::Track() const           { return m_device->Track(); }
cDeviceVideoFormatSubsystem     *cDeviceSubsystem::VideoFormat() const     { return m_device->VideoFormat(); }
}
