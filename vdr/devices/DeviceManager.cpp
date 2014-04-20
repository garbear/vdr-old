/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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
#include "devices/linux/DVBDevice.h"
#include "devices/commoninterface/CI.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceCommonInterfaceSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "channels/ChannelManager.h"
#include "settings/Settings.h"
#include "utils/Tools.h"
#include "Transfer.h"

#include <assert.h>

using namespace std;
using namespace PLATFORM;

namespace VDR
{

cDeviceManager::cDeviceManager()
 : m_devicesReady(0),
   m_bAllDevicesReady(false),
   m_nextCardIndex(0),
   m_useDevice(0)
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
  dsyslog("initialising DVB devices");

  DeviceVector devices = cDvbDevice::FindDevices();
  cDvbDevice::BondDevices(cSettings::Get().m_strDeviceBondings);
  dsyslog("%u DVB devices found", devices.size());

  for (DeviceVector::iterator it = devices.begin(); it != devices.end(); ++it)
  {
    cDvbDevice* device = dynamic_cast<cDvbDevice*>(it->get());
    if (!device)
      continue;

    AddDevice(*it);
    if ((*it)->Initialise())
    {
      CLockObject lock(m_mutex);
      ++m_devicesReady;
    }
  }

  CLockObject lock(m_mutex);
  m_bAllDevicesReady = m_devices.size() == m_devicesReady;
  return m_devices.size();
}

size_t cDeviceManager::AddDevice(DevicePtr device)
{
  device->AssertValid();

  CLockObject lock(m_mutex);
  device->SetCardIndex(m_devices.size());
  m_devices.push_back(device);
  dsyslog("registered device #%u: %s", m_devices.size(), device->DeviceName().c_str());
  return m_devices.size(); // Remember, our devices' numbers are 1-indexed
}

bool cDeviceManager::WaitForAllDevicesReady(unsigned int timeout /* = 0 */)
{
  CLockObject lock(m_mutex);
  return m_devicesReadyCondition.Wait(m_mutex, m_bAllDevicesReady, timeout * 1000);
}

DevicePtr cDeviceManager::GetDevice(unsigned int index)
{
  CLockObject lock(m_mutex);
  return (index < m_devices.size()) ? m_devices[index] : cDevice::EmptyDevice;
}

DevicePtr cDeviceManager::GetDevice(const cChannel &channel, int priority, bool bLiveView, bool bQuery /* = false */)
{
  // Collect the current priorities of all CAM slots that can decrypt the channel:
  int NumCamSlots = CamSlots.Count();
  int SlotPriority[NumCamSlots];
  int NumUsableSlots = 0;
  bool InternalCamNeeded = false;
  CLockObject lock(m_mutex);
  if (channel.GetCaId(0) >= CA_ENCRYPTED_MIN)
  {
    for (cCamSlot *CamSlot = CamSlots.First(); CamSlot; CamSlot = CamSlots.Next(CamSlot))
    {
      SlotPriority[CamSlot->Index()] = MAXPRIORITY + 1; // assumes it can't be used
      if (CamSlot->ModuleStatus() == msReady)
      {
        if (CamSlot->ProvidesCa(channel.GetCaIds()))
        {
          if (!ChannelCamRelations.CamChecked(channel.GetChannelID(), CamSlot->SlotNumber()))
          {
            SlotPriority[CamSlot->Index()] = CamSlot->Priority();
            NumUsableSlots++;
          }
        }
      }
    }
    if (!NumUsableSlots)
      InternalCamNeeded = true; // no CAM is able to decrypt this channel
  }

  bool NeedsDetachReceivers = false;
  DevicePtr d = cDevice::EmptyDevice;
  cCamSlot *s = NULL;

  uint32_t Impact = 0xFFFFFFFF; // we're looking for a device with the least impact
  for (int j = 0; j < NumCamSlots || !NumUsableSlots; j++)
  {
    if (NumUsableSlots && SlotPriority[j] > MAXPRIORITY)
    {
      continue; // there is no CAM available in this slot
    }
    dsyslog("checking %u devices", m_devices.size());
    for (int i = 0; i < m_devices.size(); i++)
    {
      m_devices[i]->AssertValid();
      if (channel.GetCaId(0) && channel.GetCaId(0) <= CA_DVB_MAX && channel.GetCaId(0) != m_devices[i]->CardIndex() + 1)
        continue; // a specific card was requested, but not this one
      bool HasInternalCam = m_devices[i]->CommonInterface()->HasInternalCam();
      if (InternalCamNeeded && !HasInternalCam)
        continue; // no CAM is able to decrypt this channel and the device uses vdr handled CAMs
      if (NumUsableSlots && !HasInternalCam && !CamSlots.Get(j)->Assign(m_devices[i], true))
        continue; // CAM slot can't be used with this device
      bool ndr;
      if (m_devices[i]->Channel()->ProvidesChannel(channel, priority, &ndr)) // this device is basically able to do the job
      {
        if (NumUsableSlots && !HasInternalCam && m_devices[i]->CommonInterface()->CamSlot() && m_devices[i]->CommonInterface()->CamSlot() != CamSlots.Get(j))
          ndr = true; // using a different CAM slot requires detaching receivers
        // Put together an integer number that reflects the "impact" using
        // this device would have on the overall system. Each condition is represented
        // by one bit in the number (or several bits, if the condition is actually
        // a numeric value). The sequence in which the conditions are listed corresponds
        // to their individual severity, where the one listed first will make the most
        // difference, because it results in the most significant bit of the result.
        uint32_t imp = 0;
        imp <<= 1; imp |= bLiveView ? ndr : 0;                                  // prefer the primary device for live viewing if we don't need to detach existing receivers
        imp <<= 1; imp |= m_devices[i]->Receiver()->Receiving();                                                               // avoid devices that are receiving
        imp <<= 4; imp |= GetClippedNumProvidedSystems(4, *m_devices[i]) - 1;                                       // avoid cards which support multiple delivery systems
        imp <<= 8; imp |= m_devices[i]->Receiver()->Priority() - IDLEPRIORITY;                                                 // use the device with the lowest priority (- IDLEPRIORITY to assure that values -100..99 can be used)
        imp <<= 8; imp |= ((NumUsableSlots && !HasInternalCam) ? SlotPriority[j] : IDLEPRIORITY) - IDLEPRIORITY;// use the CAM slot with the lowest priority (- IDLEPRIORITY to assure that values -100..99 can be used)
        imp <<= 1; imp |= ndr;                                                                                  // avoid devices if we need to detach existing receivers
        imp <<= 1; imp |= (NumUsableSlots || InternalCamNeeded) ? 0 : m_devices[i]->CommonInterface()->HasCi();                       // avoid cards with Common Interface for FTA channels
        imp <<= 1; imp |= m_devices[i]->AvoidRecording();                                                          // avoid SD full featured cards
        imp <<= 1; imp |= (NumUsableSlots && !HasInternalCam) ? !ChannelCamRelations.CamDecrypt(channel.GetChannelID(), j + 1) : 0; // prefer CAMs that are known to decrypt this channel
        if (imp < Impact)
        {
          // This device has less impact than any previous one, so we take it.
          Impact = imp;
          d = m_devices[i];
          NeedsDetachReceivers = ndr;
          if (NumUsableSlots && !HasInternalCam)
            s = CamSlots.Get(j);
        }
      }
    }
    if (!NumUsableSlots)
      break; // no CAM necessary, so just one loop over the devices
  }
  if (d && !bQuery)
  {
    if (NeedsDetachReceivers)
      d->Receiver()->DetachAllReceivers();
    if (s)
    {
      if (s->Device() != d)
      {
        if (s->Device())
          s->Device()->Receiver()->DetachAllReceivers();
        if (d->CommonInterface()->CamSlot())
          d->CommonInterface()->CamSlot()->Assign(cDevice::EmptyDevice);
        s->Assign(d);
      }
    }
    else if (d->CommonInterface()->CamSlot() && !d->CommonInterface()->CamSlot()->IsDecrypting())
      d->CommonInterface()->CamSlot()->Assign(cDevice::EmptyDevice);
  }
  return d;
}

