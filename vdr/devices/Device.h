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

/*
 * Device.h: The basic device interface
 */

#include "DeviceTypes.h"
#include "channels/ChannelTypes.h"

#include <list>
#include <string>
#include <vector>

namespace VDR
{
class cDeviceChannelSubsystem;
class cDeviceCommonInterfaceSubsystem;
class cDeviceImageGrabSubsystem;
class cDevicePIDSubsystem;
class cDevicePlayerSubsystem;
class cDeviceReceiverSubsystem;
class cDeviceScanSubsystem;
class cDeviceSectionFilterSubsystem;
class cDeviceSPUSubsystem;
class cDeviceTrackSubsystem;
class cDeviceVideoFormatSubsystem;

struct cSubsystems
{
  void Free() const; // Free the subsystem pointers (TODO: Consider removing this function by switching to shared_ptrs)
  void AssertValid() const; // Asserts on empty pointer for the subsystems below
  cDeviceChannelSubsystem         *Channel;
  cDeviceCommonInterfaceSubsystem *CommonInterface;
  cDeviceImageGrabSubsystem       *ImageGrab;
  cDevicePIDSubsystem             *PID;
  cDevicePlayerSubsystem          *Player;
  cDeviceReceiverSubsystem        *Receiver;
  cDeviceScanSubsystem            *Scan;
  cDeviceSectionFilterSubsystem   *SectionFilter;
  cDeviceSPUSubsystem             *SPU;
  cDeviceTrackSubsystem           *Track;
  cDeviceVideoFormatSubsystem     *VideoFormat;
};

class cDevice
{
protected:
  cDevice(const cSubsystems &subsystems);

public:
  virtual ~cDevice();

  static const DevicePtr EmptyDevice;

  cDeviceChannelSubsystem*         Channel(void)         const { return m_subsystems.Channel; }
  cDeviceCommonInterfaceSubsystem* CommonInterface(void) const { return m_subsystems.CommonInterface; }
  cDeviceImageGrabSubsystem*       ImageGrab(void)       const { return m_subsystems.ImageGrab; }
  cDevicePIDSubsystem*             PID(void)             const { return m_subsystems.PID; }
  cDevicePlayerSubsystem*          Player(void)          const { return m_subsystems.Player; }
  cDeviceReceiverSubsystem*        Receiver(void)        const { return m_subsystems.Receiver; }
  cDeviceScanSubsystem*            Scan(void)            const { return m_subsystems.Scan; }
  cDeviceSectionFilterSubsystem*   SectionFilter(void)   const { return m_subsystems.SectionFilter; }
  cDeviceSPUSubsystem*             SPU(void)             const { return m_subsystems.SPU; }
  cDeviceTrackSubsystem*           Track(void)           const { return m_subsystems.Track; }
  cDeviceVideoFormatSubsystem*     VideoFormat(void)     const { return m_subsystems.VideoFormat; }

  /*!
   * \brief Returns the card index of this device
   * \return The card index in the range 0..MAXDEVICES-1
   */
  int CardIndex() const { return m_cardIndex; }
  void SetCardIndex(size_t index) { m_cardIndex = index; }

  /*!
   * \brief Returns a string identifying the type of this device (like "DVB-S")
   *
   * If this device can receive different delivery systems, the returned string
   * shall be that of the currently used system. The length of the returned
   * string should not exceed 6 characters.
   */
  virtual std::string DeviceType() const = 0;

  /*!
   * \brief Returns a string identifying the name of this device
   */
  virtual std::string DeviceName() const = 0;

  /*!
   * \brief Tells whether this device has an MPEG decoder
   */
  virtual bool HasDecoder() const { return false; }

  /*!
   * \brief Returns true if this device should only be used for recording if no
   *        other device is available
   */
  virtual bool AvoidRecording() const { return false; }

  /*!
   * Initialise the device
   * @return True when initialised, false otherwise.
   */
  virtual bool Initialise(void);

  /*!
   * @return True when initialised, false otherwise
   */
  virtual bool Initialised(void) const { return m_bInitialised; }

  bool ScanTransponder(const ChannelPtr& transponder);

  void AssertValid(void) { m_subsystems.AssertValid(); }

protected:
  /*!
   * \brief Returns true if this device is ready
   *
   * Devices with conditional access hardware may need some time until they are
   * up and running. This function is called in a loop at startup until all
   * devices are ready (see WaitForAllDevicesReady()).
   */
public: // TODO
  virtual bool Ready() { return true; }

private:
  const cSubsystems m_subsystems;
  bool              m_bInitialised;
  size_t            m_cardIndex;
};
}
