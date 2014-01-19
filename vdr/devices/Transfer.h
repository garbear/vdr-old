/*
 * transfer.h: Transfer mode
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: transfer.h 2.4 2013/03/01 09:49:46 kls Exp $
 */

#ifndef __TRANSFER_H
#define __TRANSFER_H

#include "Player.h"
#include "Receiver.h"
#include "Remux.h"

class cTransfer : public cReceiver, public cPlayer {
private:
  cPatPmtGenerator patPmtGenerator;
protected:
  virtual void Activate(bool On);
  virtual void Receive(uchar *Data, int Length);
public:
  cTransfer(ChannelPtr Channel);
  virtual ~cTransfer();
  };

class cDevice;

class cTransferControl : public cControl {
private:
  cTransfer* m_transfer;
  cDevice*   m_device;
public:
  cTransferControl(cDevice *ReceiverDevice, ChannelPtr Channel);
  ~cTransferControl();
  virtual void Hide(void) {}
  };

#endif //__TRANSFER_H
