/*
 * statemachine.h: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * wirbelscan-0.0.5
 *
 * $Id$ v20101210
 */

#ifndef __WIRBELSCAN_STATEMACHINE_H_
#define __WIRBELSCAN_STATEMACHINE_H_

#include <vdr/tools.h>
#include <vdr/device.h>

/* wirbelscan-0.0.5
 */

class cStateMachine : public cThread {
private:
  enum eState {
    eStart = 0,             // init, add next_from_list to NewTransponders      (NextTransponder)
    eStop,                  // cleanup and leave loop                           ()
    eTune,                  // tune, check for lock                             (AttachReceiver, NextTransponder)
    eNextTransponder,       // next unsearched transponder from NewTransponders (Tune, Stop)
    eDetachReceiver,        // detach all receivers                             (NextTransponder)
    eScanPat,               // pat/pmt scan                                     (ScanNit)
    eScanNit,               // nit scan                                         (ScanNitOther)
    eScanSdt,               // sdt scan                                         (ScanSdtOther)
    eScanEit,               // eit scan                                         (DetachReceiver)
    eUnknown,               // oops                                             (Stop)
    eAddChannels,           // adding results
    };

  eState     state, lastState;
  cChannel * initial;
  cDevice *  dev;
  bool       stop;
  cCondWait  cWait;
  bool       useNit;

protected:
  virtual void Action(void);
  virtual void Report(eState State);
public:
  cStateMachine(cDevice * Dev, cChannel * InitialTransponder, bool UseNit);
  virtual ~cStateMachine(void);
  void DoStop() {
    stop = true;
    }
  };

#endif
