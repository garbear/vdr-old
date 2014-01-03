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
#include "devices/commoninterface/CI.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceCommonInterfaceSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "channels/ChannelManager.h"
#include "utils/Tools.h"
#include "Transfer.h"

#include <assert.h>

using namespace std;

cDeviceManager::cDeviceManager()
 : m_primaryDevice(NULL),
   m_currentChannel(1),
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
  for (vector<cDevice*>::iterator deviceIt = m_devices.begin(); deviceIt != m_devices.end(); ++deviceIt)
  {
    if (m_primaryDevice == *deviceIt)
      m_primaryDevice = NULL;
    delete *deviceIt;
  }
  for (vector<cDeviceHook*>::iterator deviceHookIt = m_deviceHooks.begin(); deviceHookIt != m_deviceHooks.end(); ++deviceHookIt)
    delete *deviceHookIt;
}

unsigned int cDeviceManager::AddDevice(cDevice *device)
{
  assert(device);
  m_devices.push_back(device);
  if (!m_primaryDevice)
    m_primaryDevice = device;
  return m_devices.size(); // Remember, our devices' numbers are 1-indexed
}

void cDeviceManager::SetPrimaryDevice(unsigned int index)
{
  assert(0 != index && index <= m_devices.size());
  index--; // TODO: Why is this 1-based???

  if (m_primaryDevice)
    m_primaryDevice->MakePrimaryDevice(false);
  m_primaryDevice = m_devices[index];
  m_primaryDevice->MakePrimaryDevice(true);
  m_primaryDevice->VideoFormat()->SetVideoFormat(Setup.VideoFormat);
}

void cDeviceManager::AddHook(cDeviceHook *hook)
{
  assert(hook);
  m_deviceHooks.push_back(hook);
}

bool cDeviceManager::DeviceHooksProvidesTransponder(const cDevice &device, const cChannel &channel) const
{
  for (vector<cDeviceHook*>::const_iterator deviceHookIt = m_deviceHooks.begin(); deviceHookIt != m_deviceHooks.end(); ++deviceHookIt)
  {
    if (!(*deviceHookIt)->DeviceProvidesTransponder(device, channel))
      return false;
  }
  return true;
}

bool cDeviceManager::WaitForAllDevicesReady(unsigned int timeout /* = 0 */)
{
  // TODO: Wait on a condition variable
  time_t start = time(NULL);
  while (time(NULL) - start < timeout)
  {
    bool ready = true;
    for (vector<cDevice*>::iterator deviceIt = m_devices.begin(); deviceIt != m_devices.end(); ++deviceIt)
    {
      if ((*deviceIt)->Ready())
      {
        ready = false;
        cCondWait::SleepMs(100);
      }
    }
    if (ready)
      return true;
  }
  return false;
}

cDevice *cDeviceManager::ActualDevice()
{
  cDevice *d = cTransferControl::ReceiverDevice();
  if (!d)
    d = PrimaryDevice();
  return d;
}

cDevice *cDeviceManager::GetDevice(unsigned int index)
{
  return (index < m_devices.size()) ? m_devices[index] : NULL;
}

