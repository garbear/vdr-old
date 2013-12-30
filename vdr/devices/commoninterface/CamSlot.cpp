/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "CamSlot.h"
#include "CI.h"
#include "channels/Channel.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceCommonInterfaceSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"

//#include "Config.h"
//#include "dvb/PAT.h"
//#include "utils/Tools.h"

#include <ctype.h>
#include <linux/dvb/ca.h>
#include <malloc.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

cCamSlots CamSlots;

#define MODULE_CHECK_INTERVAL 500 // ms
#define MODULE_RESET_TIMEOUT    2 // s

cCamSlot::cCamSlot(cCiAdapter *CiAdapter)
{
  ciAdapter = CiAdapter;
  slotIndex = -1;
  lastModuleStatus = msReset; // avoids initial reset log message
  resetTime = 0;
  resendPmt = false;
  source = transponder = 0;
  for (int i = 0; i <= MAX_CONNECTIONS_PER_CAM_SLOT; i++) // tc[0] is not used, but initialized anyway
      tc[i] = NULL;
  CamSlots.Add(this);
  slotNumber = Index() + 1;
  bProcessed = false;
  if (ciAdapter)
     ciAdapter->AddCamSlot(this);
  Reset();
}

cCamSlot::~cCamSlot()
{
  CamSlots.Del(this, false);
  DeleteAllConnections();
}

bool cCamSlot::Assign(cDevice *Device, bool Query)
{
  CLockObject lock(mutex);
  if (ciAdapter) {
     if (ciAdapter->Assign(Device, true)) {
        if (!Device && ciAdapter->assignedDevice)
           ciAdapter->assignedDevice->CommonInterface()->SetCamSlot(NULL);
        if (!Query) {
           StopDecrypting();
           source = transponder = 0;
           if (ciAdapter->Assign(Device)) {
              ciAdapter->assignedDevice = Device;
              if (Device) {
                 Device->CommonInterface()->SetCamSlot(this);
                 dsyslog("CAM %d: assigned to device %d", slotNumber, Device->DeviceNumber() + 1);
                 }
              else
                 dsyslog("CAM %d: unassigned", slotNumber);
              }
           else
              return false;
           }
        return true;
        }
     }
  return false;
}

cDevice *cCamSlot::Device(void)
{
  CLockObject lock(mutex);
  if (ciAdapter) {
     cDevice *d = ciAdapter->assignedDevice;
     if (d && d->CommonInterface()->CamSlot() == this)
        return d;
     }
  return NULL;
}

void cCamSlot::NewConnection(void)
{
  CLockObject lock(mutex);
  for (int i = 1; i <= MAX_CONNECTIONS_PER_CAM_SLOT; i++) {
      if (!tc[i]) {
         tc[i] = new cCiTransportConnection(this, i);
         tc[i]->CreateConnection();
         return;
         }
      }
  esyslog("ERROR: CAM %d: can't create new transport connection!", slotNumber);
}

void cCamSlot::DeleteAllConnections(void)
{
  CLockObject lock(mutex);
  for (int i = 1; i <= MAX_CONNECTIONS_PER_CAM_SLOT; i++) {
      delete tc[i];
      tc[i] = NULL;
      }
}

void cCamSlot::Process(cTPDU *TPDU)
{
  CLockObject lock(mutex);
  if (TPDU) {
     int n = TPDU->Tcid();
     if (1 <= n && n <= MAX_CONNECTIONS_PER_CAM_SLOT) {
        if (tc[n])
           tc[n]->Process(TPDU);
        }
     }
  for (int i = 1; i <= MAX_CONNECTIONS_PER_CAM_SLOT; i++) {
      if (tc[i]) {
         if (!tc[i]->Process()) {
           Reset();
           return;
           }
         }
      }
  if (moduleCheckTimer.TimedOut()) {
     eModuleStatus ms = ModuleStatus();
     if (ms != lastModuleStatus) {
        switch (ms) {
          case msNone:
               dbgprotocol("Slot %d: no module present\n", slotNumber);
               isyslog("CAM %d: no module present", slotNumber);
               DeleteAllConnections();
               break;
          case msReset:
               dbgprotocol("Slot %d: module reset\n", slotNumber);
               isyslog("CAM %d: module reset", slotNumber);
               DeleteAllConnections();
               break;
          case msPresent:
               dbgprotocol("Slot %d: module present\n", slotNumber);
               isyslog("CAM %d: module present", slotNumber);
               break;
          case msReady:
               dbgprotocol("Slot %d: module ready\n", slotNumber);
               isyslog("CAM %d: module ready", slotNumber);
               NewConnection();
               resendPmt = caProgramList.Count() > 0;
               break;
          default:
               esyslog("ERROR: unknown module status %d (%s)", ms, __FUNCTION__);
          }
        lastModuleStatus = ms;
        }
     moduleCheckTimer.Set(MODULE_CHECK_INTERVAL);
     }
  if (resendPmt)
     SendCaPmt(CPCI_OK_DESCRAMBLING);
  bProcessed = true;
  processed.Broadcast();
}

