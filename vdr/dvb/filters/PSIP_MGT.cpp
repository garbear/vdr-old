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

#include "PSIP_MGT.h"
#include "PSIP_EIT.h"
#include "PSIP_STT.h"
#include "utils/log/Log.h"

#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

namespace VDR
{

cPsipMgt::cPsipMgt(cDevice* device)
 : cFilter(device)
{
  OpenResource(PID_MGT, TableIdMGT);
}

bool cPsipMgt::GetPSIPData(EventVector& events)
{
  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data

  if (GetSection(pid, data))
  {
    SI::PSIP_MGT mgt(data.data(), false);
    if (mgt.CheckCRCAndParse())
    {
      vector<uint16_t> eitPids; // Packet IDs of discovered EIT tables

      SI::PSIP_MGT::TableInfo tableInfo;
      for (SI::Loop::Iterator it; mgt.tableInfoLoop.getNext(tableInfo, it);)
      {
        const uint16_t tableType = tableInfo.getTableType();
        const uint16_t tablePid = tableInfo.getPid();

        // Source: ATSC Standard A/65:2009 Table 6.3
        switch (tableType)
        {
        case 0x0000: // Terrestrial VCT with current_next_indicator='1'
        case 0x0001: // Terrestrial VCT with current_next_indicator='0'
          //filters.push_back(shared_ptr<cFilter>(new cVct(tablePid, true)));
          break;
        case 0x0002: // Cable VCT with current_next_indicator='1'
        case 0x0003: // Cable VCT with current_next_indicator='0'
          //filters.push_back(shared_ptr<cFilter>(new cVct(GetDevice(), tablePid, false)));
          break;
        case 0x0004: // Channel ETT
          //filters.push_back(shared_ptr<cFilter>(new cChannelEtt(GetDevice(), tablePid)));
          break;
        case 0x0005: // DCCSCT
          //filters.push_back(shared_ptr<cFilter>(new cDccst(GetDevice(), tablePid)));
          break;
        case 0x0100 ... 0x017F: // EIT-0 to EIT-127
        {
          //filters.push_back(shared_ptr<cFilter>(new cEitPsip(GetDevice(), tablePid, tableType - 0x0100)));
          const unsigned short eitNumber = tableType - 0x0100;
          dsyslog("MGT: Discovered EIT%d, pid=%u", eitNumber, tablePid);
          eitPids.push_back(tablePid);
          break;
        }
        case 0x0200 ... 0x027F: // Event ETT-0 to event ETT-127
          //filters.push_back(shared_ptr<cFilter>(new cEventEttPsip(GetDevice(), tablePid, tableType - 0x0200)));
          break;
        case 0x0301 ... 0x03FF: // RRT with rating_region 1-255
          //filters.push_back(shared_ptr<cFilter>(new cRrt(GetDevice(), tablePid, tableType - 0x0300)));
          break;
        case 0x1400 ... 0x14FF: // DCCT with dcc_id 0x00-0xFF
          //filters.push_back(shared_ptr<cFilter>(new cDcct(GetDevice(), tablePid, tableType - 0x1400)));
          break;
        }
      }

      // Get the/UTC offset for calculating event start times
      cPsipStt psipStt(GetDevice());
      const unsigned int gpsUtcOffset = psipStt.GetGpsUtcOffset();
      if (gpsUtcOffset == 0)
        esyslog("Error: unable to get GPS/UTC offset, assuming 0 seconds");

      cPsipEit psipEit(GetDevice(), eitPids, gpsUtcOffset);
      events = psipEit.GetEvents();

      return true;
    }
  }

  return false;
}

}
