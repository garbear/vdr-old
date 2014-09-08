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
#include "TunerHandle.h"
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
class cTunerHandle;

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

  /*!
   * Initialise the device. Must be called by subclass if overloaded.
   * @param index The device index (chosen by cDeviceManager)
   * @return True when initialised, false otherwise.
   */
  virtual bool Initialise(unsigned int index);
  virtual void Deinitialise(void);

  /*!
   * @return True when initialised, false otherwise
   */
  virtual bool Initialised(void) const { return m_bInitialised; }

  /*!
   * \brief Returns true if this device is ready
   *
   * Devices with conditional access hardware may need some time until they are
   * up and running.
   */
  virtual bool Ready() { return true; }

  /*!
   * Properties available after calling Initialise():
   *   - Index
   *   - Name
   *   - ID (string uniquely identifying this device, e.g. /dev/dvb/adapter0/frontend0
   *     on linux)
   */
  int Index() const { return m_index; }
  virtual std::string Name() const = 0;
  virtual std::string ID() const = 0;

  /*!
   * \brief Tells whether this device has an MPEG decoder
   */
  virtual bool HasDecoder() const { return false; }

  TunerHandlePtr Acquire(const ChannelPtr& channel, device_tuning_type_t type, iTunerHandleCallbacks* callbacks);
  void Release(TunerHandlePtr& handle);
  bool CanTune(device_tuning_type_t type);

  cDeviceChannelSubsystem*         Channel(void)         const { return m_subsystems.Channel; }
  cDeviceCommonInterfaceSubsystem* CommonInterface(void) const { return m_subsystems.CommonInterface; }
  cDeviceImageGrabSubsystem*       ImageGrab(void)       const { return m_subsystems.ImageGrab; }
  cDevicePlayerSubsystem*          Player(void)          const { return m_subsystems.Player; }
  cDeviceReceiverSubsystem*        Receiver(void)        const { return m_subsystems.Receiver; }
  cDeviceScanSubsystem*            Scan(void)            const { return m_subsystems.Scan; }
  cDeviceSectionFilterSubsystem*   SectionFilter(void)   const { return m_subsystems.SectionFilter; }
  cDeviceSPUSubsystem*             SPU(void)             const { return m_subsystems.SPU; }
  cDeviceTrackSubsystem*           Track(void)           const { return m_subsystems.Track; }
  cDeviceVideoFormatSubsystem*     VideoFormat(void)     const { return m_subsystems.VideoFormat; }

private:
  const cSubsystems m_subsystems;
  bool              m_bInitialised;
  size_t            m_index;
};
}
