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
#include "devices/linux/DVBDevice.h"
#include "dvb/filters/EIT.h"
#include "dvb/filters/PAT.h"
#include "dvb/filters/PSIP_MGT.h"
#include "dvb/filters/PSIP_EIT.h"
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

/*
 * cDeviceScanSubsystem
 */
cDeviceScanSubsystem::cDeviceScanSubsystem(cDevice* device)
 : cDeviceSubsystem(device),
   m_pat(new cPat(device)),
   m_eit(new cEit(device)),
   m_mgt(new cPsipMgt(device)),
   m_psipeit(new cPsipEit(device)),
   m_psipstt(new cPsipStt(device)),
   m_locked(false),
   m_type(TRANSPONDER_INVALID),
   m_device(device)
{
  m_receivers.insert(m_pat);
  m_receivers.insert(m_eit);
  m_receivers.insert(m_mgt);
  m_receivers.insert(m_psipeit);
  m_receivers.insert(m_psipstt);
  m_receivers.insert(new cSdt(device));
  m_receivers.insert(new cPsipVct(device));
}

cDeviceScanSubsystem::~cDeviceScanSubsystem(void)
{
  for (std::set<cScanReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
    delete *it;
}

TRANSPONDER_TYPE cDeviceScanSubsystem::Type(void)
{
  if (m_type == TRANSPONDER_INVALID)
  {
    //XXX
    cDvbDevice* dvbDevice = static_cast<cDvbDevice*>(m_device);
    if (dvbDevice)
    {
      if (dvbDevice->Channel()->ProvidesSource(TRANSPONDER_ATSC))
        m_type = TRANSPONDER_ATSC;
      else if (dvbDevice->Channel()->ProvidesSource(TRANSPONDER_CABLE))
        m_type = TRANSPONDER_CABLE;
      else if (dvbDevice->Channel()->ProvidesSource(TRANSPONDER_SATELLITE))
        m_type = TRANSPONDER_SATELLITE;
      else if (dvbDevice->Channel()->ProvidesSource(TRANSPONDER_TERRESTRIAL))
        m_type = TRANSPONDER_TERRESTRIAL;
    }
  }
  return m_type;
}

bool cDeviceScanSubsystem::AttachReceivers(void)
{
  bool retval(true);
  for (std::set<cScanReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
  {
    if (ReceiverOk(*it))
      retval &= (*it)->Attach();
  }
  return retval;
}

void cDeviceScanSubsystem::DetachReceivers(void)
{
  for (std::set<cScanReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
  {
    if (ReceiverOk(*it))
      (*it)->Detach();
  }
}

void cDeviceScanSubsystem::StartScan()
{
  AttachReceivers();
}

void cDeviceScanSubsystem::StopScan()
{
  DetachReceivers();
}

bool cDeviceScanSubsystem::WaitForTransponderScan(void)
{
  return Type() == TRANSPONDER_ATSC ? m_mgt->WaitForScan() : m_pat->WaitForScan();
}

bool cDeviceScanSubsystem::WaitForEPGScan(void)
{
  return Type() == TRANSPONDER_ATSC ? m_psipeit->WaitForScan(EPG_TIMEOUT) : m_eit->WaitForScan(EPG_TIMEOUT);
}

void cDeviceScanSubsystem::LockAcquired(void)
{
  for (std::set<cScanReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
  {
    if (ReceiverOk(*it))
      (*it)->LockAcquired();
  }
}

void cDeviceScanSubsystem::LockLost(void)
{
  for (std::set<cScanReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
  {
    if (ReceiverOk(*it))
      (*it)->LockLost();
  }
}

bool cDeviceScanSubsystem::ReceiverOk(cScanReceiver* receiver) const
{
  return receiver && ((m_type == TRANSPONDER_ATSC && receiver->InATSC()) || (m_type != TRANSPONDER_ATSC && receiver->InDVB()));
}

void cDeviceScanSubsystem::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageChannelLock:
  {
    StartScan();
    LockAcquired();
    PLATFORM::CLockObject lock(m_mutex);
    m_locked = true;
    m_lockCondition.Signal();
    break;
  }
  case ObservableMessageChannelLostLock:
  {
    LockLost();
    PLATFORM::CLockObject lock(m_mutex);
    m_locked = false;
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

unsigned int cDeviceScanSubsystem::GetGpsUtcOffset(void)
{
  return Type() == TRANSPONDER_ATSC ? m_psipstt->GetGpsUtcOffset() : 0;
}

void cDeviceScanSubsystem::AttachEITPids(const vector<uint16_t>& pids)
{
  if (Type() == TRANSPONDER_ATSC)
    m_psipeit->AttachPids(pids);
  //TODO
}

}