cCiSession *cCamSlot::GetSessionByResourceId(uint32_t ResourceId)
{
  CLockObject lock(mutex);
  return tc[1] ? tc[1]->GetSessionByResourceId(ResourceId) : NULL;
}

void cCamSlot::Write(cTPDU *TPDU)
{
  CLockObject lock(mutex);
  if (ciAdapter && TPDU->Size()) {
     TPDU->Dump(SlotNumber(), true);
     ciAdapter->Write(TPDU->Buffer(), TPDU->Size());
     }
}

bool cCamSlot::Reset(void)
{
  CLockObject lock(mutex);
  ChannelCamRelations.Reset(slotNumber);
  DeleteAllConnections();
  if (ciAdapter) {
     dbgprotocol("Slot %d: reset...", slotNumber);
     if (ciAdapter->Reset(slotIndex)) {
        resetTime = time(NULL);
        dbgprotocol("ok.\n");
        lastModuleStatus = msReset;
        return true;
        }
     dbgprotocol("failed!\n");
     }
  return false;
}

eModuleStatus cCamSlot::ModuleStatus(void)
{
  CLockObject lock(mutex);
  eModuleStatus ms = ciAdapter ? ciAdapter->ModuleStatus(slotIndex) : msNone;
  if (resetTime) {
     if (ms <= msReset) {
        if (time(NULL) - resetTime < MODULE_RESET_TIMEOUT)
           return msReset;
        }
     resetTime = 0;
     }
  return ms;
}

const char *cCamSlot::GetCamName(void)
{
  CLockObject lock(mutex);
  return tc[1] ? tc[1]->GetCamName() : NULL;
}

bool cCamSlot::Ready(void)
{
  CLockObject lock(mutex);
  return ModuleStatus() == msNone || tc[1] && tc[1]->Ready();
}

bool cCamSlot::HasMMI(void)
{
  return GetSessionByResourceId(RI_MMI);
}

bool cCamSlot::HasUserIO(void)
{
  CLockObject lock(mutex);
  return tc[1] && tc[1]->HasUserIO();
}

bool cCamSlot::EnterMenu(void)
{
  CLockObject lock(mutex);
  cCiApplicationInformation *api = (cCiApplicationInformation *)GetSessionByResourceId(RI_APPLICATION_INFORMATION);
  return api ? api->EnterMenu() : false;
}

cCiMenu *cCamSlot::GetMenu(void)
{
  CLockObject lock(mutex);
  cCiMMI *mmi = (cCiMMI *)GetSessionByResourceId(RI_MMI);
  if (mmi) {
     cCiMenu *Menu = mmi->Menu();
     if (Menu)
       Menu->mutex = &mutex;
     return Menu;
     }
  return NULL;
}

cCiEnquiry *cCamSlot::GetEnquiry(void)
{
  CLockObject lock(mutex);
  cCiMMI *mmi = (cCiMMI *)GetSessionByResourceId(RI_MMI);
  if (mmi) {
     cCiEnquiry *Enquiry = mmi->Enquiry();
     if (Enquiry)
       Enquiry->mutex = &mutex;
     return Enquiry;
     }
  return NULL;
}

