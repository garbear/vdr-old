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
#include "devices/commoninterface/CI.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceCommonInterfaceSubsystem.h"
#include "devices/subsystems/DeviceImageGrabSubsystem.h"
#include "devices/subsystems/DevicePIDSubsystem.h"
#include "devices/subsystems/DevicePlayerSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceSectionFilterSubsystem.h"
#include "devices/subsystems/DeviceSPUSubsystem.h"
#include "devices/subsystems/DeviceTrackSubsystem.h"
#include "devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "dvb/filters/PSIP_MGT.h"
#include "epg/EPGTypes.h"
#include "epg/Event.h"
#include "epg/Schedules.h"
#include "utils/log/Log.h"

using namespace std;

namespace VDR
{

#define TS_SCRAMBLING_TIMEOUT     3 // seconds to wait until a TS becomes unscrambled
#define TS_SCRAMBLING_TIME_OK    10 // seconds before a Channel/CAM combination is marked as known to decrypt

DevicePtr cDevice::EmptyDevice;

// --- cSubsystems -----------------------------------------------------------
void cSubsystems::Free() const
{
  delete Channel;
  delete CommonInterface;
  delete ImageGrab;
  delete PID;
  delete Player;
  delete Receiver;
  delete SectionFilter;
  delete SPU;
  delete Track;
  delete VideoFormat;
}

void cSubsystems::AssertValid() const
{
  assert(Channel);
  assert(CommonInterface);
  assert(ImageGrab);
  assert(PID);
  assert(Player);
  assert(Receiver);
  assert(SectionFilter);
  assert(SPU);
  assert(Track);
  assert(VideoFormat);
}

// --- cDevice ---------------------------------------------------------------

cDevice::cDevice(const cSubsystems &subsystems)
 : m_subsystems(subsystems),
   m_bInitialised(false),
   m_cardIndex(0)
{
  m_subsystems.AssertValid();
}

cDevice::~cDevice()
{
  SectionFilter()->StopSectionHandler();
  m_subsystems.Free(); // TODO: Remove me if we switch cSubsystems to use shared_ptrs
}

bool cDevice::Initialise(void)
{
  m_bInitialised = true;
  return m_bInitialised;
}

void *cDevice::Process()
{
  if (!IsStopped() && Receiver()->OpenDvr())
  {
    while (!IsStopped())
    {
      // Read data from the DVR device:
      uint8_t *b = NULL;
      if (!Receiver()->GetTSPacket(b))
        break;

      if (!b)
        continue;

      int Pid = TsPid(b);

      // Check whether the TS packets are scrambled:
      bool DetachReceivers = false;
      bool DescramblingOk = false;
      int CamSlotNumber = 0;
      if (CommonInterface()->m_startScrambleDetection)
      {
        cCamSlot *cs = CommonInterface()->CamSlot();
        CamSlotNumber = cs ? cs->SlotNumber() : 0;
        if (CamSlotNumber)
        {
          bool Scrambled = b[3] & TS_SCRAMBLING_CONTROL;
          int t = time(NULL) - CommonInterface()->m_startScrambleDetection;
          if (Scrambled)
          {
            if (t > TS_SCRAMBLING_TIMEOUT)
              DetachReceivers = true;
          }
          else if (t > TS_SCRAMBLING_TIME_OK)
          {
            DescramblingOk = true;
            CommonInterface()->m_startScrambleDetection = 0;
          }
        }
      }

      // Distribute the packet to all attached receivers:
      Lock();
      for (std::list<cReceiver*>::iterator it = Receiver()->m_receivers.begin(); it != Receiver()->m_receivers.end(); ++it)
      {
        cReceiver* receiver = *it;
//        dsyslog("received packet: pid=%d scrambled=%d check receiver %p", Pid, b[3] & TS_SCRAMBLING_CONTROL?1:0, receiver);
        if (receiver->WantsPid(Pid))
        {
          if (DetachReceivers)
          {
            ChannelCamRelations.SetChecked(receiver->ChannelID(), CamSlotNumber);
            Receiver()->Detach(receiver);
          }
          else
            receiver->Receive(b, TS_SIZE);
          if (DescramblingOk)
            ChannelCamRelations.SetDecrypt(receiver->ChannelID(), CamSlotNumber);
        }
      }
      Unlock();
    }
    Receiver()->CloseDvr();
  }
  return NULL;
}

bool cDevice::ScanTransponder(const ChannelPtr& transponder)
{
  try
  {
    if (transponder->GetCaId(0) &&
        transponder->GetCaId(0) != CardIndex() &&
        transponder->GetCaId(0) < CA_ENCRYPTED_MIN)
    {
      throw "Failed to scan transponder: Channel cannot be decrypted";
    }

    if (!Channel()->ProvidesTransponder(*transponder))
      throw "Failed to scan transponder: Channel is not provided by device";

    if (!Channel()->MaySwitchTransponder(*transponder))
      throw "Failed to scan transponder: Not allowed to switch transponders";

    if (!Channel()->SwitchChannel(transponder))
      throw "Failed to scan transponder: Failed to switch transponders";

    dsyslog("EIT scan: device %d source %s tp %5d MHz", CardIndex(), transponder->Source().ToString().c_str(), transponder->FrequencyMHzWithPolarization());

    EventVector events;

    cPsipMgt mgt(this);
    if (!mgt.GetPSIPData(events))
      throw "Failed to scan transponder: Tuner failed to get lock";

    if (events.empty())
      throw "Scanned transponder, but no events discovered!";

    // Finally, success! Add channels to EPG schedule
    for (EventVector::const_iterator it = events.begin(); it != events.end(); ++it)
      cSchedulesLock::AddEvent(transponder->GetChannelID(), *it);
  }
  catch (const char* errorMsg)
  {
    dsyslog("%s", errorMsg);
    return false;
  }

  return true;

}

}
