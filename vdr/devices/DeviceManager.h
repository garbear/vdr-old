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

#include "Device.h"

#include <vector>

class cChannel;

class cDeviceManager
{
private:
  cDeviceManager();
  cDeviceManager(const cDeviceManager &other); // no copy

public:
  static cDeviceManager &Get();

  ~cDeviceManager();

  /*!
   * \brief Have cDeviceManager track a new device
   * \return The new device number (starting at 1)
   */
  unsigned int AddDevice(cDevice *device);

  /*!
   * \brief Returns the total number of devices
   */
  int NumDevices() { return m_devices.size(); }

  /*!
   * \brief Returns the primary device
   */
  cDevice *PrimaryDevice() { return m_primaryDevice; }

  /*!
   * \brief Sets the primary device to 'n'
   * \param n Must be in the range 1..NumDevices()
   */
  void SetPrimaryDevice(unsigned int index);

  /*!
   * \brief Skip the specified number of steps in the card indexing
   *
   * E.g. after calling AdvanceCardIndex(2), the next device might get a card
   * index of 4 instead of 2.
   */
  void AdvanceCardIndex(unsigned int steps) { m_nextCardIndex += steps; }

  void AddHook(cDeviceHook *hook);

  bool DeviceHooksProvidesTransponder(const cDevice &device, const cChannel &channel) const;






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
   * \brief Returns the actual receiving device in case of Transfer Mode, or the
   *        primary device otherwise
   */
  cDevice *ActualDevice();

  /*!
   * \brief Gets the device with the given Index
   * \param index Must be in the range 0..numDevices-1
   * \return A pointer to the device, or NULL if the index was invalid
   */
  cDevice *GetDevice(unsigned int index);

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
  cDevice *GetDevice(const cChannel &channel, int priority, bool bLiveView, bool bQuery = false);

  /*!
   * \brief Returns a device that is not currently "occupied" and can be tuned
   *        to the transponder of the given Channel, without disturbing any
   *        receiver at priorities higher or equal to Priority
   * \return The device, or NULL if no such device is currently available
   */
  cDevice *GetDeviceForTransponder(const cChannel &channel, int priority);

  /*!
   * \brief Get the number of transponders that provide the specified channel.
   */
  unsigned int CountTransponders(const cChannel &channel) const;

  /*!
   * \brief Switches the primary device to the next available channel in the
   *        given direction
   * \param bIncrease True to increase the channel, false to decrease
   */
  bool SwitchChannel(bool bIncrease);

  /*!
   * \brief Returns the number of the current channel on the primary device
   */
  unsigned int CurrentChannel() { return m_primaryDevice ? m_currentChannel : 0; }

  /*!
   * \brief Sets the number of the current channel on the primary device,
   *        without actually switching to it. This can be used to correct the
   *        current channel number while replaying.
   */
  void SetCurrentChannel(const cChannel &channel);

  /*!
   * \brief Closes down all devices. Must be called at the end of the program.
   */
  void Shutdown();

private:
  static int GetClippedNumProvidedSystems(int availableBits, cDevice *device);

  std::vector<cDevice*>      m_devices;
  cDevice                   *m_primaryDevice;
  std::vector<cDeviceHook*>  m_deviceHooks;

  unsigned int m_currentChannel;
  unsigned int m_nextCardIndex; // Card index to give to the next new device
  unsigned int m_useDevice; // Stupid device-use flags
};
