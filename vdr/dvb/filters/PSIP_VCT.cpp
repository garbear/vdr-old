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
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
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

cPsipVct::cPsipVct(cDevice* device) :
    cScanReceiver(device, "VCT", PID_VCT)
{
}

void cPsipVct::Detach(void)
{
  m_sectionSyncer.Reset();
}

void cPsipVct::ReceivePacket(uint16_t pid, const uint8_t* data)
{
  SI::PSIP_VCT vct(data);
  if (vct.CheckCRCAndParse() && (vct.getTableId() == TableIdTVCT || vct.getTableId() == TableIdCVCT))
  {
    cSectionSyncer::SYNC_STATUS status = m_sectionSyncer.Sync(vct.getVersionNumber(), vct.getSectionNumber(), vct.getLastSectionNumber());
    if (status == cSectionSyncer::SYNC_STATUS_NOT_SYNCED || status == cSectionSyncer::SYNC_STATUS_OLD_VERSION)
      return;

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
      cTransponder transponder(m_device->Channel()->GetCurrentlyTunedTransponder());

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

      if (channelInfo.isHidden())
        continue;

      m_device->Scan()->OnChannelNamesScanned(channel);
    }
    SetScanned();
  }
}

}
