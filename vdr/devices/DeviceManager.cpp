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
#include "channels/Channel.h"
#include "settings/Settings.h"
#include "utils/log/Log.h"
#include "utils/Tools.h"

#include <assert.h>

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
  isyslog("Searching for DVB devices");

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

  return m_devicesReadyCondition.Wait(m_mutex, m_bAllDevicesReady, timeout * 1000);
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

bool cDeviceManager::OpenVideoInput(const ChannelPtr& channel, cVideoBuffer* videoBuffer)
{
  DevicePtr device = GetDevice(0); // TODO

  if (device && device->Channel()->SwitchChannel(channel))
  {
    m_VideoInput.SetVideoBuffer(videoBuffer);
    if (device->Receiver()->AttachReceiver(&m_VideoInput))
    {
      m_VideoInput.SetChannel(channel);
      m_VideoInput.PmtChange();
      return true;
    }
    m_VideoInput.ResetMembers();
  }

  return false;
}

void cDeviceManager::CloseVideoInput(void)
{
  m_VideoInput.Close();
}

bool cDeviceManager::AttachReceiver(cReceiver* receiver, const ChannelPtr& channel)
{

  return false;
}

void cDeviceManager::DetachReceiver(cReceiver* receiver)
{
  DevicePtr device = GetDevice(0); // TODO
  if (device)
    device->Receiver()->Detach(receiver);
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
