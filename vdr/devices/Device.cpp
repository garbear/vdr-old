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

#include "Device.h"

#include "DeviceManager.h"
#include "subsystems/DeviceAudioSubsystem.h"
#include "subsystems/DeviceChannelSubsystem.h"
#include "subsystems/DeviceCommonInterfaceSubsystem.h"
#include "subsystems/DeviceImageGrabSubsystem.h"
#include "subsystems/DevicePIDSubsystem.h"
#include "subsystems/DevicePlayerSubsystem.h"
#include "subsystems/DeviceReceiverSubsystem.h"
#include "subsystems/DeviceSectionFilterSubsystem.h"
#include "subsystems/DeviceSPUSubsystem.h"
#include "subsystems/DeviceTrackSubsystem.h"
#include "subsystems/DeviceVideoFormatSubsystem.h"
#include "utils/StringUtils.h"
//#include "utils/Ringbuffer.h"
//#include "CI.h"

using namespace std;

#define TS_SCRAMBLING_TIMEOUT     3 // seconds to wait until a TS becomes unscrambled
#define TS_SCRAMBLING_TIME_OK    10 // seconds before a Channel/CAM combination is marked as known to decrypt

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) \
  do \
  { \
    delete p; p = NULL; \
  } while (0)
#endif

// --- cSubsystems -----------------------------------------------------------
void cSubsystems::Free() const
{
  delete Audio;
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
  assert(Audio);
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

// --- cDeviceHook -----------------------------------------------------------

cDeviceHook::cDeviceHook()
{
  cDeviceManager::Get().AddHook(this);
}

// --- cDevice ---------------------------------------------------------------

cDevice::cDevice(const cSubsystems &subsystems, unsigned int index)
 : m_subsystems(subsystems),
   m_cardIndex(index)
{
  m_subsystems.AssertValid();

  //dsyslog("new device number %d", m_cardIndex + 1);

  string strThreadDescription = StringUtils::Format("Receiver on device %d", m_cardIndex + 1);

  // TODO: Verify that AddDevice() is called outside the constructor after constructing this class
  //m_number = cDeviceManager::Get().AddDevice(this);
}

// TODO: Remove the following comment unless we switch cSubsystems to use shared_ptrs
// Destructor can't appear in class declaration because subsystem classes (needed
// by shared_ptr's destructor) aren't defined
cDevice::~cDevice()
{
  m_subsystems.Free(); // TODO: Remove me if we switch cSubsystems to use shared_ptrs
}

bool cDevice::IsPrimaryDevice() const
{
  return cDeviceManager::Get().PrimaryDevice() == this && HasDecoder();
}

void cDevice::MakePrimaryDevice(bool bOn)
{
  // XXX do we need this?
}

void *cDevice::Process()
{
  if (!IsStopped() && Receiver()->OpenDvr())
  {
    while (!IsStopped())
    {
      // Read data from the DVR device:
      uchar *b = NULL;
      if (Receiver()->GetTSPacket(b))
      {
        if (b)
        {
          int Pid = TsPid(b);

          // Check whether the TS packets are scrambled:
          bool DetachReceivers = false;
          bool DescramblingOk = false;
          int CamSlotNumber = 0;
          if (CommonInterface()->m_startScrambleDetection)
          {
            /* TODO
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
            */
          }

          // Distribute the packet to all attached receivers:
          Lock();
          for (int i = 0; i < MAXRECEIVERS; i++)
          {
            if (Receiver()->m_receivers[i] && Receiver()->m_receivers[i]->WantsPid(Pid))
            {
              if (DetachReceivers)
              {
                //ChannelCamRelations.SetChecked(Receiver()->m_receivers[i]->ChannelID(), CamSlotNumber); // TODO
                Receiver()->Detach(Receiver()->m_receivers[i]);
              }
              else
                Receiver()->m_receivers[i]->Receive(b, TS_SIZE);
              if (DescramblingOk)
                ;//ChannelCamRelations.SetDecrypt(Receiver()->m_receivers[i]->ChannelID(), CamSlotNumber); // TODO
            }
          }
          Unlock();
        }
      }
      else
        break;
    }
    Receiver()->CloseDvr();
  }
  return NULL;
}
