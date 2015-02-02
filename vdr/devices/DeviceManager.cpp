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

#include "DeviceManager.h"
#include "Transfer.h"
#include "devices/linux/DVBDevice.h"
#include "devices/commoninterface/CI.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceCommonInterfaceSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "epg/EPGScanner.h"
#include "scan/Scanner.h"
#include "channels/Channel.h"
#include "settings/Settings.h"
#include "utils/log/Log.h"
#include "utils/Tools.h"

using namespace std;
using namespace PLATFORM;

namespace VDR
{

cDeviceManager::cDeviceManager()
 : m_devicesReady(0),
   m_bAllDevicesReady(false)
{
}

cDeviceManager &cDeviceManager::Get()
{
  static cDeviceManager instance;
  return instance;
}

cDeviceManager::~cDeviceManager()
{
}

size_t cDeviceManager::Initialise(void)
{
  isyslog("Searching for DVB devices in %s", DEV_DVB_BASE);

  DeviceVector devices = cDvbDevice::FindDevices();

  isyslog("%u DVB devices found", devices.size());

  CLockObject lock(m_mutex);

  unsigned int index = 0;
  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    const DevicePtr& device = *it;

    bool bReady = device->Initialise(index++);
    if (bReady)
      ++m_devicesReady;

    isyslog("Device %u: \"%s\" is %s", device->Index(), device->Name().c_str(), bReady ? "ready" : "NOT ready");

    m_devices.push_back(device);
  }

  m_bAllDevicesReady = m_devices.size() == m_devicesReady;
  return m_devices.size();
}

bool cDeviceManager::WaitForAllDevicesReady(unsigned int timeout /* = 0 */)
{
  CLockObject lock(m_mutex);

  // Can't wait if there's no devices
  if (m_devices.empty())
    return false;

  bool retval = m_devicesReadyCondition.Wait(m_mutex, m_bAllDevicesReady, timeout * 1000);
  if (!cScanner::Get().IsRunning())
    cEPGScanner::Get().Start();
  return retval;
}

DevicePtr cDeviceManager::GetDevice(unsigned int index)
{
  CLockObject lock(m_mutex);
  return (index < m_devices.size()) ? m_devices[index] : cDevice::EmptyDevice;
}

void cDeviceManager::Shutdown(void)
{
  CLockObject lock(m_mutex);
  m_devices.clear();
}

TunerHandlePtr cDeviceManager::OpenVideoInput(iReceiver* receiver, device_tuning_type_t type, const ChannelPtr& channel)
{
  TunerHandlePtr handle;

  if (type == TUNING_TYPE_LIVE_TV   ||
      type == TUNING_TYPE_RECORDING)
  {
    DevicePtr device = GetDevice(0); // TODO

    if (device)
    {
      TunerHandlePtr newHandle = device->Acquire(channel, type, receiver);
      if (newHandle && !newHandle->AttachMultiplexedReceiver(receiver, channel))
      {
        /** failed to attach receiver */
        device->Release(newHandle);
      }
      else if (newHandle)
      {
        /** handle acquired and receiver attached */
        handle = newHandle;
      }
    }
  }

  return handle;
}

void cDeviceManager::Notify(const Observable &obs, const ObservableMessage msg)
{
  CLockObject lock(m_mutex);
  switch (msg)
  {
  case ObservableMessageDeviceReady:
    if (m_devicesReady < m_devices.size())
      ++m_devicesReady;
    break;
  case ObservableMessageDeviceNotReady:
    if (m_devicesReady > 0)
      --m_devicesReady;
    break;
  default:
    break;
  }

  m_bAllDevicesReady = m_devicesReady == m_devices.size(); //XXX
  if (m_bAllDevicesReady)
    m_devicesReadyCondition.Broadcast();
}

}
