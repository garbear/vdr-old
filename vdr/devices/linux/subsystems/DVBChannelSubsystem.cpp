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

#include "DVBChannelSubsystem.h"
#include "devices/commoninterface/CI.h"
#include "devices/DeviceManager.h"
#include "devices/linux/DVBDevice.h"
#include "devices/linux/DVBTuner.h"
#include "devices/subsystems/DeviceCommonInterfaceSubsystem.h"
#include "devices/subsystems/DevicePlayerSubsystem.h"
#include "devices/subsystems/DevicePIDSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "channels/ChannelTypes.h"
#include "dvb/DiSEqC.h"
#include "settings/Settings.h"

#include <algorithm>
#include <linux/dvb/frontend.h>

using namespace std;

namespace VDR
{

cDvbChannelSubsystem::cDvbChannelSubsystem(cDevice *device)
 : cDeviceChannelSubsystem(device)
{
}

bool cDvbChannelSubsystem::ProvidesDeliverySystem(fe_delivery_system_t deliverySystem) const
{
  return Device<cDvbDevice>()->m_dvbTuner.HasDeliverySystem(deliverySystem);
}

bool cDvbChannelSubsystem::ProvidesSource(TRANSPONDER_TYPE source) const
{
  if (source == TRANSPONDER_INVALID)
    return true;

  if (source == TRANSPONDER_ATSC && ProvidesDeliverySystem(SYS_ATSC))
    return true;

  if (source == TRANSPONDER_CABLE && (ProvidesDeliverySystem(SYS_DVBC_ANNEX_AC) || ProvidesDeliverySystem(SYS_DVBC_ANNEX_B)))
    return true;

  if (source == TRANSPONDER_SATELLITE && (ProvidesDeliverySystem(SYS_DVBS) || ProvidesDeliverySystem(SYS_DVBS2)))
    return true;

  if (source == TRANSPONDER_TERRESTRIAL && (ProvidesDeliverySystem(SYS_DVBT) || ProvidesDeliverySystem(SYS_DVBT2)))
    return true;

  return false;
}

bool cDvbChannelSubsystem::ProvidesTransponder(const cChannel &channel) const
{
  const cTransponder& transponder = channel.GetTransponder();

  if (!ProvidesSource(transponder.Type()))
    return false; // doesn't provide source

  // requires modulation system which frontend doesn't provide - return false in these cases
  if (!ProvidesDeliverySystem(transponder.DeliverySystem())) return false;
  if (transponder.IsSatellite()   && transponder.SatelliteParams().StreamId()   != 0 && !Device<cDvbDevice>()->m_dvbTuner.HasCapability((fe_caps)FE_CAN_MULTISTREAM)) return false;
  if (transponder.IsTerrestrial() && transponder.TerrestrialParams().StreamId() != 0 && !Device<cDvbDevice>()->m_dvbTuner.HasCapability((fe_caps)FE_CAN_MULTISTREAM)) return false;
  if (transponder.Modulation() == QPSK     && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_QPSK))        return false;
  if (transponder.Modulation() == QAM_16   && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_QAM_16))      return false;
  if (transponder.Modulation() == QAM_32   && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_QAM_32))      return false;
  if (transponder.Modulation() == QAM_64   && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_QAM_64))      return false;
  if (transponder.Modulation() == QAM_128  && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_QAM_128))     return false;
  if (transponder.Modulation() == QAM_256  && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_QAM_256))     return false;
  if (transponder.Modulation() == QAM_AUTO && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_QAM_AUTO))    return false;
  if (transponder.Modulation() == VSB_8    && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_8VSB))        return false;
  if (transponder.Modulation() == VSB_16   && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_16VSB))       return false;
  // "turbo fec" is a non standard FEC used by North American broadcasters - this is a best guess to determine this condition
  if (transponder.Modulation() == PSK_8    && !Device<cDvbDevice>()->m_dvbTuner.HasCapability(FE_CAN_TURBO_FEC) && transponder.DeliverySystem() == SYS_DVBS) return false;

  if (transponder.IsSatellite()  &&
      cSettings::Get().m_bDiSEqC &&
      NULL != Diseqcs.Get(Device()->CardIndex() + 1, transponder.Type(), transponder.FrequencyKHz(), transponder.SatelliteParams().Polarization(), NULL))
  {
    return false;
  }
  return true; // TODO: Previous this checked to see if any devices provided a transponder for the channel
}

bool cDvbChannelSubsystem::ProvidesChannel(const cChannel &channel, bool *pNeedsDetachReceivers /* = NULL */) const
{
  bool result = false;
  bool needsDetachReceivers = false;

  if (Device<cDvbDevice>()->m_dvbTuner.IsOpen() && ProvidesTransponder(channel))
  {
    result = true; // XXX

    if (Receiver()->Receiving())
    {
      if (Device<cDvbDevice>()->m_dvbTuner.IsTunedTo(channel))
      {
        if ((channel.GetVideoStream().vpid && !PID()->HasPid(channel.GetVideoStream().vpid)) ||
            (channel.GetAudioStream(0).apid && !PID()->HasPid(channel.GetAudioStream(0).apid)) ||
            (channel.GetDataStream(0).dpid && !PID()->HasPid(channel.GetDataStream(0).dpid)))
        {
          if (CommonInterface()->CamSlot() && channel.GetCaId(0) >= CA_ENCRYPTED_MIN)
          {
            if (CommonInterface()->CamSlot()->CanDecrypt(channel))
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
  }
  if (pNeedsDetachReceivers)
    *pNeedsDetachReceivers = needsDetachReceivers;
  return result;
}

bool cDvbChannelSubsystem::ProvidesEIT() const
{
  return Device<cDvbDevice>()->m_dvbTuner.IsOpen();
}

unsigned int cDvbChannelSubsystem::NumProvidedSystems() const
{
  return Device<cDvbDevice>()->m_dvbTuner.DeliverySystemCount() + Device<cDvbDevice>()->m_dvbTuner.ModulationCount();
}

int cDvbChannelSubsystem::SignalStrength() const
{
  if (Device<cDvbDevice>()->m_dvbTuner.IsOpen())
    return Device<cDvbDevice>()->m_dvbTuner.GetSignalStrength();
  return -1;
}

int cDvbChannelSubsystem::SignalQuality() const
{
  if (Device<cDvbDevice>()->m_dvbTuner.IsOpen())
    return Device<cDvbDevice>()->m_dvbTuner.GetSignalQuality();
  return -1;
}

ChannelPtr cDvbChannelSubsystem::GetCurrentlyTunedTransponder() const
{
  return Device<cDvbDevice>()->m_dvbTuner.GetTransponder();
}

bool cDvbChannelSubsystem::IsTunedToTransponder(const cChannel &channel) const
{
  return Device<cDvbDevice>()->m_dvbTuner.IsTunedTo(channel);
}

bool cDvbChannelSubsystem::HasLock(void) const
{
  return Device<cDvbDevice>()->m_dvbTuner.HasLock();
}

bool cDvbChannelSubsystem::SetChannelDevice(const ChannelPtr& channel)
{
  return Device<cDvbDevice>()->m_dvbTuner.SwitchChannel(channel);
}

void cDvbChannelSubsystem::ClearChannelDevice(void)
{
  return Device<cDvbDevice>()->m_dvbTuner.ClearChannel();
}

}
