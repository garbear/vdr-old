/*
 * statemachine.c: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * wirbelscan-0.0.5
 *
 * $Id$ v20101210
 */

#include <stdarg.h>
#include <stdlib.h>
#include <vdr/tools.h>
#include "statemachine.h"
#include "scanfilter.h"
#include "common.h"
#include "dvb_wrapper.h"
#include "menusetup.h"
#include "si_ext.h"

using namespace SI_EXT;

///!-----------------------------------------------------------------
///!  v 0.0.5, a dummy receiver. Might be used real later.
///!-----------------------------------------------------------------

class cScanReceiver : public cReceiver, public cThread {
private:
protected:
  virtual void Activate(bool On) {
    if (On) {Start();  }
    else    {Cancel(0);}
    };
  virtual void Receive(uchar * Data, int Length) {};
  virtual void Action(void) {
    while (Running()) cCondWait::SleepMs(5);
    }; /*TODO: check here periodically for lock and wether we got any data!*/
public:
  cScanReceiver(tChannelID ChannelID, int AnyPid);
  virtual ~cScanReceiver() {cReceiver::Detach(); };
  };

cScanReceiver::cScanReceiver(tChannelID ChannelID, int AnyPid) :
     cReceiver(ChannelID, 99, AnyPid), cThread("dummy receiver") { }

///!-----------------------------------------------------------------
///!  v 0.0.5, store state in lastState if different and print state
///!-----------------------------------------------------------------

void cStateMachine::Report(eState State) {
  const char * stateMsg[] = { // be careful here: same order as eState
    "------- state: Start -------",
    "------- state: Stop -------",
    "------- state: Tune -------",
    "------- state: NextTransponder -------",
    "------- state: DetachReceiver -------",
    "------- state: ScanPat -------",
    "------- state: ScanNit -------",
    "------- state: ScanSdt -------",
    "------- state: ScanEit -------",
    "------- state: ERROR IN STATEMACHINE, UNKNOWN STATE. -------",
    "------- state: AddChannels -------",
    "------- NULL -------"
    };

  if (State != lastState) {
    dlog(4, "%s", stateMsg[lastState = State]);
    }
  };

///!-----------------------------------------------------------------
///!  v 0.0.5, StateMachine constructor
///!-----------------------------------------------------------------

cStateMachine::cStateMachine(cDevice * Dev, cChannel * InitialTransponder, bool UseNit) {
  dev     = Dev;
  stop    = false;
  initial = InitialTransponder;
  useNit  = UseNit;
  //  NewTransponders.Load(false, false);
  state = eStart;
  Start();
  }

///!-----------------------------------------------------------------
///!  v 0.0.5, StateMachine destructor
///!-----------------------------------------------------------------

cStateMachine::~cStateMachine(void) {
  stop = true;
  }

///!-----------------------------------------------------------------
///!  v 0.0.5, StateMachine itself
///!-----------------------------------------------------------------


