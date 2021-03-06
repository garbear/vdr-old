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

#include "Device.h"

#include "DeviceManager.h"
#include "TunerHandle.h"
#include "devices/commoninterface/CI.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceCommonInterfaceSubsystem.h"
#include "devices/subsystems/DeviceImageGrabSubsystem.h"
#include "devices/subsystems/DevicePlayerSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "devices/subsystems/DeviceSPUSubsystem.h"
#include "devices/subsystems/DeviceTrackSubsystem.h"
#include "devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "dvb/filters/PSIP_MGT.h"
#include "epg/EPGTypes.h"
#include "epg/Event.h"
#include "utils/log/Log.h"

using namespace std;

namespace VDR
{

const DevicePtr cDevice::EmptyDevice;

#define safe_delete(x) do { delete x; x = NULL; } while(0)

// --- cSubsystems -----------------------------------------------------------
void cSubsystems::Free()
{
  safe_delete(CommonInterface);
  safe_delete(ImageGrab);
  safe_delete(Player);
  safe_delete(Scan);
  safe_delete(SPU);
  safe_delete(Track);
  safe_delete(VideoFormat);
  safe_delete(Receiver);
  safe_delete(Channel);
}

void cSubsystems::AssertValid() const
{
  assert(Channel);
  assert(CommonInterface);
  assert(ImageGrab);
  assert(Player);
  assert(Receiver);
  assert(Scan);
  assert(SPU);
  assert(Track);
  assert(VideoFormat);
}

// --- cDevice ---------------------------------------------------------------

cDevice::cDevice(const cSubsystems &subsystems)
 : m_subsystems(subsystems),
   m_index(0),
   m_bInitialised(false)
{
  m_subsystems.AssertValid();
  Channel()->RegisterObserver(Scan());
}

cDevice::~cDevice()
{
  Deinitialise();
  m_subsystems.Free(); // TODO: Remove me if we switch cSubsystems to use shared_ptrs
}

bool cDevice::Initialise(unsigned int index)
{
  m_index = index;
  Receiver()->Start();
  m_bInitialised = true;
  return m_bInitialised;
}

void cDevice::Deinitialise(void)
{
  if (m_bInitialised)
  {
    m_bInitialised = false;
    Channel()->UnregisterObserver(Scan());
    Receiver()->Stop();
  }
}

bool cDevice::CanTune(device_tuning_type_t type)
{
  return Channel()->CanTune(type);
}

TunerHandlePtr cDevice::Acquire(const ChannelPtr& channel, device_tuning_type_t type, iTunerHandleCallbacks* callbacks)
{
  return Channel()->Acquire(this, channel, type, callbacks);
}

void cDevice::Release(TunerHandlePtr& handle)
{
  Channel()->Release(handle);
}

void cDevice::Release(cTunerHandle* handle, bool notify /* = true */)
{
  Channel()->Release(handle, notify);
}

}
