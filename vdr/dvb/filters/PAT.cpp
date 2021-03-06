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


#include "PAT.h"
#include "PMT.h"
#include "SDT.h"
#include "channels/Channel.h"
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

static const cScanReceiver::filter_properties pat_pids[] =
{
  { PID_PAT, TableIdPAT, 0xFF },
};

cPat::cPat(cDevice* device) :
    cScanReceiver(device, "PAT", *pat_pids),
    m_pmt(device)
{
}

void cPat::LockLost(void)
{
  cScanReceiver::LockLost();
  m_pmt.Detach();
}

bool cPat::WaitForScan(uint32_t iTimeout /* = TRANSPONDER_TIMEOUT */)
{
  return cScanReceiver::WaitForScan(iTimeout) &&
      m_pmt.WaitForScan(iTimeout);
}

void cPat::ReceivePacket(uint16_t pid, const uint8_t* data)
{
  SI::PAT tsPAT(data);
  if (tsPAT.CheckAndParse() && tsPAT.getTableId() == TableIdPAT)
  {
    bool haspmt = false;
    filter_properties filter = { pid, tsPAT.getTableId(), 0xFF };
    if (!Sync(filter, tsPAT.getVersionNumber(), tsPAT.getSectionNumber(), tsPAT.getLastSectionNumber()))
      return;

    SI::PAT::Association assoc;
    for (SI::Loop::Iterator it; tsPAT.associationLoop.getNext(assoc, it); )
    {
      // TODO: Let's do something with the NIT PID
      if (assoc.isNITPid())
        continue;

      haspmt = true;
      dsyslog("PAT: Scanning for PMT table with TSID=%d, SID=%d", tsPAT.getTransportStreamId(), assoc.getServiceId());
      m_pmt.AddTransport(m_handle, tsPAT.getTransportStreamId(), assoc.getServiceId(), assoc.getPid());
    }

    if (Synced(pid))
    {
      filter_properties filter = { pid, TableIdPAT, 0xFF };
      FilterScanned(filter);
    }

    if (haspmt)
    {
      PLATFORM::CLockObject lock(m_mutex);
      if (m_handle)
        m_pmt.Attach(m_handle);
      SetScanned();
    }
  }
}

}
