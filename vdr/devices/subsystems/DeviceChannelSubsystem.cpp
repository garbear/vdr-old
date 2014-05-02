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

#include "DeviceChannelSubsystem.h"
#include "DeviceCommonInterfaceSubsystem.h"
#include "DevicePIDSubsystem.h"
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
#include "utils/Status.h"
#include "utils/Tools.h"
#include "utils/I18N.h"

#include <time.h>

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

bool cDeviceChannelSubsystem::ProvidesTransponderExclusively(const cChannel &channel) const
{
  return ProvidesTransponder(channel) && cDeviceManager::Get().CountTransponders(channel) == 1;
}

bool cDeviceChannelSubsystem::MaySwitchTransponder(const cChannel &channel) const
{
  if (time(NULL) > m_occupiedTimeout)
  {
    return !Receiver()->Receiving() && !(PID()->m_pidHandles[ptAudio].pid ||
                                         PID()->m_pidHandles[ptVideo].pid ||
                                         PID()->m_pidHandles[ptDolby].pid);
  }
  return false;
}

bool cDeviceChannelSubsystem::SwitchChannel(ChannelPtr channel)
{
  for (int i = 3; i--; )
  {
    switch (SetChannel(channel))
    {
    case scrOk:
      return true;
    case scrNotAvailable:
      esyslog(tr("Channel not available!"));
      return false;
    case scrNoTransfer:
      esyslog(tr("Can't start Transfer Mode!"));
      return false;
    case scrFailed:
      esyslog("Tuning failed");
      break; // loop will retry
    default:
      esyslog("ERROR: invalid return value from SetChannel");
      break;
    }
    esyslog("retrying");
  }
  return false;
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

bool cDeviceChannelSubsystem::HasProgramme() const
{
  return Player()->Replaying() || PID()->m_pidHandles[ptAudio].pid || PID()->m_pidHandles[ptVideo].pid;
}

eSetChannelResult cDeviceChannelSubsystem::SetChannel(ChannelPtr channel)
{
  cStatus::MsgChannelSwitch(Device(), 0);

  cDevice *device = Device();

  bool NeedsTransferMode = (device != Device());

  eSetChannelResult Result = scrOk;

  // If this DVB card can't receive this channel, let's see if we can
  // use the card that actually can receive it and transfer data from there:

  if (NeedsTransferMode)
  {
    if (device && Player()->CanReplay())
    {
      if (device->Channel()->SetChannel(channel) == scrOk) // calling SetChannel() directly, not SwitchChannel()!
        cControl::Launch(new cTransferControl(device, channel));
      else
        Result = scrNoTransfer;
    }
    else
      Result = scrNotAvailable;
  }
  else
  {
    //cChannelManager::Get().Lock(false); // TODO

    // Stop section handling
    SectionFilter()->StopSectionHandler();

    // Tell the camSlot about the channel switch and add all PIDs of this
    // channel to it, for possible later decryption
    if (CommonInterface()->m_camSlot)
      CommonInterface()->m_camSlot->AddChannel(*channel);

    if (SetChannelDevice(*channel))
    {
      // Start section handling
      SectionFilter()->StartSectionHandler();

      // Start decrypting any PIDs that might have been set in SetChannelDevice():
      if (CommonInterface()->m_camSlot)
        CommonInterface()->m_camSlot->StartDecrypting();
    }
    else
      Result = scrFailed;
    //cChannelManager::Get().Unlock(); // TODO
  }

  if (Result == scrOk)
    cStatus::MsgChannelSwitch(Device(), channel->Number()); // only report status if channel switch successful

  return Result;
}

}
