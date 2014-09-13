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
#include "channels/ChannelManager.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
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

namespace VDR
{

cPsipEit::cPsipEit(cDevice* device, const vector<uint16_t>& pids)
 : cFilter(device)
{
  for (vector<uint16_t>::const_iterator itPid = pids.begin(); itPid != pids.end(); ++itPid)
    OpenResource(*itPid, TableIdEIT);
}

bool cPsipEit::ScanEvents(iFilterCallback* callback, unsigned int gpsUtcOffset)
{
  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  while (!GetResources().empty() && GetSection(pid, data))
  {
    // For logging purposes
    unsigned int numEvents = 0;

    SI::PSIP_EIT psipEit(data.data());
    if (psipEit.CheckCRCAndParse())
    {
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

        ChannelPtr channel = cChannelManager::Get().GetByFrequencyAndATSCSourceId(GetTransponder().FrequencyHz(), psipEit.getSourceId());
        if (channel)
          thisEvent->SetChannelID(channel->ID());
        else
          dsyslog("failed to find channel for event - freq=%u source=%u", GetTransponder().FrequencyHz(), psipEit.getSourceId());

        thisEvent->FixEpgBugs();

        callback->OnEventScanned(thisEvent);
        numEvents++;
      }
    }

    dsyslog("EIT: Found PID %u (%u bytes) with %u events", pid, data.size(), numEvents);

    CloseResource(pid);
  }

  // Scan was successful if all resources were encountered and closed
  return GetResources().empty();
}

}
