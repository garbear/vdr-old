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
#include "devices/subsystems/DeviceChannelSubsystem.h"
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

/*
 * cDeviceScanSubsystem
 */
cDeviceScanSubsystem::cDeviceScanSubsystem(cDevice* device)
 : cDeviceSubsystem(device),
   m_pat(new cPat(device)),
   m_eit(new cEit(device)),
   m_mgt(new cPsipMgt(device)),
   m_sdt(new cSdt(device)),
   m_psipeit(new cPsipEit(device)),
   m_psipstt(new cPsipStt(device)),
   m_locked(false),
   m_type(TRANSPONDER_INVALID),
   m_device(device),
   m_scanFinished(true)
{
  m_receivers.insert(m_pat);
  m_receivers.insert(m_eit);
  m_receivers.insert(m_mgt);
  m_receivers.insert(m_psipeit);
  m_receivers.insert(m_psipstt);
  m_receivers.insert(m_sdt);
  m_receivers.insert(new cPsipVct(device));
}

cDeviceScanSubsystem::~cDeviceScanSubsystem(void)
{
  for (std::set<cScanReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
    delete *it;
}

void cDeviceScanSubsystem::SetScanned(cScanReceiver* receiver)
{
  CLockObject lock(m_waitingMutex);
  std::set<cScanReceiver*>::iterator it = m_waitingForReceivers.find(receiver);
  if (it != m_waitingForReceivers.end())
  {
    m_waitingForReceivers.erase(it);
    m_scanFinished = m_waitingForReceivers.empty();
  }

  if (m_scanFinished)
    m_waitingCondition.Broadcast();
}

void cDeviceScanSubsystem::ResetScanned(cScanReceiver* receiver)
{
  CLockObject lock(m_waitingMutex);
  if (receiver->InChannelScan())
  {
    if (m_waitingForReceivers.find(receiver) == m_waitingForReceivers.end())
    {
      m_waitingForReceivers.insert(receiver);
      m_scanFinished = false;
    }
  }
}

bool cDeviceScanSubsystem::AttachReceivers(void)
{
  bool retval(true);
  for (std::set<cScanReceiver*>::iterator it = m_receivers.begin(); it != m_receivers.end(); ++it)
  {
    if (ReceiverOk(*it))
    {
      retval &= (*it)->Attach();

      if ((*it)->InChannelScan())
      {
        CLockObject lock(m_waitingMutex);
        m_scanFinished = false;
        m_waitingForReceivers.insert(*it);
      }
    }
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

  CLockObject lock(m_waitingMutex);
  m_scanFinished = m_waitingForReceivers.empty();
  m_waitingForReceivers.clear();
  m_waitingCondition.Broadcast();
}

/**
 * Wait for lock
 * blocks until all receivers are attached and resources are opened
 * @param iTimeoutMs  timeout in milliseconds
 * @return true if receivers were attached, false if not
 */
bool cDeviceScanSubsystem::WaitForLock(uint32_t iTimeoutMs)
{
  CLockObject lock(m_waitingMutex);
  return m_lockCondition.Wait(m_waitingMutex, m_locked, iTimeoutMs);
}

bool cDeviceScanSubsystem::WaitForTransponderScan(void)
{
  if (WaitForLock(TRANSPONDER_TIMEOUT))
  {
    CLockObject lock(m_waitingMutex);
    return m_waitingCondition.Wait(m_waitingMutex, m_scanFinished, TRANSPONDER_TIMEOUT);
  }

  return false;
}

bool cDeviceScanSubsystem::WaitForEPGScan(void)
{
  if (WaitForLock(TRANSPONDER_TIMEOUT))
  {
    if (Channel()->ProvidesSource(TRANSPONDER_ATSC))
    {
      CTimeout timeout(EPG_TIMEOUT);
      if (m_mgt->WaitForScan(timeout.TimeLeft()))
      {
        uint32_t timeLeft = timeout.TimeLeft();
        if (timeLeft > 0)
          return m_psipeit->WaitForScan(timeLeft);
      }
    }
    else
    {
      return m_eit->WaitForScan(EPG_TIMEOUT);
    }
  }

  return false;
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

bool cDeviceScanSubsystem::ReceiverOk(cScanReceiver* receiver)
{
  if (receiver)
  {
    if (Channel()->ProvidesSource(TRANSPONDER_ATSC) && receiver->InATSC())
      return true;
    if (!Channel()->ProvidesSource(TRANSPONDER_ATSC) && receiver->InDVB())
      return true;
  }
  return false;
}

void cDeviceScanSubsystem::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageChannelLock:
  {
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
  cScheduleManager::Get().AddEvent(event, m_device->Channel()->GetCurrentlyTunedTransponder());
}

unsigned int cDeviceScanSubsystem::GetGpsUtcOffset(void)
{
  return Channel()->ProvidesSource(TRANSPONDER_ATSC) ? m_psipstt->GetGpsUtcOffset() : 0;
}

}
