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
#include "devices/Device.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "utils/log/Log.h"

#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

namespace VDR
{

static const cScanReceiver::filter_properties mgt_pids[] =
{
  { PID_MGT, TableIdMGT, 0xFF },
};

cPsipMgt::cPsipMgt(cDevice* device) :
    cScanReceiver(device, "MGT", *mgt_pids)
{
}

void cPsipMgt::ReceivePacket(uint16_t pid, const uint8_t* data)
{
  SI::PSIP_MGT mgt(data);
  if (mgt.CheckAndParse())
  {
    size_t eitPids = 0;
    size_t mgtPids = 0;

    filter_properties filter = { pid, mgt.getTableId(), 0xFF };
    if (!Sync(filter, mgt.getVersionNumber(), mgt.getSectionNumber(), mgt.getLastSectionNumber()))
      return;

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
        //m_device->Scan()->PsipVCT()->AddPid(tablePid); // VCT already opens tablePid (0x1FFB)
        ++mgtPids;
        break;
      case 0x0002: // Cable VCT with current_next_indicator='1'
      case 0x0003: // Cable VCT with current_next_indicator='0'
        //m_device->Scan()->PsipVCT()->AddPid(tablePid); // VCT already opens tablePid (0x1FFB)
        ++mgtPids;
        break;
      case 0x0004: // Channel ETT
        //TODO filters.push_back(shared_ptr<cFilter>(new cChannelEtt(GetDevice(), tablePid)));
        break;
      case 0x0005: // DCCSCT
        //TODO filters.push_back(shared_ptr<cFilter>(new cDccst(GetDevice(), tablePid)));
        break;
      case 0x0100 ... 0x017F: // EIT-0 to EIT-127
      {
        //filters.push_back(shared_ptr<cFilter>(new cEitPsip(GetDevice(), tablePid, tableType - 0x0100)));
        const unsigned short eitNumber = tableType - 0x0100;
        m_device->Scan()->PsipEIT()->AddPid(tablePid);
        ++eitPids;
        break;
      }
      case 0x0200 ... 0x027F: // Event ETT-0 to event ETT-127
        //TODO filters.push_back(shared_ptr<cFilter>(new cEventEttPsip(GetDevice(), tablePid, tableType - 0x0200)));
        break;
      case 0x0301 ... 0x03FF: // RRT with rating_region 1-255
        //TODO filters.push_back(shared_ptr<cFilter>(new cRrt(GetDevice(), tablePid, tableType - 0x0300)));
        break;
      case 0x1400 ... 0x14FF: // DCCT with dcc_id 0x00-0xFF
        //TODO filters.push_back(shared_ptr<cFilter>(new cDcct(GetDevice(), tablePid, tableType - 0x1400)));
        break;
      }
    }

    dsyslog("MGT: Discovered %lu EIT tables and %lu MGT tables", eitPids, mgtPids);
    SetScanned();
  }
}

}