void cStateMachine::Action(void) {
  cChannel      * Transponder        = NULL;
  cChannel      * ScannedTransponder = NULL;
  cChannel      * NewTransponder     = NULL;
  cScanReceiver * aReceiver          = NULL;
  cPatScanner   * PatScanner         = NULL;
  cNitScanner   * NitScanner         = NULL;
  cNitScanner   * NitOtherScanner    = NULL;
  cSdtScanner   * SdtScanner         = NULL;
  cEitScanner   * EitScanner         = NULL;
  eState          newState           = state;
  int             count = 0;
  bool            s2 = false;

  s2 = GetCapabilities(dev->CardIndex()) & 0x10000000;

  while (Running() && !stop) {
    cWait.Wait(10);
    Report(state);
    switch (state) {
       case eStart:
         Transponder = initial;
         newState    = eTune;
         ScannedTransponder = new cChannel(* Transponder);
         NewTransponders.Add(ScannedTransponder);
         break;

       case eStop:
         stop = true;
         goto DIRECT_EXIT;
         break;

       case eTune:
         if (is_known_initial_transponder(Transponder, false, &ScannedTransponders)) {
            newState = eNextTransponder;
            break;
            }
         lTransponder = PrintTransponder(Transponder);
         dlog(0, "   tuning to %s", *lTransponder);
         
         if (MenuScanning) {
           MenuScanning->SetTransponder(Transponder);
           MenuScanning->SetProgress(-1, DVB_TERR, -1);
           }

         ScannedTransponder = new cChannel(* Transponder);
         ScannedTransponders.Add(ScannedTransponder);

         dev->SwitchChannel(Transponder, false);
         aReceiver = new cScanReceiver(Transponder->GetChannelID(), 99);
         dev->AttachReceiver(aReceiver);

         cCondWait::SleepMs(1000);
         if (dev->HasLock(3000)) {
           newState = eScanNit;
           dlog(0, "   has lock.");
           }
         else {
           DELETENULL(aReceiver);
           newState = eNextTransponder;
           }
         lStrength = GetFrontendStrength(dev->CardIndex());
         if (MenuScanning)
           MenuScanning->SetStr(lStrength, dev->HasLock(1));
         break;

       case eNextTransponder:
         nextTransponders = NewTransponders.Count() - ScannedTransponders.Count();
         if (! useNit)
             goto DIRECT_EXIT;

         if (NewTransponder == NULL) {
           NewTransponder = NewTransponders.First();
           }

         NewTransponder = NewTransponders.Next(NewTransponder);

         if (NULL == (Transponder = NewTransponder)) {
           newState = eStop;
           }
         else {
           newState = eTune;
           }
         break;

       case eDetachReceiver:
         if (dev) {
           dev->DetachAllReceivers();
           }
         DELETENULL(aReceiver);
         if (stop) {
           newState = eStop;
           }
         else {
           newState = eNextTransponder;
           }
         break;

       case eScanNit:
         if (NULL == NitScanner) {
           NitScanner = new cNitScanner(TABLE_ID_NIT_ACTUAL);
           dev->AttachFilter(NitScanner);
           }
         if (NULL == NitOtherScanner) {
           NitOtherScanner = new cNitScanner(TABLE_ID_NIT_OTHER);
           dev->AttachFilter(NitOtherScanner);
           }
         if ((NitScanner != NULL) && (NitScanner != NULL)) {
           if (!NitScanner->Active() && !NitOtherScanner->Active()) {
             dev->Detach(NitScanner);
             DELETENULL(NitScanner);
             dev->Detach(NitOtherScanner);
             DELETENULL(NitOtherScanner);
             if (stop) {
               newState = eDetachReceiver;
               }
             else {
               newState = eScanPat;
               }
             }
           }
         break;

       case eScanPat:
         if (NULL == PatScanner) {
           PatScanner = new cPatScanner(dev);
           dev->AttachFilter(PatScanner);
           }
         else if (!PatScanner->Active()) {
           dev->Detach(PatScanner);
           DELETENULL(PatScanner);
           if (stop) {
             newState = eDetachReceiver;
             }
           else {
             if (NewChannels.Count()) {
               SdtScanner = NULL;
               newState = eScanSdt;
               }
             else
               newState = eDetachReceiver;
             }
           break;
           }
         break;

       case eScanSdt:
         if (NULL == SdtScanner) {
           SdtScanner = new cSdtScanner(TABLE_ID_SDT_ACTUAL);
           dev->AttachFilter(SdtScanner);
           }
         else if (!SdtScanner->Active()) {
           dev->Detach(SdtScanner);
           DELETENULL(SdtScanner);
           if (stop) {
             newState = eDetachReceiver;
             }
           else {
             newState = eAddChannels;
             }
           break;
           }
         break;

       case eAddChannels:
         if ((count = AddChannels())) {
           dlog(1, "added %d channels", count);
           }
         if (MenuScanning)
           MenuScanning->SetChan(count);
         EitScanner = NULL; 
         newState = eScanEit;
         break;

       case eScanEit:
         if (NULL == EitScanner) {{
           EitScanner = new cEitScanner();
           dev->AttachFilter(EitScanner);
           }
         cCondWait::SleepMs(10000);
         dev->Detach(EitScanner);
         DELETENULL(EitScanner);
         }

         newState = eDetachReceiver;
         break;

       case eUnknown:
         newState = eStop;
         break;
       default: newState = eUnknown; //every unhandled state should come here.
      }
    state = newState;
    }
DIRECT_EXIT:
  Cancel();
  }
