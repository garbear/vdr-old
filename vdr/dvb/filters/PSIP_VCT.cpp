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

#include "PSIP_VCT.h"
#include "channels/Channel.h"
#include "sources/linux/DVBTransponderParams.h"
#include "utils/log/Log.h"

#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

namespace VDR
{

cPsipVct::cPsipVct(cDevice* device)
 : cFilter(device)
{
  OpenResource(PID_VCT, TableIdTVCT);
  OpenResource(PID_VCT, TableIdCVCT);
}

ChannelVector cPsipVct::GetChannels(void)
{
  ChannelVector channels;

  // TODO: Might be multiple sections, need section syncer

  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  if (GetSection(pid, data))
  {
    SI::PSIP_VCT vct(data.data(), false);
    if (vct.CheckCRCAndParse())
    {
      SI::PSIP_VCT::ChannelInfo channelInfo;
      for (SI::Loop::Iterator it; vct.channelInfoLoop.getNext(channelInfo, it); )
      {
        ChannelPtr channel = ChannelPtr(new cChannel);

        // Short name
        char buffer[22] = { };
        channelInfo.getShortName(buffer);
        channel->SetName(buffer, buffer, ""); // TODO: Parse descriptors for long name

        // Modulation mode
        cDvbTransponderParams params;
        unsigned int symbolRateHz = 0;
        ModulationMode modulation = (ModulationMode)channelInfo.getModulationMode();
        switch (modulation)
        {
        case ModulationModeSCTE_1:
          params.SetModulation(QAM_64);
          symbolRateHz = 5057000; // TODO
          break;
        case ModulationModeSCTE_2:
          params.SetModulation(QAM_256);
          symbolRateHz = 5361000; // TODO
          break;
        case ModulationModeATSC_VSB8:
          params.SetModulation(VSB_8);
          break;
        case ModulationModeATSC_VSB16:
          params.SetModulation(VSB_16);
          break;
        default:
          break;
        }
        const unsigned int frequencyHz = 0; // TODO
        channel->SetTransponderData(cSource::stAtsc, symbolRateHz, frequencyHz, params);

        // Transport Stream ID (TID) and program number / service ID (SID)
        // TODO: SID is 0xFFFF for analog channels
        channel->SetId(0, channelInfo.getTSID(), channelInfo.getServiceId());

        dsyslog("VCT: Found %s: %s (%d.%d, TSID=%u, SID=%u)",
            channelInfo.isHidden() ? "hidden channel" : "channel", channel->ShortName().c_str(),
            channelInfo.getMajorNumber(), channelInfo.getMinorNumber(),
            channel->GetTid(), channel->GetSid());

        if (channelInfo.isHidden())
          continue;

        channels.push_back(channel);
      }
    }
  }

  return channels;
}

}