DevicePtr cDeviceManager::GetDeviceForTransponder(const cChannel &channel, int priority)
{
  DevicePtr Device = cDevice::EmptyDevice;
  for (std::vector<DevicePtr>::const_iterator it = m_devices.begin(); it != m_devices.end(); ++it)
  {
    if ((*it)->Channel()->IsTunedToTransponder(channel))
      return (*it); // if any device is tuned to the transponder, we're done

    if ((*it)->Channel()->ProvidesTransponder(channel))
    {
      if ((*it)->Channel()->MaySwitchTransponder(channel))
        Device = (*it); // this device may switch to the transponder without disturbing any receiver or live view
      else if (!(*it)->Channel()->Occupied() && (*it)->Channel()->MaySwitchTransponder(channel)) // MaySwitchTransponder() implicitly calls Occupied()
      {
        if ((*it)->Receiver()->Priority() < priority && (!Device || (*it)->Receiver()->Priority() < Device->Receiver()->Priority()))
          Device = (*it); // use this one only if no other with less impact can be found
      }
    }
  }

  return Device;
}

size_t cDeviceManager::CountTransponders(const cChannel &channel) const
{
  size_t count = 0;
  CLockObject lock(m_mutex);
  for (vector<DevicePtr>::const_iterator it = m_devices.begin(); it != m_devices.end(); ++it)
  {
    if ((*it)->Channel()->ProvidesTransponder(channel))
      count++;
  }
  return count;
}

void cDeviceManager::Shutdown(void)
{
  CLockObject lock(m_mutex);
  m_devices.clear();
}

int cDeviceManager::GetClippedNumProvidedSystems(int availableBits, const cDevice& device)
{
  int MaxNumProvidedSystems = (1 << availableBits) - 1;
  int NumProvidedSystems = device.Channel()->NumProvidedSystems();
  if (NumProvidedSystems > MaxNumProvidedSystems)
  {
    esyslog("ERROR: device %d supports %d modulation systems but cDevice::GetDevice() currently only supports %d delivery systems which should be fixed", device.CardIndex() + 1, NumProvidedSystems, MaxNumProvidedSystems);
    NumProvidedSystems = MaxNumProvidedSystems;
  }
  else if (NumProvidedSystems <= 0)
  {
    esyslog("ERROR: device %d reported an invalid number (%d) of supported delivery systems - assuming 1", device.CardIndex() + 1, NumProvidedSystems);
    NumProvidedSystems = 1;
  }
  return NumProvidedSystems;
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

size_t cDeviceManager::NumDevices()
{
  CLockObject lock(m_mutex);
  return m_devices.size();
}

}
