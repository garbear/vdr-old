/*
 * eitscan.c: EIT scanner
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: eitscan.c 2.7 2012/04/07 14:39:28 kls Exp $
 */

#include "EITScan.h"
#include <stdlib.h>
#include "channels/ChannelManager.h"
#include "devices/Transfer.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"

// --- cScanData -------------------------------------------------------------

cScanData::cScanData(const cChannel* channel)
{
  m_channel = new cChannel(*channel);
}

cScanData::~cScanData(void)
{
  delete m_channel;
}

int cScanData::Source(void) const
{
  return m_channel->Source();
}

int cScanData::Transponder(void) const
{
  return m_channel->Transponder();
}

int cScanData::Compare(const cListObject &ListObject) const
{
  const cScanData *sd = (const cScanData *)&ListObject;
  int r = Source() - sd->Source();
  if (r == 0)
     r = Transponder() - sd->Transponder();
  return r;
}

// --- cScanList -------------------------------------------------------------

void cScanList::AddTransponders(const cTransponderList& Channels)
{
  //TODO convert to std::vector
}

void cScanList::AddTransponders(const cChannelManager& channels)
{
  channels.AddTransponders(this);
  Sort();
}

void cScanList::AddTransponder(const cChannel *Channel)
{
  if (Channel->Source() && Channel->Transponder()) {
     for (cScanData *sd = First(); sd; sd = Next(sd)) {
         if (sd->Source() == Channel->Source() && ISTRANSPONDER(sd->Transponder(), Channel->Transponder()))
            return;
         }
     Add(new cScanData(Channel));
     }
}

// --- cTransponderList ------------------------------------------------------

class cTransponderList : public cList<cChannel> {
public:
  void AddTransponder(cChannel *Channel);
  };

void cTransponderList::AddTransponder(cChannel *Channel)
{
  for (cChannel *ch = First(); ch; ch = Next(ch)) {
      if (ch->Source() == Channel->Source() && ch->Transponder() == Channel->Transponder()) {
         delete Channel;
         return;
         }
      }
  Add(Channel);
}

// --- cEITScanner -----------------------------------------------------------

cEITScanner EITScanner;

cEITScanner::cEITScanner(void)
{
  lastScan = lastActivity = time(NULL);
  currentChannel = 0;
  scanList = NULL;
  transponderList = NULL;
}

cEITScanner::~cEITScanner()
{
  delete scanList;
  delete transponderList;
}

void cEITScanner::AddTransponder(cChannel *Channel)
{
  if (!transponderList)
     transponderList = new cTransponderList;
  transponderList->AddTransponder(Channel);
}

void cEITScanner::ForceScan(void)
{
  lastActivity = 0;
}

void cEITScanner::Activity(void)
{
  if (currentChannel) {
     Channels.SwitchTo(currentChannel);
     currentChannel = 0;
     }
  lastActivity = time(NULL);
}

void cEITScanner::Process(void)
{
  if (Setup.EPGScanTimeout || !lastActivity) { // !lastActivity means a scan was forced
     time_t now = time(NULL);
     if (now - lastScan > ScanTimeout && now - lastActivity > ActivityTimeout) {
        if (Channels.Lock(false, 10)) {
           if (!scanList) {
              scanList = new cScanList;
              if (transponderList) {
                 scanList->AddTransponders(*transponderList);
                 delete transponderList;
                 transponderList = NULL;
                 }
              scanList->AddTransponders(Channels);
              }
           bool AnyDeviceSwitched = false;
           for (int i = 0; i < cDeviceManager::Get().NumDevices(); i++) {
               cDevice *Device = cDeviceManager::Get().GetDevice(i);
               if (Device && Device->Channel()->ProvidesEIT()) {
                  for (cScanData *ScanData = scanList->First(); ScanData; ScanData = scanList->Next(ScanData)) {
                      const cChannel *Channel = ScanData->GetChannel();
                      if (Channel) {
                         if (!Channel->Ca() || Channel->Ca() == Device->DeviceNumber() + 1 || Channel->Ca() >= CA_ENCRYPTED_MIN) {
                            if (Device->Channel()->ProvidesTransponder(*Channel)) {
                               if (Device->Receiver()->Priority() < 0) {
                                  bool MaySwitchTransponder = Device->Channel()->MaySwitchTransponder(*Channel);
                                  if (MaySwitchTransponder || Device->Channel()->ProvidesTransponderExclusively(*Channel) && now - lastActivity > Setup.EPGScanTimeout * 3600) {
                                     if (!MaySwitchTransponder) {
                                        if (Device == cDeviceManager::Get().ActualDevice() && !currentChannel) {
                                           cDeviceManager::Get().PrimaryDevice()->Player()->StopReplay(); // stop transfer mode
                                           currentChannel = cDeviceManager::Get().CurrentChannel();
                                           isyslog(tr("Starting EPG scan"));
                                           }
                                        }
                                     //dsyslog("EIT scan: device %d  source  %-8s tp %5d", Device->DeviceNumber() + 1, *cSource::ToString(Channel->Source()), Channel->Transponder());
                                     Device->Channel()->SwitchChannel(*Channel, false);
                                     scanList->Del(ScanData);
                                     AnyDeviceSwitched = true;
                                     break;
                                     }
                                  }
                               }
                            }
                         }
                      }
                  }
               }
           if (!scanList->Count() || !AnyDeviceSwitched) {
              delete scanList;
              scanList = NULL;
              if (lastActivity == 0) // this was a triggered scan
                 Activity();
              }
           Channels.Unlock();
           }
        lastScan = time(NULL);
        }
     }
}
