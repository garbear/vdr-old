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

#include "DeviceChannelSubsystem.h"
#include "DeviceCommonInterfaceSubsystem.h"
#include "DevicePlayerSubsystem.h"
#include "DeviceReceiverSubsystem.h"
#include "DeviceSectionFilterSubsystem.h"
#include "DeviceSPUSubsystem.h"
#include "devices/commoninterface/CI.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "devices/Transfer.h"
#include "Player.h"
#include "utils/log/Log.h"
#include "utils/Tools.h"
#include "utils/I18N.h"

#include <time.h>
#include <algorithm>

using namespace PLATFORM;

namespace VDR
{

#define MAXOCCUPIEDTIMEOUT  99U // max. time (in seconds) a device may be occupied

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) \
  do \
  { \
    delete p; p = NULL; \
  } while (0)
#endif

using namespace std;

cDeviceChannelSubsystem::cDeviceChannelSubsystem(cDevice *device)
 : cDeviceSubsystem(device),
   m_occupiedTimeout(0)
{
}

bool cDeviceChannelSubsystem::SwitchChannel(const ChannelPtr& channel)
{
  // Stop section handling
  SectionFilter()->Stop();
  Receiver()->Stop();

  // Tell the camSlot about the channel switch and add all PIDs of this
  // channel to it, for possible later decryption
  if (CommonInterface()->m_camSlot)
    CommonInterface()->m_camSlot->AddChannel(*channel);

  if (Tune(channel->GetTransponder()))
  {
    // Start section handling
    SectionFilter()->Start();
    Receiver()->Start();

    // Start decrypting any PIDs that might have been set in SetChannelDevice():
    if (CommonInterface()->m_camSlot)
      CommonInterface()->m_camSlot->StartDecrypting();

    SetChanged();
    NotifyObservers(ObservableMessageChannelLock);

    return true;
  }

  return false;
}

void cDeviceChannelSubsystem::ClearChannel(void)
{
  ClearTransponder();
}

unsigned int cDeviceChannelSubsystem::Occupied() const
{
  int seconds = m_occupiedTimeout - time(NULL);
  return seconds > 0 ? seconds : 0;
}

void cDeviceChannelSubsystem::SetOccupied(unsigned int seconds)
{
  m_occupiedTimeout = time(NULL) + min(seconds, MAXOCCUPIEDTIMEOUT);
}

TunerHandlePtr cDeviceChannelSubsystem::Acquire(const ChannelPtr& channel, device_tuning_type_t type, iTunerHandleCallbacks* callbacks)
{
  bool valid(true);
  bool switchNeeded(true);
  TunerHandlePtr handle;
  std::vector<TunerHandlePtr> lowerPrio;

  {
    CLockObject lock(m_mutex);
    for (std::vector<TunerHandlePtr>::iterator it = m_activeTransponders.begin(); valid &&it != m_activeTransponders.end(); ++it)
    {
      if ((*it)->Channel()->GetTransponder() != channel->GetTransponder())
      {
        /** found subscription for another transponder */
        if ((*it)->Type() > type)
        {
          /** subscription with lower prio than this one */
          lowerPrio.push_back(*it);
        }
        else if ((*it)->Type() < type)
        {
          /** subscription with higher prio than this one */
          valid = false;
        }
      }
      else
      {
        switchNeeded = false;
        break;
      }
    }

    if (valid)
    {
      /** remove lower prio registrations */
      for (std::vector<TunerHandlePtr>::iterator it = lowerPrio.begin(); it != lowerPrio.end(); ++it)
      {
        std::vector<TunerHandlePtr>::iterator it2 = std::find(m_activeTransponders.begin(), m_activeTransponders.end(), *it);
        if (it2 != m_activeTransponders.end())
          m_activeTransponders.erase(it2);
      }

      /** add new registration */
      handle = TunerHandlePtr(new cTunerHandle(type, this, callbacks, channel));
      m_activeTransponders.push_back(handle);

      if (switchNeeded)
        SwitchChannel(channel);
    }
  }

  if (valid)
  {
    /** notify lower prio that they lost prio */
    for (std::vector<TunerHandlePtr>::iterator it = lowerPrio.begin(); it != lowerPrio.end(); ++it)
      (*it)->LostPriority();
  }

  return handle;
}

void cDeviceChannelSubsystem::Release(TunerHandlePtr& handle)
{
  CLockObject lock(m_mutex);
  std::vector<TunerHandlePtr>::iterator it = std::find(m_activeTransponders.begin(), m_activeTransponders.end(), handle);
  if (it != m_activeTransponders.end())
    m_activeTransponders.erase(it);
}

void cDeviceChannelSubsystem::Release(cTunerHandle* handle)
{
  CLockObject lock(m_mutex);
  for (std::vector<TunerHandlePtr>::iterator it = m_activeTransponders.begin(); it != m_activeTransponders.end(); ++it)
  {
    if ((*it).get() == handle)
    {
      m_activeTransponders.erase(it);
      break;
    }
  }
}

}
