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

#include "Types.h"
#include "Device.h"
#include "utils/Observer.h"

#include <vector>

class cChannel;

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
   * \brief Have cDeviceManager track a new device
   * \return The new device number (starting at 1)
   */
  size_t AddDevice(DevicePtr device);

  /*!
   * \brief Returns the total number of devices
   */
  size_t NumDevices();

  /*!
   * \brief Skip the specified number of steps in the card indexing
   *
   * E.g. after calling AdvanceCardIndex(2), the next device might get a card
   * index of 4 instead of 2.
   */
  void AdvanceCardIndex(unsigned int steps) { m_nextCardIndex += steps; }






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
   * \brief Returns a monotonically-incrementing index
   * \return An index 1 greater than the previous index returned by this function?
   */
  //unsigned int GetNextCardIndex();

  /*!
   * \brief Calculates the next card index
   *
   * Each device in a given machine must have a unique card index, which will be
   * used to identify the device for assigning Ca parameters and deciding
   * whether to actually use that device in this particular instance of VDR.
   * Every time a new cDevice is created, it will be given the current
   * nextCardIndex, and then nextCardIndex will be automatically incremented by
   * 1. A derived class can determine whether a given device shall be used by
   * checking UseDevice(NextCardIndex()). If a device is skipped, or if there
   * are possible device indexes left after a derived class has set up all its
   * devices, NextCardIndex(n) must be called, where n is the number of card
   * indexes to skip.
   */
  //int NextCardIndex(int n = 0);

  /*!
   * \brief Sets the 'useDevice' flag of the given device
   *
   * If this function is not called before initializing, all devices will be used.
   */
  void SetUseDevice(unsigned int n) { m_useDevice |= (1 << n); }

  /*!
   * \brief Tells whether the device with the given card index shall be used in
   *        this instance of VDR
   */
  bool UseDevice(unsigned int n) { return m_useDevice == 0 || (m_useDevice & (1 << n)) != 0; }

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
  DevicePtr GetDevice(const cChannel &channel, int priority, bool bLiveView, bool bQuery = false);

  /*!
   * \brief Returns a device that is not currently "occupied" and can be tuned
   *        to the transponder of the given Channel, without disturbing any
   *        receiver at priorities higher or equal to Priority
   * \return The device, or NULL if no such device is currently available
   */
  DevicePtr GetDeviceForTransponder(const cChannel &channel, int priority);

  /*!
   * \brief Get the number of transponders that provide the specified channel.
   */
  size_t CountTransponders(const cChannel &channel) const;

  /*!
   * \brief Closes down all devices. Must be called at the end of the program.
   */
  void Shutdown();

  void Notify(const Observable &obs, const ObservableMessage msg);

  DeviceVector::const_iterator Iterator(void) const { return m_devices.begin(); }
  bool IteratorHasNext(const DeviceVector::const_iterator& it) const { return it != m_devices.end(); }
  void IteratorNext(DeviceVector::const_iterator& it) const { ++it; }

private:
  static int GetClippedNumProvidedSystems(int availableBits, const cDevice& device);

  PLATFORM::CMutex           m_mutex;
  DeviceVector               m_devices;
  size_t                     m_devicesReady;
  bool                       m_bAllDevicesReady; //XXX make CCondition support other things than bools...
  PLATFORM::CCondition<bool> m_devicesReadyCondition;

  unsigned int m_nextCardIndex; // Card index to give to the next new device
  unsigned int m_useDevice; // Stupid device-use flags
};
