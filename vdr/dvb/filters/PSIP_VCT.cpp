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
#include "utils/log/Log.h"
#include "utils/UTF8Utils.h"

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

bool cPsipVct::ScanChannels(iFilterCallback* callback)
{
  bool bSuccess = false;

  // TODO: Might be multiple sections, need section syncer

  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  if (GetSection(pid, data))
  {
    bSuccess = true;

    SI::PSIP_VCT vct(data.data());
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

        Descriptor* dtor;
        SI::Loop::Iterator it2;
        if ((dtor = channelInfo.channelDescriptors.getNext(it2, PSIP_ExtendedChannelNameDescriptorTag)))
        {
          CharArray tmp = dtor->getData();
          std::vector<uint32_t> utf8Symbols;
          for (int i = 0; i < dtor->getLength(); ++i)
            utf8Symbols.push_back(tmp[i]);

          std::string strDecoded = cUtf8Utils::Utf8FromArray(utf8Symbols);
          isyslog("TEST: found name descriptor %s length %d", strDecoded.c_str(), dtor->getLength());
        }

        // Number and subnumber
        channel->SetNumber(channelInfo.getMajorNumber(), channelInfo.getMinorNumber());

        // Transponder and modulation mode
        cTransponder transponder(GetTransponder());

        ModulationMode modulation = (ModulationMode)channelInfo.getModulationMode();
        //unsigned int symbolRateHz = 0;
        switch (modulation)
        {
        case ModulationModeSCTE_1:
          transponder.SetModulation(QAM_64);
          //transponder.SetSymbolRate(5057000); // TODO: Why was symbol rate set for an ATSC transponder?
          break;
        case ModulationModeSCTE_2:
          transponder.SetModulation(QAM_256);
          //transponder.SetSymbolRate(5361000); // TODO: Why was symbol rate set for an ATSC transponder?
          break;
        case ModulationModeATSC_VSB8:
          transponder.SetModulation(VSB_8);
          break;
        case ModulationModeATSC_VSB16:
          transponder.SetModulation(VSB_16);
          break;
        default:
          break;
        }

        channel->SetTransponder(transponder);

        // Transport Stream ID (TID) and program number / service ID (SID)
        // TODO: SID is 0xFFFF for analog channels
        channel->SetId(0, channelInfo.getTSID(), channelInfo.getServiceId());
        channel->SetATSCSourceId(channelInfo.getSourceID());

        dsyslog("VCT: %s: %s (%d-%d, TSID=%u, SID=%u, source=%u)",
            channelInfo.isHidden() ? "Skipping hidden channel" : "Found channel",
            channel->ShortName().c_str(), channel->Number(), channel->SubNumber(),
            channel->ID().Tsid(), channel->ID().Sid(), channelInfo.getSourceID());

        if (channelInfo.isHidden())
          continue;

        callback->OnChannelNamesScanned(channel);
      }
    }
  }
  return bSuccess;
}

}