void cCamSlot::SendCaPmt(uint8_t CmdId)
{
  CLockObject lock(mutex);
  cCiConditionalAccessSupport *cas = (cCiConditionalAccessSupport *)GetSessionByResourceId(RI_CONDITIONAL_ACCESS_SUPPORT);
  if (cas) {
     const int *CaSystemIds = cas->GetCaSystemIds();
     if (CaSystemIds && *CaSystemIds) {
        if (caProgramList.Count()) {
           for (int Loop = 1; Loop <= 2; Loop++) {
               for (cCiCaProgramData *p = caProgramList.First(); p; p = caProgramList.Next(p)) {
                   if (p->modified || resendPmt) {
                      bool Active = false;
                      cCiCaPmt CaPmt(CmdId, source, transponder, p->programNumber, CaSystemIds);
                      for (cCiCaPidData *q = p->pidList.First(); q; q = p->pidList.Next(q)) {
                          if (q->active) {
                             CaPmt.AddPid(q->pid, q->streamType);
                             Active = true;
                             }
                          }
                      if ((Loop == 1) != Active) { // first remove, then add
                         if (cas->RepliesToQuery())
                            CaPmt.SetListManagement(Active ? CPLM_ADD : CPLM_UPDATE);
                         if (Active || cas->RepliesToQuery())
                            cas->SendPMT(&CaPmt);
                         p->modified = false;
                         }
                      }
                   }
               }
           resendPmt = false;
           }
        else {
           cCiCaPmt CaPmt(CmdId, 0, 0, 0, NULL);
           cas->SendPMT(&CaPmt);
           }
        }
     }
}

const int *cCamSlot::GetCaSystemIds(void)
{
  CLockObject lock(mutex);
  cCiConditionalAccessSupport *cas = (cCiConditionalAccessSupport *)GetSessionByResourceId(RI_CONDITIONAL_ACCESS_SUPPORT);
  return cas ? cas->GetCaSystemIds() : NULL;
}

int cCamSlot::Priority(void)
{
  cDevice *d = Device();
  return d ? d->Receiver()->Priority() : IDLEPRIORITY;
}

bool cCamSlot::ProvidesCa(const int *CaSystemIds)
{
  CLockObject lock(mutex);
  cCiConditionalAccessSupport *cas = (cCiConditionalAccessSupport *)GetSessionByResourceId(RI_CONDITIONAL_ACCESS_SUPPORT);
  if (cas) {
     for (const int *ids = cas->GetCaSystemIds(); ids && *ids; ids++) {
         for (const int *id = CaSystemIds; *id; id++) {
             if (*id == *ids)
                return true;
             }
         }
     }
  return false;
}

void cCamSlot::AddPid(int ProgramNumber, int Pid, int StreamType)
{
  CLockObject lock(mutex);
  cCiCaProgramData *ProgramData = NULL;
  for (cCiCaProgramData *p = caProgramList.First(); p; p = caProgramList.Next(p)) {
      if (p->programNumber == ProgramNumber) {
         ProgramData = p;
         for (cCiCaPidData *q = p->pidList.First(); q; q = p->pidList.Next(q)) {
             if (q->pid == Pid)
                return;
             }
         }
      }
  if (!ProgramData)
     caProgramList.Add(ProgramData = new cCiCaProgramData(ProgramNumber));
  ProgramData->pidList.Add(new cCiCaPidData(Pid, StreamType));
}

void cCamSlot::SetPid(int Pid, bool Active)
{
  CLockObject lock(mutex);
  for (cCiCaProgramData *p = caProgramList.First(); p; p = caProgramList.Next(p)) {
      for (cCiCaPidData *q = p->pidList.First(); q; q = p->pidList.Next(q)) {
          if (q->pid == Pid) {
             if (q->active != Active) {
                q->active = Active;
                p->modified = true;
                }
             return;
             }
         }
      }
}

// see ISO/IEC 13818-1
#define STREAM_TYPE_VIDEO    0x02
#define STREAM_TYPE_AUDIO    0x04
#define STREAM_TYPE_PRIVATE  0x06

void cCamSlot::AddChannel(const cChannel *Channel)
{
  CLockObject lock(mutex);
  if (source != Channel->Source() || transponder != Channel->Transponder())
     StopDecrypting();
  source = Channel->Source();
  transponder = Channel->Transponder();
  if (Channel->Ca() >= CA_ENCRYPTED_MIN) {
     AddPid(Channel->Sid(), Channel->Vpid(), STREAM_TYPE_VIDEO);
     for (const int *Apid = Channel->Apids(); *Apid; Apid++)
         AddPid(Channel->Sid(), *Apid, STREAM_TYPE_AUDIO);
     for (const int *Dpid = Channel->Dpids(); *Dpid; Dpid++)
         AddPid(Channel->Sid(), *Dpid, STREAM_TYPE_PRIVATE);
     for (const int *Spid = Channel->Spids(); *Spid; Spid++)
         AddPid(Channel->Sid(), *Spid, STREAM_TYPE_PRIVATE);
     }
}

#define QUERY_REPLY_WAIT  100 // ms to wait between checks for a reply

