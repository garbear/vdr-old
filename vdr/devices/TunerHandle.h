#pragma once
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

#include "channels/Channel.h"
#include "channels/ChannelTypes.h"
#include "devices/DeviceTypes.h"
#include <memory>

namespace VDR
{
class cDeviceChannelSubsystem;
class iReceiver;

// prio high -> low
typedef enum
{
  TUNING_TYPE_RECORDING,
  TUNING_TYPE_LIVE_TV,
  TUNING_TYPE_CHANNEL_SCAN,
  TUNING_TYPE_EPG_SCAN,
  TUNING_TYPE_NONE,
} device_tuning_type_t;

class iTunerHandleCallbacks
{
public:
  virtual ~iTunerHandleCallbacks(void) {}

  virtual void LockAcquired(void) = 0;
  virtual void LockLost(void) = 0;
  virtual void LostPriority(void) = 0;
};

class cDevice;
class cTunerHandle;
typedef std::shared_ptr<cTunerHandle> TunerHandlePtr;

class cTunerHandle
{
public:
  cTunerHandle(device_tuning_type_t type, cDevice* tuner, iTunerHandleCallbacks* callbacks, const ChannelPtr& channel);
  virtual ~cTunerHandle(void);

  ChannelPtr Channel(void) const { return m_channel; }
  device_tuning_type_t Type(void) const { return m_type; }
  std::string ToString(void) const;
  void LockAcquired(void);
  void LockLost(void);
  void LostPriority(void);
  bool SignalQuality(signal_quality_info_t& info) const;

  void Release(bool notify = true);
  void StartEPGScanAfterRelease(bool setto) { m_startEpgScan = setto; }
  void StartChannelScanAfterRelease(bool setto) { m_startChannelScan = setto; }

  bool AttachMultiplexedReceiver(iReceiver* receiver, const ChannelPtr& channel);
  bool AttachMultiplexedReceiver(iReceiver* receiver, uint16_t pid, STREAM_TYPE type = STREAM_TYPE_UNDEFINED);
  bool AttachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask);
  void DetachStreamingReceiver(iReceiver* receiver, uint16_t pid, uint8_t tid, uint8_t mask, bool wait);
  void DetachReceiver(iReceiver* receiver, bool wait);

  static const TunerHandlePtr EmptyHandle;

private:
  device_tuning_type_t   m_type;
  cDevice*               m_tuner;
  iTunerHandleCallbacks* m_callbacks;
  const ChannelPtr&      m_channel;
  bool                   m_startEpgScan;
  bool                   m_startChannelScan;
  PLATFORM::CMutex       m_mutex;
  std::set<iReceiver*>   m_receivers;
};

}
