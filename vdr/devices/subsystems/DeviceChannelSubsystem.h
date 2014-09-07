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

#include "channels/Channel.h" // For cChannel::EmptyChannel
#include "channels/ChannelTypes.h"
#include "devices/DeviceSubsystem.h"
#include "devices/Device.h"
#include "devices/TunerHandle.h"
#include "transponders/TransponderTypes.h"
#include "utils/Observer.h"
#include "Config.h" // For IDLEPRIORITY

namespace VDR
{

class cDeviceChannelSubsystem : protected cDeviceSubsystem, public Observable
{
public:
  cDeviceChannelSubsystem(cDevice *device);
  virtual ~cDeviceChannelSubsystem() { }

  /*!
   * \brief Returns true if this device can provide the given source
   */
  virtual bool ProvidesSource(TRANSPONDER_TYPE source) const { return false; }

  /*!
   * \brief Returns true if this device can provide the transponder of the given
   *        Channel (which implies that it can provide the Channel's source)
   */
  virtual bool ProvidesTransponder(const cChannel &channel) const { return false; }

  /*!
   * \brief Returns true if this device can provide the given channel
   *
   * \param needsDetachReceivers If given, the resulting value in it will tell
   *        the caller whether or not it will have to detach any currently
   *        attached receivers from this device before calling SwitchChannel.
   *        Note that the return value in NeedsDetachReceivers is only
   *        meaningful if the function itself actually returns true.
   * \return The default implementation always returns false, so a derived
   *         cDevice class able to provide channels must implement this function
   */
  virtual bool ProvidesChannel(const cChannel &channel, bool *needsDetachReceivers = NULL) const { return false; }

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
  virtual cTransponder GetCurrentlyTunedTransponder() const { return cTransponder::EmptyTransponder; }

  /*!
   * \brief Returns true if this device is currently tuned to the given Channel's transponder
   */
  virtual bool IsTunedToTransponder(const cChannel &channel) const { return false; }

  /*!
   * \brief Switches the device to the given Channel (actual physical setup).
   *        Blocks until the channel is successfully set or the action fails
   *        (perhaps due to a timeout).
   */
  bool SwitchChannel(const ChannelPtr& channel);

  void ClearChannel(void);

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
  virtual bool HasLock(void) const { return true; }

  bool TuningAllowed(device_tuning_type_t type, const cTransponder& transponder);

  void Release(TunerHandlePtr& handle);
  void Release(cTunerHandle* handle);
  TunerHandlePtr Acquire(const ChannelPtr& channel, device_tuning_type_t type, iTunerHandleCallbacks* callbacks);

protected:
  /*!
   * \brief Sets the device to the given channel (actual physical setup). Blocks
   *        until the channel is successfully set or the action fails (perhaps
   *        due to a timeout).
   */
  virtual bool Tune(const cTransponder& transponder) = 0;
  virtual void ClearTransponder(void) = 0;

private:
  time_t               m_occupiedTimeout;
  std::vector<TunerHandlePtr> m_activeTransponders;
  PLATFORM::CMutex            m_mutex;
};
}
