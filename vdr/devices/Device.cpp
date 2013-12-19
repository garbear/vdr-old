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
#include "utils/Ringbuffer.h"
#include "CI.h"

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

// --- cTSBuffer -------------------------------------------------------------

cTSBuffer::cTSBuffer(int file, unsigned int size, int cardIndex)
 : m_file(file),
   m_cardIndex(cardIndex),
   m_bDelivered(false)
{
  SetDescription("TS buffer on device %d", cardIndex);

  m_ringBuffer = new cRingBufferLinear(size, TS_SIZE, true, "TS");
  m_ringBuffer->SetTimeouts(100, 100);
  m_ringBuffer->SetIoThrottle();
  Start();
}

cTSBuffer::~cTSBuffer()
{
  Cancel(3);
  delete m_ringBuffer;
}

void cTSBuffer::Action()
{
  if (m_ringBuffer)
  {
    bool firstRead = true;
    cPoller Poller(m_file);
    while (Running())
    {
      if (firstRead || Poller.Poll(100))
      {
        firstRead = false;
        int r = m_ringBuffer->Read(m_file);
        if (r < 0 && FATALERRNO)
        {
          if (errno == EOVERFLOW)
            esyslog("ERROR: driver buffer overflow on device %d", m_cardIndex);
          else
          {
            LOG_ERROR;
            break;
          }
        }
      }
    }
  }
}

uchar *cTSBuffer::Get()
{
  int Count = 0;
  if (m_bDelivered)
  {
    m_ringBuffer->Del(TS_SIZE);
    m_bDelivered = false;
  }
  uchar *p = m_ringBuffer->Get(Count);
  if (p && Count >= TS_SIZE)
  {
    if (*p != TS_SYNC_BYTE)
    {
      for (int i = 1; i < Count; i++)
      {
        if (p[i] == TS_SYNC_BYTE)
        {
          Count = i;
          break;
        }
      }
      m_ringBuffer->Del(Count);
      esyslog("ERROR: skipped %d bytes to sync on TS packet on device %d", Count, m_cardIndex);
      return NULL;
    }
    m_bDelivered = true;
    return p;
  }
  return NULL;
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
  dsyslog("new device number %d", m_cardIndex + 1);

  string strThreadDescription = StringUtils::Format("Receiver on device %d", m_cardIndex + 1);

  m_number = cDeviceManager::Get().AddDevice(this);
}

bool cDevice::IsPrimaryDevice() const
{
  return cDeviceManager::Get().PrimaryDevice() == this && HasDecoder();
}

void cDevice::MakePrimaryDevice(bool bOn)
{
  if (!bOn)
  {
    SAFE_DELETE(m_liveSubtitle);
  }
}

void cDevice::Action()
{
  if (Running() && Receiver()->OpenDvr())
  {
    while (Running())
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
          for (int i = 0; i < MAXRECEIVERS; i++)
          {
            if (Receiver()->m_receivers[i] && Receiver()->m_receivers[i]->WantsPid(Pid))
            {
              if (DetachReceivers)
              {
                ChannelCamRelations.SetChecked(Receiver()->m_receivers[i]->ChannelID(), CamSlotNumber);
                Receiver()->Detach(Receiver()->m_receivers[i]);
              }
              else
                Receiver()->m_receivers[i]->Receive(b, TS_SIZE);
              if (DescramblingOk)
                ChannelCamRelations.SetDecrypt(Receiver()->m_receivers[i]->ChannelID(), CamSlotNumber);
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
}
