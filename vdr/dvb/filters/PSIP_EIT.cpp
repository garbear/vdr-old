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

void cPsipEit::AddPid(TunerHandlePtr handle, uint16_t pid)
{
  filter_properties eitFilter = { pid, TableIdEIT, 0xFF };
  PLATFORM::CLockObject lock(m_mutex);
  m_handle = handle;
  if (m_handle)
    AddFilter(eitFilter);
}

void cPsipEit::ReceivePacket(uint16_t pid, const uint8_t* data)
{
  // Get the/UTC offset for calculating event start times
  const unsigned int gpsUtcOffset = m_device->Scan()->GetGpsUtcOffset();
  unsigned int numEvents = 0;

  /** wait for the PMT scan to complete first */
  if (!m_device->Scan()->PAT()->PmtScanned())
    return;

  SI::PSIP_EIT psipEit(data);
  if (psipEit.CheckAndParse())
  {
    filter_properties filter = { pid, psipEit.getTableId(), 0xFF };
    if (!Sync(filter, psipEit.getVersionNumber(), psipEit.getSectionNumber(), psipEit.getLastSectionNumber()))
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

    if (Synced(pid))
    {
      filter_properties eitFilter = { pid, TableIdEIT, 0xFF };
      FilterScanned(eitFilter);
    }
  }

  if (numEvents > 0)
    dsyslog("EIT: Found PID %u with %u events", pid, numEvents);
  if (NbOpenPids() == 0)
    SetScanned();
}

}
