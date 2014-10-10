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

#include "devices/DeviceSubsystem.h"
#include "dvb/filters/Filter.h"
#include "lib/platform/threads/mutex.h"
#include "lib/platform/threads/threads.h"
#include "transponders/Transponder.h"
#include "utils/Observer.h"

namespace VDR
{
class cScanReceiver;
class cEit;
class cPat;
class cSdt;
class cPsipMgt;
class cPsipEit;
class cPsipStt;

class cDeviceScanSubsystem : protected cDeviceSubsystem,
                             public    Observer,
                             public    iFilterCallback
{
public:
  cDeviceScanSubsystem(cDevice* device);
  virtual ~cDeviceScanSubsystem(void);

  void StartScan(void);
  void StopScan(void);

  bool WaitForTransponderScan(void);
  bool WaitForEPGScan(void);
  bool AttachReceivers(void);
  void DetachReceivers(void);

  virtual void Notify(const Observable &obs, const ObservableMessage msg);

  virtual void OnChannelPropsScanned(const ChannelPtr& channel);
  virtual void OnChannelNamesScanned(const ChannelPtr& channel);
  virtual void OnEventScanned(const EventPtr& event);

  unsigned int GetGpsUtcOffset(void);
  void AttachEITPids(const std::vector<uint16_t>& pids);

  TRANSPONDER_TYPE Type(void);
  void SetType(TRANSPONDER_TYPE type) { m_type = type; }

  cSdt* SDT(void) const { return m_sdt; }

private:
  void LockAcquired(void);
  void LockLost(void);
  bool ReceiverOk(cScanReceiver* receiver);

  cPat*                      m_pat;
  cEit*                      m_eit;
  cSdt*                      m_sdt;
  cPsipMgt*                  m_mgt;
  cPsipEit*                  m_psipeit;
  cPsipStt*                  m_psipstt;
  std::set<cScanReceiver*>   m_receivers;
  PLATFORM::CMutex           m_mutex;
  PLATFORM::CCondition<bool> m_lockCondition;
  bool                       m_locked;
  TRANSPONDER_TYPE           m_type;
  cDevice*                   m_device;
};

}
