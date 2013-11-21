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
#include "DevicePIDSubsystem.h"
#include "DevicePlayerSubsystem.h"
#include "DeviceReceiverSubsystem.h"
#include "DeviceSectionFilterSubsystem.h"
#include "DeviceSPUSubsystem.h"
#include "../../devices/Device.h"
#include "../../devices/DeviceManager.h"
#include "../../../channels.h"
#include "../../../player.h"
#include "../../../skins.h"
#include "../../../tools.h"
#include "../../../transfer.h"

#include <time.h>

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

bool cDeviceChannelSubsystem::SwitchChannel(const cChannel &channel, bool bLiveView)
{
  if (bLiveView)
  {
    isyslog("switching to channel %d", channel.Number());
    cControl::Shutdown(); // prevents old channel from being shown too long if cDeviceManager::GetDevice() takes longer
  }
  for (int i = 3; i--; )
  {
    switch (SetChannel(channel, bLiveView))
    {
    case scrOk:
      return true;
    case scrNotAvailable:
      Skins.Message(mtInfo, tr("Channel not available!"));
      return false;
    case scrNoTransfer:
      Skins.Message(mtError, tr("Can't start Transfer Mode!"));
      return false;
    case scrFailed:
      break; // loop will retry
    default:
      esyslog("ERROR: invalid return value from SetChannel");
      break;
    }
    esyslog("retrying");
  }
  return false;
}

void cDeviceChannelSubsystem::ForceTransferMode()
{
  if (!cTransferControl::ReceiverDevice())
  {
    cChannel *channel = Channels.GetByNumber(cDeviceManager::Get().CurrentChannel());
    if (channel)
      SetChannelDevice(*channel, false); // this implicitly starts Transfer Mode
  }
}

unsigned int cDeviceChannelSubsystem::Occupied() const
{
  int seconds = m_occupiedTimeout - time(NULL);
  return seconds > 0 ? seconds : 0;
}

void cDeviceChannelSubsystem::SetOccupied(unsigned int seconds)
{
  m_occupiedTimeout = time(NULL) + ::min(seconds, MAXOCCUPIEDTIMEOUT);
}

bool cDeviceChannelSubsystem::HasProgramme() const
{
  return Player()->Replaying() || PID()->m_pidHandles[ptAudio].pid || PID()->m_pidHandles[ptVideo].pid;
}

eSetChannelResult cDeviceChannelSubsystem::SetChannel(const cChannel &channel, bool bLiveView)
{
  cStatus::MsgChannelSwitch(Device(), 0, bLiveView);

  if (bLiveView)
  {
    Player()->StopReplay();
    SAFE_DELETE(SPU()->m_liveSubtitle);
    SAFE_DELETE(SPU()->m_dvbSubtitleConverter);
  }

  cDevice *device = (bLiveView && Device()->IsPrimaryDevice()) ? cDeviceManager::Get().GetDevice(channel, LIVEPRIORITY, true) : Device();

  bool NeedsTransferMode = device != this;

  eSetChannelResult Result = scrOk;

  // If this DVB card can't receive this channel, let's see if we can
  // use the card that actually can receive it and transfer data from there:

  if (NeedsTransferMode)
  {
    if (device && Player()->CanReplay())
    {
      if (device->Channel()->SetChannel(channel, false) == scrOk) // calling SetChannel() directly, not SwitchChannel()!
        cControl::Launch(new cTransferControl(device, &channel));
      else
        Result = scrNoTransfer;
    }
    else
      Result = scrNotAvailable;
  }
  else
  {
    Channels.Lock(false);

    // Stop section handling:
    if (SectionFilter()->m_sectionHandler)
    {
      SectionFilter()->m_sectionHandler->SetStatus(false);
      SectionFilter()->m_sectionHandler->SetChannel(NULL);
    }

    // Tell the camSlot about the channel switch and add all PIDs of this
    // channel to it, for possible later decryption:
    if (CommonInterface()->m_camSlot)
      CommonInterface()->m_camSlot->AddChannel(&channel);

    if (SetChannelDevice(channel, bLiveView))
    {
      // Start section handling:
      if (SectionFilter()->m_sectionHandler)
      {
        SectionFilter()->m_sectionHandler->SetChannel(&channel);
        SectionFilter()->m_sectionHandler->SetStatus(true);
      }

      // Start decrypting any PIDs that might have been set in SetChannelDevice():
      if (CommonInterface()->m_camSlot)
        CommonInterface()->m_camSlot->StartDecrypting();
    }
    else
      Result = scrFailed;
    Channels.Unlock();
  }

  if (Result == scrOk)
  {
    if (bLiveView && Device()->IsPrimaryDevice())
    {
      cDeviceManager::Get().SetCurrentChannel(channel);
      // Set the available audio tracks:
      Track()->ClrAvailableTracks();
      for (int i = 0; i < MAXAPIDS; i++)
        Track()->SetAvailableTrack(ttAudio, i, channel.Apid(i), channel.Alang(i));
      if (Setup.UseDolbyDigital)
      {
        for (int i = 0; i < MAXDPIDS; i++)
          Track()->SetAvailableTrack(ttDolby, i, channel.Dpid(i), channel.Dlang(i));
      }
      for (int i = 0; i < MAXSPIDS; i++)
        Track()->SetAvailableTrack(ttSubtitle, i, channel.Spid(i), channel.Slang(i));
      if (!NeedsTransferMode)
        Track()->EnsureAudioTrack(true);
      Track()->EnsureSubtitleTrack();
    }
    cStatus::MsgChannelSwitch(Device(), channel.Number(), bLiveView); // only report status if channel switch successful
  }

  return Result;
}
