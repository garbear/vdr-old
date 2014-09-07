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
#include "TunerHandle.h"
#include "channels/ChannelTypes.h"
#include "utils/Observer.h"

#include <vector>

namespace VDR
{

class iReceiver;

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
   * \brief Closes down all devices. Must be called at the end of the program.
   */
  void Shutdown();

  TunerHandlePtr OpenVideoInput(iReceiver* receiver, const ChannelPtr& channel);
  void CloseVideoInput(iReceiver* receiver);

  void Notify(const Observable &obs, const ObservableMessage msg);

private:
  PLATFORM::CMutex           m_mutex;
  DeviceVector               m_devices;
  size_t                     m_devicesReady;
  bool                       m_bAllDevicesReady; //XXX make CCondition support other things than bools...
  PLATFORM::CCondition<bool> m_devicesReadyCondition;
};

}
