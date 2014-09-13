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

class cSectionScanner : public PLATFORM::CThread
{
public:
  cSectionScanner(cDevice* device, iFilterCallback* callback);
  virtual ~cSectionScanner(void) { }

  /*!
   * Returns true if the scan ran until completion, or false if it was
   * terminated early (even if some data was reported to callback).
   */
  bool WaitForExit(unsigned int timeoutMs);

protected:
  cDevice* const         m_device;
  iFilterCallback* const m_callback;
  volatile bool          m_bSuccess;
  PLATFORM::CEvent       m_exitEvent;
};

class cChannelPropsScanner : public cSectionScanner
{
public:
  cChannelPropsScanner(cDevice* device, iFilterCallback* callback) : cSectionScanner(device, callback) { }
  virtual ~cChannelPropsScanner(void) { }

protected:
  void* Process(void);
};

class cChannelNamesScanner : public cSectionScanner
{
public:
  cChannelNamesScanner(cDevice* device, iFilterCallback* callback) : cSectionScanner(device, callback) { }
  virtual ~cChannelNamesScanner(void) { }

protected:
  void* Process(void);
};

class cEventScanner : public cSectionScanner
{
public:
  cEventScanner(cDevice* device, iFilterCallback* callback) : cSectionScanner(device, callback) { }
  virtual ~cEventScanner(void) { }

protected:
  void* Process(void);
};

class cDeviceScanSubsystem : protected cDeviceSubsystem,
                             public    Observer,
                             public    iFilterCallback
{
public:
  cDeviceScanSubsystem(cDevice* device);
  virtual ~cDeviceScanSubsystem(void) { }

  void StartScan(void);

  bool WaitForTransponderScan(void);
  bool WaitForEPGScan(void);

  virtual void Notify(const Observable &obs, const ObservableMessage msg);

  virtual void OnChannelPropsScanned(const ChannelPtr& channel);
  virtual void OnChannelNamesScanned(const ChannelPtr& channel);
  virtual void OnEventScanned(const EventPtr& event);

private:
  cChannelPropsScanner m_channelPropsScanner;
  cChannelNamesScanner m_channelNamesScanner;
  cEventScanner        m_eventScanner;
};

}
