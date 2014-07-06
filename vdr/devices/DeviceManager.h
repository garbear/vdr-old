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

#include "Device.h"
#include "DeviceTypes.h"
#include "channels/ChannelTypes.h"
#include "utils/Observer.h"

#include <vector>

namespace VDR
{

class cDeviceManager : public Observer
{
private:
  cDeviceManager();
  cDeviceManager(const cDeviceManager &other); // no copy

public:
  static cDeviceManager &Get();

  ~cDeviceManager();

  /*!
   * Detect devices and initialise them
   * @return The total number of devices known to this class
   */
  size_t Initialise(void);

  /*!
   * \brief Returns the total number of devices
   */
  size_t NumDevices();

  /*!
   * \brief Waits until all devices have become ready, or the given Timeout has expired
   * \param timeout The timeout in seconds
   * \return True if all devices have become ready within the given timeout.
   *
   * While waiting, the Ready() function of each device is called in turn, until
   * they all return true.
   */
  bool WaitForAllDevicesReady(unsigned int timeout = 0);

  /*!
   * \brief Gets the device with the given Index
   * \param index Must be in the range 0..numDevices-1
   * \return A pointer to the device, or NULL if the index was invalid
   */
  DevicePtr GetDevice(unsigned int index);

  /*!
   * \brief Returns a device that is able to receive the given Channel at the
   *        given Priority, with the least impact on active recordings and live
   *        viewing
   * \param bQuery If true, no actual CAM assignments or receiver detachments
   *        will be done, so that this function can be called without any side
   *        effects in order to just determine whether a device is available for
   *        the given Channel
   * \sa ProvidesChannel()
   *
   * The LiveView parameter tells whether the device will be used for live
   * viewing or a recording. If the Channel is encrypted, a CAM slot that claims
   * to be able to decrypt the channel is automatically selected and assigned to
   * the returned device. Whether or not this combination of device and CAM slot
   * is actually able to decrypt the channel can only be determined by checking
   * the "scrambling control" bits of the received TS packets. The Action()
   * function automatically does this and takes care that after detaching any
   * receivers because the channel can't be decrypted, this device/CAM
   * combination will be skipped in the next call to GetDevice().
   */
  DevicePtr GetDevice(const cChannel &channel, bool bLiveView, bool bQuery = false);

  /*!
   * \brief Returns a device that is not currently "occupied" and can be tuned
   *        to the transponder of the given Channel
   * \return The device, or NULL if no such device is currently available
   */
  DevicePtr GetDeviceForTransponder(const cChannel &channel);

  /*!
   * \brief Get the number of transponders that provide the specified channel.
   */
  size_t CountTransponders(const cChannel &channel) const;

  /*!
   * \brief Closes down all devices. Must be called at the end of the program.
   */
  void Shutdown();

  void Notify(const Observable &obs, const ObservableMessage msg);

  /**
   * Scan a transponder using all devices known to cDeviceManager. Returns false if there are no more channels to
   * scan, or all devices fail to tune to the channel.
   */
  bool ScanTransponder(const ChannelPtr& transponder);

private:
  static int GetClippedNumProvidedSystems(int availableBits, const cDevice& device);

  PLATFORM::CMutex           m_mutex;
  DeviceVector               m_devices;
  size_t                     m_devicesReady;
  bool                       m_bAllDevicesReady; //XXX make CCondition support other things than bools...
  PLATFORM::CCondition<bool> m_devicesReadyCondition;
};

}
