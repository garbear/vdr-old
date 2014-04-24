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

#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>
#include <sstream>

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

        dsyslog("Scanning PMT table for transport stream ID = %d, service ID = %d", tsPAT.getTransportStreamId(), assoc.getServiceId());
        cPmt pmt(GetDevice(), tsPAT.getTransportStreamId(), assoc.getServiceId(), assoc.getPid());

        ChannelPtr channel = pmt.GetChannel();
        if (channel)
        {
          // Log a comma-separated list of streams we found in the channel
          stringstream logStreams;
          logStreams << "Found channel with streams: " << channel->GetVideoStream().vpid << " (Video)";
          for (vector<AudioStream>::const_iterator it = channel->GetAudioStreams().begin(); it != channel->GetAudioStreams().end(); ++it)
            logStreams << ", " << it->apid << " (Audio)";
          for (vector<DataStream>::const_iterator it = channel->GetDataStreams().begin(); it != channel->GetDataStreams().end(); ++it)
            logStreams << ", " << it->dpid << " (Data)";
          for (vector<SubtitleStream>::const_iterator it = channel->GetSubtitleStreams().begin(); it != channel->GetSubtitleStreams().end(); ++it)
            logStreams << ", " << it->spid << " (Sub)";
          if (channel->GetTeletextStream().tpid != 0)
            logStreams << ", " << channel->GetTeletextStream().tpid << " (Teletext)";
          dsyslog("%s", logStreams.str().c_str());

          channels.push_back(channel);
        }
      }
    }
  }

  return channels;
}

}
