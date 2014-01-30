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

#define SCAN_TRANSPONDER_TIMEOUT_MS (20 * 1000)

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
  m_bScanned = false;
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

// --- cScanList -------------------------------------------------------------

void
cScanList::AddTransponders(const cTransponderList& Channels)
{
  Channels.AddTranspondersToScanList(*this);
}

void
cScanList::AddTransponders(const cChannelManager& channels)
{
  channels.AddTransponders(this);
}

void
cScanList::AddTransponder(ChannelPtr Channel)
{
  if (Channel->Source() && Channel->Transponder())
  {
    for (std::vector<cScanData>::const_iterator it = m_list.begin(); it != m_list.end(); ++it)
    {
      if ((*it).Source() == Channel->Source() &&
          ISTRANSPONDER((*it).Transponder(), Channel->Transponder()))
        return;
    }
    m_list.push_back(cScanData(Channel));
  }
}

cScanData* cScanList::Next(void)
{
  for (std::vector<cScanData>::iterator it = m_list.begin(); it != m_list.end(); ++it)
  {
    if (!(*it).Scanned())
      return &(*it);
  }

  return NULL;
}

size_t cScanList::TotalTransponders(void) const
{
  return m_list.size();
}

size_t cScanList::UnscannedTransponders(void) const
{
  size_t cnt(0);
  for (std::vector<cScanData>::const_iterator it = m_list.begin(); it != m_list.end(); ++it)
  {
    if (!(*it).Scanned())
      ++cnt;
  }

  return cnt;
}

// --- cTransponderList ------------------------------------------------------

void
cTransponderList::AddTransponder(ChannelPtr Channel)
{
  for (std::vector<ChannelPtr>::iterator it = m_channels.begin(); it != m_channels.end(); ++it)
  {
    if ((*it)->Source() == Channel->Source() && (*it)->Transponder() == Channel->Transponder())
      return;
  }
  m_channels.push_back(Channel);
}

void
cTransponderList::AddTranspondersToScanList(cScanList& scanlist) const
{
  for (std::vector<ChannelPtr>::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it)
    scanlist.AddTransponder(*it);
}

// --- cEITScanner -----------------------------------------------------------

cEITScanner EITScanner;

cEITScanner::cEITScanner(void) :
    m_nextTransponderScan(0),
    m_nextFullScan(0),
    m_scanList(NULL),
    m_transponderList(NULL),
    m_bScanFinished(false)
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
  m_nextTransponderScan.Init(0);
  m_nextFullScan.Init(0);
}

void cEITScanner::CreateScanList(void)
{
  if (!m_scanList)
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
}

bool cEITScanner::ScanDevice(DevicePtr device)
{
  assert(device);
  bool bSwitched(false);

  cScanData* ScanData = m_scanList->Next();
  if (ScanData)
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
      bSwitched = device->Channel()->SwitchChannel(Channel);
      ScanData->SetScanned();
    }
  }

  return bSwitched;
}

void cEITScanner::SaveEPGData(void)
{
  cSchedulesLock SchedulesLock(true);
  cSchedules *s = SchedulesLock.Get();
  if (s)
    s->Save();
}

bool cEITScanner::SwitchNextTransponder(void)
{
  for (DeviceVector::const_iterator it = cDeviceManager::Get().Iterator(); cDeviceManager::Get().IteratorHasNext(it); cDeviceManager::Get().IteratorNext(it))
  {
    if (*it && (*it)->Channel() && (*it)->Channel()->ProvidesEIT())
    {
      if (ScanDevice(*it))
        return true;
    }
  }
  return false;
}

void cEITScanner::Process(void)
{
  if (!m_nextTransponderScan.TimeLeft() && !m_nextFullScan.TimeLeft())
  {
    SaveEPGData();

    if (m_bScanFinished)
    {
      m_bScanFinished = false;
      bool bFailed = m_scanList->UnscannedTransponders() > 0;
      SAFE_DELETE(m_scanList);

      if (!bFailed)
      {
        cChannelManager::Get().Save();
        SaveEPGData();
      }

      m_nextFullScan.Init(g_setup.EPGScanTimeout * 1000 * 60);
      dsyslog("EIT scan %s, next scan in %d minutes", bFailed ? "failed" : "finished", g_setup.EPGScanTimeout);
      return;
    }

    CreateScanList();

    if (!SwitchNextTransponder())
      m_bScanFinished = true;
    else
      m_nextTransponderScan.Init(SCAN_TRANSPONDER_TIMEOUT_MS);
  }
}