cDevice *cDeviceManager::GetDevice(const cChannel &channel, int priority, bool bLiveView, bool bQuery /* = false */)
{
  // Collect the current priorities of all CAM slots that can decrypt the channel:
  int NumCamSlots = CamSlots.Count();
  int SlotPriority[NumCamSlots];
  int NumUsableSlots = 0;
  bool InternalCamNeeded = false;
  if (channel.Ca() >= CA_ENCRYPTED_MIN)
  {
    for (cCamSlot *CamSlot = CamSlots.First(); CamSlot; CamSlot = CamSlots.Next(CamSlot))
    {
      SlotPriority[CamSlot->Index()] = MAXPRIORITY + 1; // assumes it can't be used
      if (CamSlot->ModuleStatus() == msReady)
      {
        if (CamSlot->ProvidesCa(channel.Caids()))
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
  cDevice *d = NULL;
  cCamSlot *s = NULL;

  uint32_t Impact = 0xFFFFFFFF; // we're looking for a device with the least impact
  for (int j = 0; j < NumCamSlots || !NumUsableSlots; j++)
  {
    if (NumUsableSlots && SlotPriority[j] > MAXPRIORITY)
      continue; // there is no CAM available in this slot
    for (int i = 0; i < m_devices.size(); i++)
    {
      if (channel.Ca() && channel.Ca() <= CA_DVB_MAX && channel.Ca() != m_devices[i]->CardIndex() + 1)
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
        imp <<= 1; imp |= bLiveView ? !m_devices[i]->IsPrimaryDevice() || ndr : 0;                                  // prefer the primary device for live viewing if we don't need to detach existing receivers
        imp <<= 1; imp |= (!m_devices[i]->Receiver()->Receiving() && (m_devices[i] != cTransferControl::ReceiverDevice() || m_devices[i]->IsPrimaryDevice())) || ndr; // use receiving devices if we don't need to detach existing receivers, but avoid primary device in local transfer mode
        imp <<= 1; imp |= m_devices[i]->Receiver()->Receiving();                                                               // avoid devices that are receiving
        imp <<= 4; imp |= GetClippedNumProvidedSystems(4, m_devices[i]) - 1;                                       // avoid cards which support multiple delivery systems
        imp <<= 1; imp |= m_devices[i] == cTransferControl::ReceiverDevice();                                      // avoid the Transfer Mode receiver device
        imp <<= 8; imp |= m_devices[i]->Receiver()->Priority() - IDLEPRIORITY;                                                 // use the device with the lowest priority (- IDLEPRIORITY to assure that values -100..99 can be used)
        imp <<= 8; imp |= ((NumUsableSlots && !HasInternalCam) ? SlotPriority[j] : IDLEPRIORITY) - IDLEPRIORITY;// use the CAM slot with the lowest priority (- IDLEPRIORITY to assure that values -100..99 can be used)
        imp <<= 1; imp |= ndr;                                                                                  // avoid devices if we need to detach existing receivers
        imp <<= 1; imp |= (NumUsableSlots || InternalCamNeeded) ? 0 : m_devices[i]->CommonInterface()->HasCi();                       // avoid cards with Common Interface for FTA channels
        imp <<= 1; imp |= m_devices[i]->AvoidRecording();                                                          // avoid SD full featured cards
        imp <<= 1; imp |= (NumUsableSlots && !HasInternalCam) ? !ChannelCamRelations.CamDecrypt(channel.GetChannelID(), j + 1) : 0; // prefer CAMs that are known to decrypt this channel
        imp <<= 1; imp |= m_devices[i]->IsPrimaryDevice();                                                         // avoid the primary device
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
          d->CommonInterface()->CamSlot()->Assign(NULL);
        s->Assign(d);
      }
    }
    else if (d->CommonInterface()->CamSlot() && !d->CommonInterface()->CamSlot()->IsDecrypting())
      d->CommonInterface()->CamSlot()->Assign(NULL);
  }
  return d;
}

cDevice *cDeviceManager::GetDeviceForTransponder(const cChannel &channel, int priority)
{
  cDevice *Device = NULL;
  for (int i = 0; i < NumDevices(); i++)
  {
    if (cDevice *d = GetDevice(i))
    {
      if (d->Channel()->IsTunedToTransponder(channel))
        return d; // if any device is tuned to the transponder, we're done

      if (d->Channel()->ProvidesTransponder(channel))
      {
        if (d->Channel()->MaySwitchTransponder(channel))
          Device = d; // this device may switch to the transponder without disturbing any receiver or live view
        else if (!d->Channel()->Occupied() && d->Channel()->MaySwitchTransponder(channel)) // MaySwitchTransponder() implicitly calls Occupied()
        {
          if (d->Receiver()->Priority() < priority && (!Device || d->Receiver()->Priority() < Device->Receiver()->Priority()))
            Device = d; // use this one only if no other with less impact can be found
        }
      }
    }
  }
  return Device;
}

unsigned int cDeviceManager::CountTransponders(const cChannel &channel) const
{
  unsigned int count = 0;
  for (vector<cDevice*>::const_iterator it = m_devices.begin(); it != m_devices.end(); ++it)
  {
    if ((*it)->Channel()->ProvidesTransponder(channel))
      count++;
  }
  return count;
}

bool cDeviceManager::SwitchChannel(bool bIncrease)
{
  bool result = false;
  int offset = bIncrease ? 1 : -1;

  // prevents old channel from being shown too long if GetDevice() takes longer
  cControl::Shutdown();

  int n = CurrentChannel() + offset;
  int first = n;
  ChannelPtr channel = cChannelManager::Get().GetByNumber(n, offset);
  while (channel)
  {
    // try only channels which are currently available
    if (GetDevice(*channel, LIVEPRIORITY, true, true))
      break;
    n = channel->Number() + offset;
    channel = cChannelManager::Get().GetByNumber(n, offset);
  }

  if (channel)
  {
    int d = n - first;
    if (abs(d) == 1)
      dsyslog("skipped channel %d", first);
    else if (d)
      dsyslog("skipped channels %d..%d", first, n - sgn(d));
    if (m_primaryDevice->Channel()->SwitchChannel(*channel, true))
      result = true;
  }
//  else if (n != first)
//    Skins.Message(mtError, tr("Channel not available!"));

  return result;
}

void cDeviceManager::SetCurrentChannel(const cChannel &channel)
{
  m_currentChannel = channel.Number();
}

void cDeviceManager::Shutdown(void)
{
  m_deviceHooks.clear();
  for (int i = 0; i < NumDevices(); i++)
  {
    delete m_devices[i];
    m_devices[i] = NULL;
  }
}

int cDeviceManager::GetClippedNumProvidedSystems(int availableBits, cDevice *device)
{
  int MaxNumProvidedSystems = (1 << availableBits) - 1;
  int NumProvidedSystems = device->Channel()->NumProvidedSystems();
  if (NumProvidedSystems > MaxNumProvidedSystems)
  {
    esyslog("ERROR: device %d supports %d modulation systems but cDevice::GetDevice() currently only supports %d delivery systems which should be fixed", device->CardIndex() + 1, NumProvidedSystems, MaxNumProvidedSystems);
    NumProvidedSystems = MaxNumProvidedSystems;
  }
  else if (NumProvidedSystems <= 0)
  {
    esyslog("ERROR: device %d reported an invalid number (%d) of supported delivery systems - assuming 1", device->CardIndex() + 1, NumProvidedSystems);
    NumProvidedSystems = 1;
  }
  return NumProvidedSystems;
}
