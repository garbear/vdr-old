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

#include "Transfer.h"
#include "Device.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "platform/threads/mutex.h"
#include "utils/log/Log.h"

namespace VDR
{
// --- cTransfer -------------------------------------------------------------

cTransfer::cTransfer(ChannelPtr Channel)
:cReceiver(Channel)
{
  patPmtGenerator.SetChannel(Channel);
}

cTransfer::~cTransfer()
{
  cReceiver::Detach();
  cPlayer::Detach();
}

void cTransfer::Activate(bool On)
{
  if (On) {
     PlayTs(patPmtGenerator.GetPat(), TS_SIZE);
     int Index = 0;
     while (uint8_t *pmt = patPmtGenerator.GetPmt(Index))
           PlayTs(pmt, TS_SIZE);
     }
  else
     cPlayer::Detach();
}

#define MAXRETRIES    20 // max. number of retries for a single TS packet
#define RETRYWAIT      5 // time (in ms) between two retries

void cTransfer::Receive(uint8_t *Data, int Length)
{
  if (cPlayer::IsAttached()) {
     // Transfer Mode means "live tv", so there's no point in doing any additional
     // buffering here. The TS packets *must* get through here! However, every
     // now and then there may be conditions where the packet just can't be
     // handled when offered the first time, so that's why we try several times:
     for (int i = 0; i < MAXRETRIES; i++) {
         if (PlayTs(Data, Length) > 0)
            return;
         PLATFORM::CEvent::Sleep(RETRYWAIT);
         }
     DeviceClear();
     esyslog("ERROR: TS packet not accepted in Transfer Mode");
     }
}

// --- cTransferControl ------------------------------------------------------

cTransferControl::cTransferControl(cDevice *ReceiverDevice, ChannelPtr Channel)
:cControl(m_transfer = new cTransfer(Channel), true),
 m_device(ReceiverDevice)
{
  ReceiverDevice->Receiver()->AttachReceiver(m_transfer);
}

cTransferControl::~cTransferControl()
{
  delete m_transfer;
}

}
