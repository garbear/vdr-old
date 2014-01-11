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

#include "DVBChannelSubsystem.h"
#include "devices/commoninterface/CI.h"
#include "devices/DeviceManager.h"
#include "devices/linux/DVBDevice.h"
#include "devices/linux/DVBTuner.h"
#include "devices/subsystems/DeviceCommonInterfaceSubsystem.h"
#include "devices/subsystems/DevicePlayerSubsystem.h"
#include "devices/subsystems/DevicePIDSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "channels/ChannelManager.h"
#include "dvb/DiSEqC.h"

#include <algorithm>
#include <linux/dvb/frontend.h>

using namespace std;

cDvbChannelSubsystem::cDvbChannelSubsystem(cDevice *device)
 : cDeviceChannelSubsystem(device),
   m_dvbTuner(NULL)
{
}

cDvbChannelSubsystem::~cDvbChannelSubsystem()
{
  delete m_dvbTuner;
}

bool cDvbChannelSubsystem::ProvidesDeliverySystem(fe_delivery_system deliverySystem) const
{
  return GetDevice<cDvbDevice>()->DvbChannel()->GetTuner()->HasDeliverySystem(deliverySystem);
}

bool cDvbChannelSubsystem::ProvidesSource(int source) const
{
  int type = source & cSource::st_Mask;
  if (type == cSource::stNone)
    return true;
  if (type == cSource::stAtsc  && ProvidesDeliverySystem(SYS_ATSC))
    return true;
  if (type == cSource::stCable && (ProvidesDeliverySystem(SYS_DVBC_ANNEX_AC) || ProvidesDeliverySystem(SYS_DVBC_ANNEX_B)))
    return true;
  if (type == cSource::stSat   && (ProvidesDeliverySystem(SYS_DVBS) || ProvidesDeliverySystem(SYS_DVBS2)))
    return true;
  if (type == cSource::stTerr  && (ProvidesDeliverySystem(SYS_DVBT) || ProvidesDeliverySystem(SYS_DVBT2)))
    return true;
  return false;
}

bool cDvbChannelSubsystem::ProvidesTransponder(const cChannel &channel) const
{
  if (!ProvidesSource(channel.Source()))
    return false; // doesn't provide source

  cDvbTransponderParams dtp(channel.Parameters());

  // requires modulation system which frontend doesn't provide - return false in these cases
  if (!ProvidesDeliverySystem(cDvbTuner::GetRequiredDeliverySystem(channel, &dtp))) return false;
  if (dtp.StreamId()   != 0        && !m_dvbTuner->HasCapability(FE_CAN_MULTISTREAM)) return false;
  if (dtp.Modulation() == QPSK     && !m_dvbTuner->HasCapability(FE_CAN_QPSK))        return false;
  if (dtp.Modulation() == QAM_16   && !m_dvbTuner->HasCapability(FE_CAN_QAM_16))      return false;
  if (dtp.Modulation() == QAM_32   && !m_dvbTuner->HasCapability(FE_CAN_QAM_32))      return false;
  if (dtp.Modulation() == QAM_64   && !m_dvbTuner->HasCapability(FE_CAN_QAM_64))      return false;
  if (dtp.Modulation() == QAM_128  && !m_dvbTuner->HasCapability(FE_CAN_QAM_128))     return false;
  if (dtp.Modulation() == QAM_256  && !m_dvbTuner->HasCapability(FE_CAN_QAM_256))     return false;
  if (dtp.Modulation() == QAM_AUTO && !m_dvbTuner->HasCapability(FE_CAN_QAM_AUTO))    return false;
  if (dtp.Modulation() == VSB_8    && !m_dvbTuner->HasCapability(FE_CAN_8VSB))        return false;
  if (dtp.Modulation() == VSB_16   && !m_dvbTuner->HasCapability(FE_CAN_16VSB))       return false;
  // "turbo fec" is a non standard FEC used by North American broadcasters - this is a best guess to determine this condition
  if (dtp.Modulation() == PSK_8    && !m_dvbTuner->HasCapability(FE_CAN_TURBO_FEC) && dtp.System() == SYS_DVBS) return false;

  if (!cSource::IsSat(channel.Source()) ||
      (!Setup.DiSEqC || Diseqcs.Get(Device()->CardIndex() + 1, channel.Source(), channel.Frequency(), dtp.Polarization(), NULL)))
    return cDeviceManager::Get().DeviceHooksProvidesTransponder(*Device(), channel);
  return false;
}

