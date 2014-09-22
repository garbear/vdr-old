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
#include "channels/Channel.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "utils/log/Log.h"

#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

namespace VDR
{

cPat::cPat(cDevice* device) :
    m_device(device),
    m_pmt(device),
    m_locked(false),
    m_scanned(false)
{
}

bool cPat::Attach(void)
{
  return m_device->Receiver()->AttachReceiver(this, PID_PAT);
}

bool cPat::WaitForScan(uint32_t iTimeout /* = TRANSPONDER_TIMEOUT */)
{
  PLATFORM::CLockObject lock(m_mutex);
  return m_scannedEvent.Wait(m_mutex, m_scanned, iTimeout) &&
      m_pmt.WaitForScan(iTimeout);
}

void cPat::Receive(const std::vector<uint8_t>& data)
{
  SI::PAT tsPAT(data.data());
  if (tsPAT.CheckCRCAndParse() && tsPAT.getTableId() == TableIdPAT)
  {
    SI::PAT::Association assoc;
    for (SI::Loop::Iterator it; m_locked && tsPAT.associationLoop.getNext(assoc, it); )
    {
      // TODO: Let's do something with the NIT PID
      if (assoc.isNITPid())
        continue;

      dsyslog("PAT: Scanning for PMT table with TSID=%d, SID=%d", tsPAT.getTransportStreamId(), assoc.getServiceId());
      m_pmt.AddTransport(tsPAT.getTransportStreamId(), assoc.getServiceId(), assoc.getPid());
    }

    PLATFORM::CLockObject lock(m_mutex);
    m_scanned = true;
    m_scannedEvent.Broadcast();
  }
}

void cPat::LockAcquired(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  m_locked  = true;
  m_scanned = false;
}

void cPat::LockLost(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  m_locked = false;
  m_scanned = false;
}

}