bool cCamSlot::CanDecrypt(const cChannel *Channel)
{
  if (Channel->Ca() < CA_ENCRYPTED_MIN)
     return true; // channel not encrypted
  if (!IsDecrypting())
     return true; // any CAM can decrypt at least one channel
  CLockObject lock(mutex);
  cCiConditionalAccessSupport *cas = (cCiConditionalAccessSupport *)GetSessionByResourceId(RI_CONDITIONAL_ACCESS_SUPPORT);
  if (cas && cas->RepliesToQuery()) {
     cCiCaPmt CaPmt(CPCI_QUERY, Channel->Source(), Channel->Transponder(), Channel->Sid(), GetCaSystemIds());
     CaPmt.SetListManagement(CPLM_ADD); // WORKAROUND: CPLM_ONLY doesn't work with Alphacrypt 3.09 (deletes existing CA_PMTs)
     CaPmt.AddPid(Channel->Vpid(), STREAM_TYPE_VIDEO);
     for (const int *Apid = Channel->Apids(); *Apid; Apid++)
         CaPmt.AddPid(*Apid, STREAM_TYPE_AUDIO);
     for (const int *Dpid = Channel->Dpids(); *Dpid; Dpid++)
         CaPmt.AddPid(*Dpid, STREAM_TYPE_PRIVATE);
     for (const int *Spid = Channel->Spids(); *Spid; Spid++)
         CaPmt.AddPid(*Spid, STREAM_TYPE_PRIVATE);
     cas->SendPMT(&CaPmt);
     cTimeMs Timeout(QUERY_REPLY_TIMEOUT);
     do {
        if (processed.Wait(mutex, bProcessed, QUERY_REPLY_WAIT))
          bProcessed = false;
        if ((cas = (cCiConditionalAccessSupport *)GetSessionByResourceId(RI_CONDITIONAL_ACCESS_SUPPORT)) != NULL) { // must re-fetch it, there might have been a reset
           if (cas->ReceivedReply())
              return cas->CanDecrypt();
           }
        else
           return false;
        } while (!Timeout.TimedOut());
     dsyslog("CAM %d: didn't reply to QUERY", SlotNumber());
     }
  return false;
}

void cCamSlot::StartDecrypting(void)
{
  SendCaPmt(CPCI_OK_DESCRAMBLING);
}

void cCamSlot::StopDecrypting(void)
{
  CLockObject lock(mutex);
  if (caProgramList.Count()) {
     caProgramList.Clear();
     SendCaPmt(CPCI_NOT_SELECTED);
     }
}

bool cCamSlot::IsDecrypting(void)
{
  CLockObject lock(mutex);
  if (caProgramList.Count()) {
     for (cCiCaProgramData *p = caProgramList.First(); p; p = caProgramList.Next(p)) {
         if (p->modified)
            return true; // any modifications need to be processed before we can assume it's no longer decrypting
         for (cCiCaPidData *q = p->pidList.First(); q; q = p->pidList.Next(q)) {
             if (q->active)
                return true;
             }
         }
     }
  return false;
}

// --- cChannelCamRelation ---------------------------------------------------

#define CAM_CHECKED_TIMEOUT  15 // seconds before a CAM that has been checked for a particular channel will be checked again

class cChannelCamRelation : public cListObject {
private:
  tChannelID channelID;
  uint32_t camSlotsChecked;
  uint32_t camSlotsDecrypt;
  time_t lastChecked;
public:
  cChannelCamRelation(tChannelID ChannelID);
  bool TimedOut(void);
  tChannelID ChannelID(void) { return channelID; }
  bool CamChecked(int CamSlotNumber);
  bool CamDecrypt(int CamSlotNumber);
  void SetChecked(int CamSlotNumber);
  void SetDecrypt(int CamSlotNumber);
  void ClrChecked(int CamSlotNumber);
  void ClrDecrypt(int CamSlotNumber);
  };

cChannelCamRelation::cChannelCamRelation(tChannelID ChannelID)
{
  channelID = ChannelID;
  camSlotsChecked = 0;
  camSlotsDecrypt = 0;
  lastChecked = 0;
}

bool cChannelCamRelation::TimedOut(void)
{
  return !camSlotsDecrypt && time(NULL) - lastChecked > CAM_CHECKED_TIMEOUT;
}

bool cChannelCamRelation::CamChecked(int CamSlotNumber)
{
  if (lastChecked && time(NULL) - lastChecked > CAM_CHECKED_TIMEOUT) {
     lastChecked = 0;
     camSlotsChecked = 0;
     }
  return camSlotsChecked & (1 << (CamSlotNumber - 1));
}

