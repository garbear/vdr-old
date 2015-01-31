/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "PSIP_EIT.h"
#include "PSIP_STT.h"
#include "PAT.h"
#include "channels/ChannelManager.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "epg/ScheduleManager.h"
#include "utils/DateTime.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>
#include <vector>

using namespace SI;
using namespace SI_EXT;
using namespace std;

#define DESCRIPTION_SEPARATOR  ": "

// From UTF8Utils.h
#ifndef Utf8BufSize
#define Utf8BufSize(s)  ((s) * 4)
#endif

using namespace PLATFORM;

#define PSIP_EIT_MAX_OPEN_FILTER (10)

namespace VDR
{

cPsipEit::cPsipEit(cDevice* device)
 : cScanReceiver(device, "EIT (PSIP)")
{
}

void cPsipEit::OpenPid(uint16_t pid)
{
  CLockObject lock(m_mutex);
  std::map<uint16_t, PSIP_PID_STATE>::iterator it = m_eitPids.find(pid);
  if (it != m_eitPids.end())
  {
    it->second = PSIP_PID_STATE_OPEN;
    filter_properties eitFilter = { pid, TableIdEIT, 0xFF };
    AddFilter(eitFilter);
  }
}

size_t cPsipEit::OpenPids(void) const
{
  size_t retval(0);
  CLockObject lock(m_mutex);
  for (std::map<uint16_t, PSIP_PID_STATE>::const_iterator it; it != m_eitPids.end(); ++it)
    if (it->second == PSIP_PID_STATE_OPEN)
      ++retval;
  return retval;
}

void cPsipEit::AddPid(uint16_t pid)
{
  CLockObject lock(m_mutex);
  if (m_eitPids.find(pid) != m_eitPids.end())
    return;
  m_eitPids.insert(make_pair(pid, PSIP_PID_STATE_WAITING));

  if (OpenPids() < PSIP_EIT_MAX_OPEN_FILTER)
    OpenPid(pid);
}

void cPsipEit::PidScanned(uint16_t pid)
{
  filter_properties eitFilter = { pid, TableIdEIT, 0xFF };
  RemoveFilter(eitFilter);
  uint16_t nextPid(~0);
  size_t activePids(0);

  CLockObject lock(m_mutex);
  std::map<uint16_t, PSIP_PID_STATE>::iterator it = m_eitPids.find(pid);
  if (it != m_eitPids.end())
    it->second = PSIP_PID_STATE_DONE;

  for (std::map<uint16_t, PSIP_PID_STATE>::const_iterator it; it != m_eitPids.end(); ++it)
  {
    switch (it->second)
    {
      case PSIP_PID_STATE_OPEN:
        ++activePids;
        break;
      case PSIP_PID_STATE_WAITING:
        if (nextPid == ~0)
          nextPid = it->first;
        break;
      default:
        break;
    }
  }

  if (activePids < PSIP_EIT_MAX_OPEN_FILTER && nextPid != ~0)
    OpenPid(nextPid);
}

void cPsipEit::Detach(bool wait /* = false */)
{
  cScanReceiver::Detach(wait);
  CLockObject lock(m_mutex);
  m_eitPids.clear();
}

void cPsipEit::ReceivePacket(uint16_t pid, const uint8_t* data)
{
  // Get the/UTC offset for calculating event start times
  const unsigned int gpsUtcOffset = m_device->Scan()->GetGpsUtcOffset();
  unsigned int numEvents = 0;

  SI::PSIP_EIT psipEit(data);
  if (psipEit.CheckAndParse())
  {
    /** wait for the PMT scan to complete first */
    if (!m_device->Scan()->PAT()->PmtScanned())
      return;
    if (!Sync(pid, psipEit.getVersionNumber(), psipEit.getSectionNumber(), psipEit.getLastSectionNumber()))
      return;

    SI::PSIP_EIT::Event psipEitEvent;
    for (SI::Loop::Iterator it; psipEit.eventLoop.getNext(psipEitEvent, it); )
    {
      EventPtr thisEvent = EventPtr(new cEvent(psipEitEvent.getEventId()));

      // Convert start time fom GPS to POSIX time system
      CDateTime posixEpoch;
      posixEpoch.SetDate(1970, 1, 1); // 1 January 1970

      CDateTime gpsEpoch;
      gpsEpoch.SetDate(1980, 1, 6); // 6 January 1980

      CDateTime startTimeGps(psipEitEvent.getStartTime());

      CDateTime startTimePosix = startTimeGps + (gpsEpoch - posixEpoch) - CDateTimeSpan(0, 0, 0, gpsUtcOffset);
      thisEvent->SetStartTime(startTimePosix);
      thisEvent->SetEndTime(startTimePosix + CDateTimeSpan(0, 0, 0, psipEitEvent.getLengthInSeconds()));

      thisEvent->SetTableID(psipEit.getTableId());
      thisEvent->SetVersion(psipEit.getVersionNumber());

      vector<string> titleStrings;
      SI::PSIPString psipString;
      for (SI::Loop::Iterator it2; psipEitEvent.textLoop.getNext(psipString, it2); )
      {
        char buffer[Utf8BufSize(256)] = { }; // TODO: Size?
        psipString.getText(buffer, sizeof(buffer));
        titleStrings.push_back(buffer);
      }

      thisEvent->SetTitle(StringUtils::Join(titleStrings, "/"));

      thisEvent->SetAtscSourceID(psipEit.getSourceId());

      thisEvent->FixEpgBugs();

      cScheduleManager::Get().AddEvent(thisEvent, m_device->Channel()->GetCurrentlyTunedTransponder());
      numEvents++;
    }
  }

  if (numEvents > 0)
    dsyslog("EIT: Found PID %u with %u events", pid, numEvents);
  PidScanned(pid);
}

}
