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
#include "Config.h" // For IDLEPRIORITY

enum eSetChannelResult
{
  scrOk,
  scrNotAvailable,
  scrNoTransfer,
  scrFailed
};

class cChannel;

class cDeviceChannelSubsystem : protected cDeviceSubsystem
{
public:
  cDeviceChannelSubsystem(cDevice *device);
  virtual ~cDeviceChannelSubsystem() { }

  /*!
   * \brief Returns true if this device can provide the given source
   */
  virtual bool ProvidesSource(int source) const { return false; }

  /*!
   * \brief Returns true if this device can provide the transponder of the given
   *        Channel (which implies that it can provide the Channel's source)
   */
  virtual bool ProvidesTransponder(const cChannel &channel) const { return false; }

  /*!
   * \brief Returns true if this is the only device that is able to provide the
   *        given channel's transponder
   */
  virtual bool ProvidesTransponderExclusively(const cChannel &channel) const;

  /*!
   * \brief Returns true if this device can provide the given channel
   *
   * \param priority Used to device whether the caller's request can be honored
   *        Only used if the device has cReceivers attached to it. The special
   *        priority value IDLEPRIORITY will tell the caller whether this device
   *        is principally able to provide the given channel, regardless of any
   *        attached cReceivers.
   * \param needsDetachReceivers If given, the resulting value in it will tell
   *        the caller whether or not it will have to detach any currently
   *        attached receivers from this device before calling SwitchChannel.
   *        Note that the return value in NeedsDetachReceivers is only
   *        meaningful if the function itself actually returns true.
   * \return The default implementation always returns false, so a derived
   *         cDevice class able to provide channels must implement this function
   */
  virtual bool ProvidesChannel(const cChannel &channel, int priority = IDLEPRIORITY, bool *needsDetachReceivers = NULL) const { return false; }

  /*!
   * \brief Returns true if this device provides EIT data and thus wants to be
   *        tuned to the channels it can receive regularly to update the data
   * \return The default implementation returns false
   */
  virtual bool ProvidesEIT() const { return false; }

  /*!
   * \brief Returns the number of individual "delivery systems" this device provides
   * \return The default implementation returns 0, so any derived class that can
   *         actually provide channels must implement this function. The result
   *         of this function is used when selecting a device, in order to avoid
   *         devices that provide more than one system.
   */
  virtual unsigned int NumProvidedSystems() const { return 0; }

  /*!
   * \brief Returns the "strength" of the currently received signal
   * \return A value in the range 0 (no signal at all) through 100 (best
   *         possible signal). A value of -1 indicates that this device has no
   *         concept of a "signal strength".
   */
  virtual int SignalStrength() const { return -1; }

  /*!
   * \brief Returns the "quality" of the currently received signal
   * \return A value in the range 0 (worst quality) through 100 (best possible
   *         quality). A value of -1 indicates that this device has no concept
   *         of a "signal quality".
   */
  virtual int SignalQuality() const { return -1; }

  /*!
   * \brief Returns a pointer to the currently tuned transponder
   * \return A local copy of one of the channels in the global cChannels list.
   *         May be NULL if the device is not tuned to any transponder.
   */
  virtual const cChannel *GetCurrentlyTunedTransponder() const { return NULL; }

  /*!
   * \brief Returns true if this device is currently tuned to the given Channel's transponder
   */
  virtual bool IsTunedToTransponder(const cChannel &channel) const { return false; }

  /*!
   * \brief Returns true if it is ok to switch to the Channel's transponder on
   *        this device, without disturbing any other activities
   * \return False if an occupied timeout has been set for this device and that timeout has not yet expired
   */
  virtual bool MaySwitchTransponder(const cChannel &channel) const;

  /*!
   * \brief Switches the device to the given Channel, initiating transfer mode
   *        if necessary.
   */
  bool SwitchChannel(const cChannel &channel);

  /*!
   * \brief Returns the number of seconds this device is still occupied for
   */
  unsigned int Occupied() const;

  /*!
   * \brief Sets the occupied timeout for this device to the given number of seconds
   * \param seconds Values greater than MAXOCCUPIEDTIMEOUT will silently be ignored
   *
   * This can be used to tune a device to a particular transponder and make sure
   * it will stay there for a certain amount of time, for instance to collect
   * EPG data. This function shall only be called after the device has been
   * successfully tuned to the requested transponder.
   */
  void SetOccupied(unsigned int seconds);

  /*!
   * \brief Returns true if the device has a lock on the requested transponder
   * \param timeoutMs The timeout. If > 0, waits for the given number of ms before returning false
   * \return Default is true, a specific device implementation may return false
   *         to indicate that it is not ready yet.
   */
  virtual bool HasLock(unsigned int timeoutMs = 0) const { return true; }

  /*!
   * \brief Returns true if the device is currently showing any programme to the
   *        user, either through replaying or live
   */
  virtual bool HasProgramme() const;

protected:
  /*!
   * \brief Sets the device to the given channel (actual physical setup)
   */
  virtual bool SetChannelDevice(const cChannel &channel) { return false; }

private:
  /*!
   * \brief Sets the device to the given channel (general setup)
   */
  eSetChannelResult SetChannel(const cChannel &Channel);

  time_t     m_occupiedTimeout;
};
