/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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
#include "utils/log/Log.h"

#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

namespace VDR
{

cPat::cPat(cDevice* device)
 : cFilter(device)
{
  OpenResource(PID_PAT, TableIdPAT);
}

ChannelVector cPat::GetChannels(void)
{
  ChannelVector channels;

  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  if (GetSection(pid, data))
  {
    SI::PAT tsPAT(data.data(), false);
    if (tsPAT.CheckCRCAndParse())
    {
      SI::PAT::Association assoc;
      for (SI::Loop::Iterator it; tsPAT.associationLoop.getNext(assoc, it); )
      {
        // TODO: Let's do something with the NIT PID
        if (assoc.isNITPid())
          continue;

        dsyslog("PAT: Scanning for PMT table with transport stream ID = %d, service ID = %d", tsPAT.getTransportStreamId(), assoc.getServiceId());
        cPmt pmt(GetDevice(), tsPAT.getTransportStreamId(), assoc.getServiceId(), assoc.getPid());

        ChannelPtr channel = pmt.GetChannel();
        if (channel)
          channels.push_back(channel);
      }
    }
  }

  return channels;
}

}
