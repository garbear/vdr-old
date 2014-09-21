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

#include "DeviceScanSubsystem.h"
#include "DeviceChannelSubsystem.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "dvb/filters/PAT.h"
#include "dvb/filters/PSIP_MGT.h"
#include "dvb/filters/PSIP_VCT.h"
#include "dvb/filters/SDT.h"
#include "epg/Event.h"
#include "epg/ScheduleManager.h"
#include "lib/platform/util/timeutils.h"

#include <algorithm>

using namespace PLATFORM;
using namespace std;

// TODO: This can be shorted by fetching PMT tables in parallel
#define TRANSPONDER_TIMEOUT 5000
#define EPG_TIMEOUT         10000

namespace VDR
{
class cSectionScanner : protected PLATFORM::CThread
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
  cChannelPropsScanner(cDevice* device, iFilterCallback* callback) : cSectionScanner(device, callback), m_pat(NULL) { }
  virtual ~cChannelPropsScanner(void) { Abort(); delete m_pat; }
  void Start(void);
  void Abort(void);

protected:
  void* Process(void);
  cPat* m_pat;
};

class cChannelNamesScanner : public cSectionScanner
{
public:
  cChannelNamesScanner(cDevice* device, iFilterCallback* callback) : cSectionScanner(device, callback), m_vct(NULL), m_sdt(NULL), m_bAbort(false) { }
  virtual ~cChannelNamesScanner(void) { Abort(); delete m_vct; delete m_sdt;}
  void Abort(void);
  void Start(void);

protected:
  void* Process(void);
  bool      m_bAbort;
  cPsipVct* m_vct;
  cSdt*     m_sdt;
};

class cEventScanner : public cSectionScanner
{
public:
  cEventScanner(cDevice* device, iFilterCallback* callback) : cSectionScanner(device, callback), m_mgt(NULL) { }
  virtual ~cEventScanner(void) { Abort(); delete m_mgt; }
  void Abort(void);
  void Start(void);

protected:
  void* Process(void);
  cPsipMgt* m_mgt;
};

/*
 * cSectionScanner
 */
cSectionScanner::cSectionScanner(cDevice* device, iFilterCallback* callback)
 : m_device(device),
   m_callback(callback),
   m_bSuccess(true)
{
}

bool cSectionScanner::WaitForExit(unsigned int timeoutMs)
{
  StopThread(timeoutMs);
  return m_bSuccess;
}

/*
 * cChannelPropsScanner
 */
void* cChannelPropsScanner::Process(void)
{
  m_bSuccess = m_pat->ScanChannels(m_callback);
  cChannelManager::Get().NotifyObservers();

  return NULL;
}

void cChannelPropsScanner::Start(void)
{
  if (!m_pat)
    m_pat = new cPat(m_device);
  CreateThread(false);
}

void cChannelPropsScanner::Abort(void)
{
  if (m_pat)
    m_pat->Abort();
  StopThread(0);
}

/*
 * cChannelNamesScanner
 */
void* cChannelNamesScanner::Process(void)
{
  m_bSuccess = false;
  m_bAbort   = false;

  // TODO: Use SDT for non-ATSC tuners
  m_bSuccess = m_vct->ScanChannels(m_callback);

  if (m_bSuccess && !m_bAbort) {
    m_sdt->ScanChannels();
  }

  cChannelManager::Get().NotifyObservers();

  return NULL;
}

void cChannelNamesScanner::Start(void)
{
  if (!m_vct)
    m_vct = new cPsipVct(m_device);
  if (!m_sdt)
    m_sdt = new cSdt(m_device);
  CreateThread(false);
}

void cChannelNamesScanner::Abort(void)
{
  m_bAbort = true;
  if (m_vct)
    m_vct->Abort();
  if (m_sdt)
    m_sdt->Abort();
  StopThread(0);
}

/*
 * cEventScanner
 */
void* cEventScanner::Process(void)
{
  m_bSuccess = m_mgt->ScanPSIPData(m_callback);

  cScheduleManager::Get().NotifyObservers();

  return NULL;
}

void cEventScanner::Start(void)
{
  if (!m_mgt)
    m_mgt = new cPsipMgt(m_device);
  CreateThread(false);
}

void cEventScanner::Abort(void)
{
  if (m_mgt)
    m_mgt->Abort();
  StopThread(0);
}

/*
 * cDeviceScanSubsystem
 */
cDeviceScanSubsystem::cDeviceScanSubsystem(cDevice* device)
 : cDeviceSubsystem(device),
   m_channelPropsScanner(new cChannelPropsScanner(device, this)),
   m_channelNamesScanner(new cChannelNamesScanner(device, this)),
   m_eventScanner(new cEventScanner(device, this))
{
}

cDeviceScanSubsystem::~cDeviceScanSubsystem(void)
{
  delete m_channelNamesScanner;
  delete m_channelPropsScanner;
  delete m_eventScanner;
}

void cDeviceScanSubsystem::StartScan()
{
  m_channelPropsScanner->Start();
  m_channelNamesScanner->Start();
  m_eventScanner->Start();
}

void cDeviceScanSubsystem::StopScan()
{
  m_channelPropsScanner->Abort();
  m_channelNamesScanner->Abort();
  m_eventScanner->Abort();
}

bool cDeviceScanSubsystem::WaitForTransponderScan(void)
{
  const int64_t startMs = GetTimeMs();
  const int64_t timeoutMs = TRANSPONDER_TIMEOUT;

  if (m_channelPropsScanner->WaitForExit(timeoutMs))
  {
    const int64_t duration = GetTimeMs() - startMs;
    const int64_t timeLeft = timeoutMs - duration;
    return m_channelNamesScanner->WaitForExit(std::max(timeLeft, (int64_t)1));
  }

  return false;
}

bool cDeviceScanSubsystem::WaitForEPGScan(void)
{
  const int64_t startMs = GetTimeMs();
  const int64_t timeoutMs = EPG_TIMEOUT;

  return m_eventScanner->WaitForExit(timeoutMs);
}

void cDeviceScanSubsystem::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageChannelLock:
  {
    StartScan();
    break;
  }
  case ObservableMessageChannelLostLock:
  {
    StopScan();
    break;
  }
  default:
    break;
  }
}

void cDeviceScanSubsystem::OnChannelPropsScanned(const ChannelPtr& channel)
{
  cChannelManager::Get().MergeChannelProps(channel);
}

void cDeviceScanSubsystem::OnChannelNamesScanned(const ChannelPtr& channel)
{
  cChannelManager::Get().MergeChannelNamesAndModulation(channel);
}

void cDeviceScanSubsystem::OnEventScanned(const EventPtr& event)
{
  cScheduleManager::Get().AddEvent(event);
}

}
