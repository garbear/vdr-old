/*
 * transfer.c: Transfer mode
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: transfer.c 2.8.1.1 2013/08/22 12:37:02 kls Exp $
 */

#include "Transfer.h"
#include "subsystems/DeviceReceiverSubsystem.h"
#include "platform/threads/mutex.h"

namespace VDR
{
// --- cTransfer -------------------------------------------------------------

cTransfer::cTransfer(ChannelPtr Channel)
:cReceiver(Channel, TRANSFERPRIORITY)
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
