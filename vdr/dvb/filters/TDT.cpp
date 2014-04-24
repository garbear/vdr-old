/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "TDT.h"
#include "settings/Settings.h"
#include "utils/CalendarUtils.h"
#include "utils/DateTime.h"

#include <errno.h>
#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

using namespace PLATFORM;
using namespace SI;
using namespace SI_EXT;
using namespace std;

#define MAX_TIME_DIFF   1 // number of seconds the local time may differ from dvb time before making any corrections
#define MAX_ADJ_DIFF   10 // number of seconds the local time may differ from dvb time to allow smooth adjustment
#define ADJ_DELTA     300 // number of seconds between calls for smooth time adjustment

namespace VDR
{

//CMutex    cTdt::m_mutex;
//CDateTime cTdt::m_lastAdj;

cTdt::cTdt(cDevice* device)
 : cFilter(device)
{
  OpenResource(PID_TDT, TableIdTDT);
}

time_t cTdt::GetTime(void)
{
  time_t t = 0;

  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  if (GetSection(pid, data))
  {
    SI::TDT tsTDT(data.data(), false);
    tsTDT.CheckParse();

    /*
    if (cSettings::Get().m_bSetSystemTime)
    {
#ifdef ADJUST_SYSTEM_TIME_FROM_DVB
      // XXX not adjusted to CDateTime yet
      time_t dvbtim = tsTDT.getTime();
      time_t loctim = time(NULL);

      int diff = dvbtim - loctim;
      if (std::abs(diff) > MAX_TIME_DIFF)
      {
        CLockObject lock(m_mutex);

        if (std::abs(diff) > MAX_ADJ_DIFF)
        {
          if (stime(&dvbtim) == 0)
            isyslog("System time changed from %s (%ld) to %s (%ld)", CalendarUtils::TimeToString(loctim).c_str(), loctim, CalendarUtils::TimeToString(dvbtim).c_str(), dvbtim);
          else
            esyslog("ERROR while setting system time: %s", strerror(errno));
        }
        else if ((CDateTime::GetUTCDateTime() - m_lastAdj).GetSecondsTotal() > ADJ_DELTA)
        {
          m_lastAdj = CDateTime::GetUTCDateTime();

          timeval delta;
          delta.tv_sec = diff;
          delta.tv_usec = 0;
          if (adjtime(&delta, NULL) == 0)
            isyslog("System time adjustment initiated from %s (%ld) to %s (%ld)", CalendarUtils::TimeToString(loctim).c_str(), loctim, CalendarUtils::TimeToString(dvbtim).c_str(), dvbtim);
          else
            esyslog("ERROR while adjusting system time: %s", strerror(errno));
        }
      }
#endif
    }
    */
  }

  return t;
}

}
