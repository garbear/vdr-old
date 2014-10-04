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
#include "dvb/filters/PSIP_STT.h"
#include "dvb/filters/PSIP_VCT.h"
#include "dvb/filters/SDT.h"
#include "epg/Event.h"
#include "epg/ScheduleManager.h"
#include "lib/platform/util/timeutils.h"

#include <algorithm>

using namespace PLATFORM;
using namespace std;

// TODO: This can be shorted by fetching PMT tables in parallel
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

class cChannelNamesScanner : public cSectionScanner
{
public:
  cChannelNamesScanner(cDevice* device, iFilterCallback* callback) : cSectionScanner(device, callback),  m_sdt(NULL), m_bAbort(false) { }
  virtual ~cChannelNamesScanner(void) { Abort(); delete m_sdt;}
  void Abort(void);
  void Start(void);

protected:
  void* Process(void);
  bool      m_bAbort;
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
 * cChannelNamesScanner
 */
void* cChannelNamesScanner::Process(void)
{
  m_bSuccess = true;
  m_bAbort   = false;

  if (m_bSuccess && !m_bAbort) {
    m_sdt->ScanChannels();
  }

  cChannelManager::Get().NotifyObservers();

  return NULL;
}

void cChannelNamesScanner::Start(void)
{
  StopThread(0);
  if (m_sdt)
    delete m_sdt;
  m_sdt = new cSdt(m_device);
  CreateThread(true);
}

void cChannelNamesScanner::Abort(void)
{
  m_bAbort = true;
  if (m_sdt)
    m_sdt->Abort();
  StopThread(-1);
}

/*
 * cEventScanner
 */
void* cEventScanner::Process(void)
{
//  m_bSuccess = m_mgt->ScanPSIPData(m_callback);

  cScheduleManager::Get().NotifyObservers();

  return NULL;
}

void cEventScanner::Start(void)
{
  StopThread(0);
  if (m_mgt)
    delete m_mgt;
  m_mgt = new cPsipMgt(m_device);
  CreateThread(true);
}

void cEventScanner::Abort(void)
{
//  if (m_mgt)
//    m_mgt->Abort();
  StopThread(-1);
}

/*
 * cDeviceScanSubsystem
 */
cDeviceScanSubsystem::cDeviceScanSubsystem(cDevice* device)
 : cDeviceSubsystem(device),
   m_channelNamesScanner(new cChannelNamesScanner(device, this)),
   m_eventScanner(new cEventScanner(device, this)),
   m_pat(new cPat(device)),
   m_mgt(new cPsipMgt(device)),
   m_stt(new cPsipStt(device)),
   m_vct(new cPsipVct(device))
{
}

cDeviceScanSubsystem::~cDeviceScanSubsystem(void)
{
  delete m_channelNamesScanner;
  delete m_eventScanner;
  delete m_pat;
  delete m_mgt;
  delete m_stt;
  delete m_vct;
}

bool cDeviceScanSubsystem::AttachReceivers(void)
{
  bool retval = true;
  retval &= PAT()->Attach();
  retval &= MGT()->Attach();
  retval &= STT()->Attach();
  retval &= VCT()->Attach();
  return retval;
}

void cDeviceScanSubsystem::DetachReceivers(void)
{
  PAT()->Detach();
  MGT()->Detach();
  STT()->Detach();
  VCT()->Detach();
}

void cDeviceScanSubsystem::StartScan()
{
  m_channelNamesScanner->Start();
  m_eventScanner->Start();
  AttachReceivers();
}

void cDeviceScanSubsystem::StopScan()
{
  DetachReceivers();
  m_channelNamesScanner->Abort();
  m_eventScanner->Abort();
}

bool cDeviceScanSubsystem::WaitForTransponderScan(void)
{
  return m_pat->WaitForScan() &&
      m_mgt->WaitForScan();
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
