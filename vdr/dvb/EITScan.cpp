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

cEITScanner::cEITScanner(void) :
    m_nextScan(0),
    m_scanList(NULL),
    m_transponderList(NULL)
{
}

cEITScanner::~cEITScanner()
{
  delete m_scanList;
  delete m_transponderList;
}

void cEITScanner::AddTransponder(ChannelPtr Channel)
{
  if (!m_transponderList)
    m_transponderList = new cTransponderList;
  m_transponderList->AddTransponder(Channel);
}

void cEITScanner::ForceScan(void)
{
  m_nextScan.Init(0);
}

void cEITScanner::CreateScanList(void)
{
  m_scanList = new cScanList;
  if (m_transponderList)
  {
    m_scanList->AddTransponders(*m_transponderList);
    delete m_transponderList;
    m_transponderList = NULL;
  }
  m_scanList->AddTransponders(cChannelManager::Get());
}

bool cEITScanner::ScanDevice(DevicePtr device)
{
  assert(device);
  bool bSwitched(false);

  for (cScanData *ScanData = m_scanList->First(); ScanData; ScanData = m_scanList->Next(ScanData))
  {
    ChannelPtr Channel = ScanData->GetChannel();
    if (Channel &&
        /** not encrypted or able to decrypt */
        (!Channel->Ca() || Channel->Ca() == device->CardIndex() || Channel->Ca() >= CA_ENCRYPTED_MIN) &&
        /** provided by this device */
        device->Channel()->ProvidesTransponder(*Channel) &&
        /** XXX priority? */
        device->Receiver()->Priority() < 0 &&
        /** allowed to switch */
        (device->Channel()->MaySwitchTransponder(*Channel) || device->Channel()->ProvidesTransponderExclusively(*Channel)))
    {
      dsyslog("EIT scan: device %d  source  %-8s tp %5d", device->CardIndex(), cSource::ToString(Channel->Source()).c_str(), Channel->Transponder());
      device->Channel()->SwitchChannel(Channel);
      m_scanList->Del(ScanData);
      bSwitched = true;
      break;
    }
  }

  return bSwitched;
}

void cEITScanner::Process(void)
{
  if (!m_nextScan.TimeLeft())
  {
    CreateScanList();
    bool AnyDeviceSwitched = false;

    for (DeviceVector::const_iterator it = cDeviceManager::Get().Iterator(); cDeviceManager::Get().IteratorHasNext(it); cDeviceManager::Get().IteratorNext(it))
    {
      if (*it && (*it)->Channel() && (*it)->Channel()->ProvidesEIT())
        AnyDeviceSwitched |= ScanDevice(*it);
    }

    if (!m_scanList->Count() || !AnyDeviceSwitched)
    {
      delete m_scanList;
      m_scanList = NULL;
    }
    m_nextScan.Init(ScanTimeout * 1000);
  }
}