bool cChannelCamRelation::CamDecrypt(int CamSlotNumber)
{
  return camSlotsDecrypt & (1 << (CamSlotNumber - 1));
}

void cChannelCamRelation::SetChecked(int CamSlotNumber)
{
  camSlotsChecked |= (1 << (CamSlotNumber - 1));
  lastChecked = time(NULL);
  ClrDecrypt(CamSlotNumber);
}

void cChannelCamRelation::SetDecrypt(int CamSlotNumber)
{
  camSlotsDecrypt |= (1 << (CamSlotNumber - 1));
  ClrChecked(CamSlotNumber);
}

void cChannelCamRelation::ClrChecked(int CamSlotNumber)
{
  camSlotsChecked &= ~(1 << (CamSlotNumber - 1));
  lastChecked = 0;
}

void cChannelCamRelation::ClrDecrypt(int CamSlotNumber)
{
  camSlotsDecrypt &= ~(1 << (CamSlotNumber - 1));
}

// --- cChannelCamRelations --------------------------------------------------

#define CHANNEL_CAM_RELATIONS_CLEANUP_INTERVAL 3600 // seconds between cleanups

cChannelCamRelations ChannelCamRelations;

cChannelCamRelations::cChannelCamRelations(void)
{
  lastCleanup = time(NULL);
}

void cChannelCamRelations::Cleanup(void)
{
  CLockObject lock(mutex);
  if (time(NULL) - lastCleanup > CHANNEL_CAM_RELATIONS_CLEANUP_INTERVAL) {
     for (cChannelCamRelation *ccr = First(); ccr; ) {
         cChannelCamRelation *c = ccr;
         ccr = Next(ccr);
         if (c->TimedOut())
            Del(c);
         }
     lastCleanup = time(NULL);
     }
}

cChannelCamRelation *cChannelCamRelations::GetEntry(tChannelID ChannelID)
{
  CLockObject lock(mutex);
  Cleanup();
  for (cChannelCamRelation *ccr = First(); ccr; ccr = Next(ccr)) {
      if (ccr->ChannelID() == ChannelID)
         return ccr;
      }
  return NULL;
}

cChannelCamRelation *cChannelCamRelations::AddEntry(tChannelID ChannelID)
{
  CLockObject lock(mutex);
  cChannelCamRelation *ccr = GetEntry(ChannelID);
  if (!ccr)
     Add(ccr = new cChannelCamRelation(ChannelID));
  return ccr;
}

void cChannelCamRelations::Reset(int CamSlotNumber)
{
  CLockObject lock(mutex);
  for (cChannelCamRelation *ccr = First(); ccr; ccr = Next(ccr)) {
      ccr->ClrChecked(CamSlotNumber);
      ccr->ClrDecrypt(CamSlotNumber);
      }
}

bool cChannelCamRelations::CamChecked(tChannelID ChannelID, int CamSlotNumber)
{
  CLockObject lock(mutex);
  cChannelCamRelation *ccr = GetEntry(ChannelID);
  return ccr ? ccr->CamChecked(CamSlotNumber) : false;
}

bool cChannelCamRelations::CamDecrypt(tChannelID ChannelID, int CamSlotNumber)
{
  CLockObject lock(mutex);
  cChannelCamRelation *ccr = GetEntry(ChannelID);
  return ccr ? ccr->CamDecrypt(CamSlotNumber) : false;
}

void cChannelCamRelations::SetChecked(tChannelID ChannelID, int CamSlotNumber)
{
  CLockObject lock(mutex);
  cChannelCamRelation *ccr = AddEntry(ChannelID);
  if (ccr)
     ccr->SetChecked(CamSlotNumber);
}

void cChannelCamRelations::SetDecrypt(tChannelID ChannelID, int CamSlotNumber)
{
  CLockObject lock(mutex);
  cChannelCamRelation *ccr = AddEntry(ChannelID);
  if (ccr)
     ccr->SetDecrypt(CamSlotNumber);
}

void cChannelCamRelations::ClrChecked(tChannelID ChannelID, int CamSlotNumber)
{
  CLockObject lock(mutex);
  cChannelCamRelation *ccr = GetEntry(ChannelID);
  if (ccr)
     ccr->ClrChecked(CamSlotNumber);
}

void cChannelCamRelations::ClrDecrypt(tChannelID ChannelID, int CamSlotNumber)
{
  CLockObject lock(mutex);
  cChannelCamRelation *ccr = GetEntry(ChannelID);
  if (ccr)
     ccr->ClrDecrypt(CamSlotNumber);
}
