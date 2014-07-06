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

#include "channels/Channel.h"
#include "channels/ChannelID.h"
#include "platform/threads/mutex.h"
#include "Config.h"

#include <set>
#include <stdint.h>

namespace VDR
{
class cDevice;
class cDevicePIDSubsystem;

class cReceiver {
  friend class cDevice;
private:
  PLATFORM::CMutex   m_mutex;
  cChannelID         m_channelID;
  cDevice*           m_device;
  std::set<uint16_t> m_pids;

protected:
  void Detach(void);

public: // TODO
  bool WantsPid(int Pid);
  bool AddToPIDSubsystem(cDevicePIDSubsystem* pidSys) const;
  void RemoveFromPIDSubsystem(cDevicePIDSubsystem* pidSys) const;
  bool DeviceAttached(cDevice* device) const;
  void AttachDevice(cDevice* device);
  void DetachDevice(void);

  virtual void Activate(bool On) {}
               ///< This function is called just before the cReceiver gets attached to
               ///< (On == true) and right after it gets detached from (On == false) a cDevice. It can be used
               ///< to do things like starting/stopping a thread.
               ///< It is guaranteed that Receive() will not be called before Activate(true).
protected:
  virtual void Receive(uint8_t *Data, int Length) = 0;
               ///< This function is called from the cDevice we are attached to, and
               ///< delivers one TS packet from the set of PIDs the cReceiver has requested.
               ///< The data packet must be accepted immediately, and the call must return
               ///< as soon as possible, without any unnecessary delay. Each TS packet
               ///< will be delivered only ONCE, so the cReceiver must make sure that
               ///< it will be able to buffer the data if necessary.
public:
  cReceiver(const ChannelPtr& Channel = cChannel::EmptyChannel);
               ///< Creates a new receiver for the given Channel
               ///< If Channel is not NULL, its pids are set by a call to SetPids().
               ///< Otherwise pids can be added to the receiver by separate calls to the AddPid[s]
               ///< functions.
               ///< The total number of PIDs added to a receiver must not exceed MAXRECEIVEPIDS.
               ///< Priority may be any value in the range MINPRIORITY...MAXPRIORITY. Negative values indicate
               ///< that this cReceiver may be detached at any time in favor of a timer recording
               ///< or live viewing (without blocking the cDevice it is attached to).
  virtual ~cReceiver();
  void AddPid(uint16_t pid);
               ///< Adds the given pid to the list of PIDs of this receiver.
  void AddPids(const std::set<uint16_t>& pids);
               ///< Adds the given list of pids to the list of PIDs of this receiver.
  void UpdatePids(const std::set<uint16_t>& pids);
  void SetPids(const cChannel& Channel);
               ///< Sets the PIDs of this receiver to those of the given Channel,
               ///< replacing any previously stored PIDs. If Channel is NULL, all
               ///< PIDs will be cleared. Parameters in the Setup may control whether
               ///< certain types of PIDs (like Dolby Digital, for instance) are
               ///< actually set. The Channel's ID is stored and can later be retrieved
               ///< through ChannelID(). The ChannelID is necessary to allow the device
               ///< that will be used for this receiver to detect and store whether the
               ///< channel can be decrypted in case this is an encrypted channel.
  cChannelID ChannelID(void) const;
  bool IsAttached(void) const;
               ///< Returns true if this receiver is (still) attached to a device.
               ///< A receiver may be automatically detached from its device in
               ///< case the device is needed otherwise, so code that uses a cReceiver
               ///< should repeatedly check whether it is still attached, and if
               ///< it isn't, delete it (or take any other appropriate measures).
  };

}