bool cDvbChannelSubsystem::ProvidesChannel(const cChannel &channel, int priority /* = IDLEPRIORITY */, bool *pNeedsDetachReceivers /* = NULL */) const
{
  bool result = false;
  bool hasPriority = priority == IDLEPRIORITY || priority > this->Receiver()->Priority();
  bool needsDetachReceivers = false;
  GetDevice<cDvbDevice>()->m_bNeedsDetachBondedReceivers = false;

  if (m_dvbTuner && ProvidesTransponder(channel))
  {
    result = hasPriority;
    if (priority > IDLEPRIORITY)
    {
      if (Receiver()->Receiving())
      {
        if (m_dvbTuner->IsTunedTo(channel))
        {
          if ((channel.Vpid() && !PID()->HasPid(channel.Vpid())) ||
              (channel.Apid(0) && !PID()->HasPid(channel.Apid(0))) ||
              (channel.Dpid(0) && !PID()->HasPid(channel.Dpid(0))))
          {
            if (CommonInterface()->CamSlot() && channel.Ca() >= CA_ENCRYPTED_MIN)
            {
              if (CommonInterface()->CamSlot()->CanDecrypt(&channel))
                result = true;
              else
                needsDetachReceivers = true;
            }
            else
              result = true;
          }
          else
            result = true;
        }
        else
          needsDetachReceivers = Receiver()->Receiving();
      }

      if (result)
      {
        PLATFORM::CLockObject lock(GetDevice<cDvbDevice>()->m_bondMutex);
        if (!GetDevice<cDvbDevice>()->BondingOk(channel))
        {
          // This device is bonded, so we need to check the priorities of the others:
          for (cDvbDevice *d = GetDevice<cDvbDevice>()->m_bondedDevice; d && d != GetDevice<cDvbDevice>(); d = d->m_bondedDevice)
          {
            if (d->Receiver()->Priority() >= priority)
            {
              result = false;
              break;
            }
            needsDetachReceivers |= d->Receiver()->Receiving();
          }
          GetDevice<cDvbDevice>()->m_bNeedsDetachBondedReceivers = true;
          needsDetachReceivers |= Receiver()->Receiving();
        }
      }
    }
  }
  if (pNeedsDetachReceivers)
    *pNeedsDetachReceivers = needsDetachReceivers;
  return result;
}

bool cDvbChannelSubsystem::ProvidesEIT() const
{
  return m_dvbTuner != NULL;
}

unsigned int cDvbChannelSubsystem::NumProvidedSystems() const
{
  return m_dvbTuner->m_deliverySystems.size() + m_dvbTuner->m_numModulations;
}

int cDvbChannelSubsystem::SignalStrength() const
{
  if (m_dvbTuner)
    return m_dvbTuner->GetSignalStrength();
  return -1;
}

int cDvbChannelSubsystem::SignalQuality() const
{
  if (m_dvbTuner)
    return m_dvbTuner->GetSignalQuality();
  return -1;
}

const cChannel *cDvbChannelSubsystem::GetCurrentlyTunedTransponder() const
{
  if (m_dvbTuner)
    return &m_dvbTuner->GetTransponder();
  return NULL;
}

bool cDvbChannelSubsystem::IsTunedToTransponder(const cChannel &channel) const
{
  if (m_dvbTuner)
    return m_dvbTuner->IsTunedTo(channel);
  return false;
}

bool cDvbChannelSubsystem::MaySwitchTransponder(const cChannel &channel) const
{
  return GetDevice<cDvbDevice>()->BondingOk(channel, true) && Channel()->MaySwitchTransponder(channel);
}

bool cDvbChannelSubsystem::HasLock(unsigned int timeoutMs) const
{
  if (m_dvbTuner)
    return m_dvbTuner->Locked(timeoutMs);
  return false;
}

bool cDvbChannelSubsystem::SetChannelDevice(const cChannel &channel, bool bLiveView)
{
  if (m_dvbTuner)
    m_dvbTuner->SetChannel(channel);
  return true;
}
