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

class cTransponderList
{
public:
  void AddTransponder(ChannelPtr Channel);
  void AddTranspondersToScanList(cScanList& scanlist) const;

private:
  std::vector<ChannelPtr> m_channels;
};

// --- cScanData -------------------------------------------------------------

cScanData::cScanData(ChannelPtr channel)
{
  m_channel = channel;
}

cScanData::~cScanData(void)
{
}

int
cScanData::Source(void) const
{
  return m_channel->Source();
}

int
cScanData::Transponder(void) const
{
  return m_channel->Transponder();
}

int
cScanData::Compare(const cListObject &ListObject) const
{
  const cScanData *sd = (const cScanData *) &ListObject;
  int r = Source() - sd->Source();
  if (r == 0)
    r = Transponder() - sd->Transponder();
  return r;
}

// --- cScanList -------------------------------------------------------------

void
cScanList::AddTransponders(const cTransponderList& Channels)
{
  Channels.AddTranspondersToScanList(*this);
  Sort();
}

void
cScanList::AddTransponders(const cChannelManager& channels)
{
  channels.AddTransponders(this);
  Sort();
}

void
cScanList::AddTransponder(ChannelPtr Channel)
{
  if (Channel->Source() && Channel->Transponder())
  {
    for (cScanData *sd = First(); sd; sd = Next(sd))
    {
      if (sd->Source() == Channel->Source()
          && ISTRANSPONDER(sd->Transponder(), Channel->Transponder()))
        return;
    }
    Add(new cScanData(Channel));
  }
}

// --- cTransponderList ------------------------------------------------------

void
cTransponderList::AddTransponder(ChannelPtr Channel)
{
  for (std::vector<ChannelPtr>::iterator it = m_channels.begin();
      it != m_channels.end(); ++it)
  {
    if ((*it)->Source() == Channel->Source()
        && (*it)->Transponder() == Channel->Transponder())
    {
      return;
    }
  }
  m_channels.push_back(Channel);
}

void
cTransponderList::AddTranspondersToScanList(cScanList& scanlist) const
{
  for (std::vector<ChannelPtr>::const_iterator it = m_channels.begin();
      it != m_channels.end(); ++it)
    scanlist.AddTransponder(*it);
}

// --- cEITScanner -----------------------------------------------------------

cEITScanner EITScanner;

cEITScanner::cEITScanner(void)
{
  lastScan = lastActivity = time(NULL);
  scanList = NULL;
  transponderList = NULL;
}

cEITScanner::~cEITScanner()
{
  delete scanList;
  delete transponderList;
}

void
cEITScanner::AddTransponder(ChannelPtr Channel)
{
  if (!transponderList)
    transponderList = new cTransponderList;
  transponderList->AddTransponder(Channel);
}

void
cEITScanner::ForceScan(void)
{
  lastActivity = 0;
}

void
cEITScanner::Activity(void)
{
  lastActivity = time(NULL);
}

void
cEITScanner::Process(void)
{
  if (Setup.EPGScanTimeout || !lastActivity)
  { // !lastActivity means a scan was forced
    time_t now = time(NULL);
    if (now - lastScan > ScanTimeout && now - lastActivity > ActivityTimeout)
    {
//        XXX if (cChannelManager::Get().Lock(false, 10)) {
      if (1)
      {
        if (!scanList)
        {
          scanList = new cScanList;
          if (transponderList)
          {
            scanList->AddTransponders(*transponderList);
            delete transponderList;
            transponderList = NULL;
          }
          scanList->AddTransponders(cChannelManager::Get());
        }
        bool AnyDeviceSwitched = false;
        for (int i = 0; i < cDeviceManager::Get().NumDevices(); i++)
        {
          DevicePtr Device = cDeviceManager::Get().GetDevice(i);
          if (Device != cDevice::EmptyDevice
              && Device->Channel()->ProvidesEIT())
          {
            for (cScanData *ScanData = scanList->First(); ScanData; ScanData =
                scanList->Next(ScanData))
            {
              ChannelPtr Channel = ScanData->GetChannel();
              if (Channel)
              {
                if (!Channel->Ca() || Channel->Ca() == Device->CardIndex()
                    || Channel->Ca() >= CA_ENCRYPTED_MIN)
                {
                  if (Device->Channel()->ProvidesTransponder(*Channel))
                  {
                    if (Device->Receiver()->Priority() < 0)
                    {
                      bool MaySwitchTransponder =
                          Device->Channel()->MaySwitchTransponder(*Channel);
                      if (MaySwitchTransponder
                          || Device->Channel()->ProvidesTransponderExclusively(
                              *Channel)
                              && now - lastActivity
                                  > Setup.EPGScanTimeout * 3600)
                      {
                        dsyslog("EIT scan: device %d  source  %-8s tp %5d",
                            Device->CardIndex(),
                            cSource::ToString(Channel->Source()).c_str(),
                            Channel->Transponder());
                        Device->Channel()->SwitchChannel(Channel);
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
        if (!scanList->Count() || !AnyDeviceSwitched)
        {
          delete scanList;
          scanList = NULL;
          if (lastActivity == 0) // this was a triggered scan
            Activity();
        }
//           XXX Channels.Unlock();
      }
      lastScan = time(NULL);
    }
  }
}
