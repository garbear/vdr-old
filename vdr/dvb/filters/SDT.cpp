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

#include "SDT.h"
#include "channels/Channel.h"
#include "utils/CommonMacros.h"
#include "utils/StringUtils.h"
#include "utils/Tools.h"
#include "Types.h"

#include <assert.h>
#include <libsi/si_ext.h>
#include <libsi/descriptor.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

#ifndef Utf8BufSize
#define Utf8BufSize(s)  ((s) * 4)
#endif

namespace VDR
{

/*
ChannelPtr SdtGetByService(int source, int tid, int sid)
{
  // TODO: Need to lock m_newChannels
  for (ChannelVector::iterator channelIt = m_newChannels.begin(); channelIt != m_newChannels.end(); ++channelIt)
  {
    ChannelPtr& channel = *channelIt;
    if (channel->Source() == source && channel->Tid() == tid && channel->Sid() == sid)
      return channel;
  }
  return cChannel::EmptyChannel;
}

ChannelPtr SdtFoundService(ChannelPtr channel, int nid, int tid, int sid)
{
  assert((bool)channel);
  ChannelPtr newChannel = SdtGetByService(channel->Source(), tid, sid);
  if (newChannel)
  {
    newChannel->SetId(nid, tid, sid); // Update NID
    return newChannel;
  }

  // Before returning an empty channel ptr, add a new channel and transponder to their respective vectors
  newChannel = ChannelPtr(new cChannel);
  newChannel->CopyTransponderData(*channel);

  if (!IsKnownInitialTransponder(*newChannel, true))
  {
    ChannelPtr transponder = newChannel->Clone();
    m_transponders.push_back(transponder);
    dsyslog("   SDT: Add transponder");
  }

  newChannel->SetId(nid, tid, sid);
  dsyslog("   SDT: Add channel (NID = %d, TID = %d, SID = %d)", nid, tid, sid);
  m_newChannels.push_back(newChannel);

  return cChannel::EmptyChannel;
}
*/

cSdt::cSdt(cDevice* device, SI::TableId tableId /* = SI::TableIdSDT */)
 : cFilter(device),
   m_tableId(tableId)
{
  assert(m_tableId == TableIdSDT || m_tableId == TableIdSDT_other);

  //m_sectionSyncer.Reset();

  OpenResource(PID_SDT, m_tableId); // SDT
}

ChannelVector cSdt::GetChannels()
{
  ChannelVector  channels;
  cSectionSyncer syncSdt;

  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  while (GetSection(pid, data))
  {
    SI::SDT sdt(data.data(), false);
    if (sdt.CheckCRCAndParse())
    {
      cSectionSyncer::SYNC_STATUS status = syncSdt.Sync(sdt.getVersionNumber(), sdt.getSectionNumber(), sdt.getLastSectionNumber());
      if (status == cSectionSyncer::SYNC_STATUS_NOT_SYNCED)
        continue;
      if (status == cSectionSyncer::SYNC_STATUS_OLD_VERSION)
        break;

      assert(status == cSectionSyncer::SYNC_STATUS_NEW_VERSION);

      //HEXDUMP(data.data(), Length);

      SI::SDT::Service SiSdtService;
      for (SI::Loop::Iterator it; sdt.serviceLoop.getNext(SiSdtService, it);)
      {
        /*
        ChannelPtr channel = SdtFoundService(Channel(), sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
        if (!channel)
          continue;
        */
        ChannelPtr channel = ChannelPtr(new cChannel);
        channel->SetId(sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
        assert(GetCurrentlyTunedTransponder() != NULL); // TODO
        channel->CopyTransponderData(*GetCurrentlyTunedTransponder());

        SI::Descriptor * d;
        for (SI::Loop::Iterator it2; (d = SiSdtService.serviceDescriptors.getNext(it2));)
        {
          switch ((unsigned) d->getDescriptorTag())
          {
            case SI::ServiceDescriptorTag:
            {
              SI::ServiceDescriptor * sd = (SI::ServiceDescriptor *) d;
              switch (sd->getServiceType())
              {
                // ---television---
                case digital_television_service:
                case MPEG2_HD_digital_television_service:
                case advanced_codec_SD_digital_television_service:
                case advanced_codec_HD_digital_television_service:

                // ---radio---
                case digital_radio_sound_service:
                case advanced_codec_digital_radio_sound_service:

                // ---references---
                case digital_television_NVOD_reference_service:
                case advanced_codec_SD_NVOD_reference_service:
                case advanced_codec_HD_NVOD_reference_service:

                // ---time shifted services---
                case digital_television_NVOD_timeshifted_service:
                case advanced_codec_SD_NVOD_timeshifted_service:
                case advanced_codec_HD_NVOD_timeshifted_service:
                {
                  char NameBuf[Utf8BufSize(1024)];
                  char ShortNameBuf[Utf8BufSize(1024)];
                  char ProviderNameBuf[Utf8BufSize(1024)];
                  sd->serviceName.getText(NameBuf, ShortNameBuf, sizeof(NameBuf), sizeof(ShortNameBuf));
                  sd->providerName.getText(ProviderNameBuf, sizeof(ProviderNameBuf));

                  string strName = compactspace(NameBuf);
                  string strShortName = compactspace(ShortNameBuf);
                  string strProviderName = compactspace(ProviderNameBuf);

                  // Some cable providers don't mark short channel names according to the
                  // standard, but rather go their own way and use "name>short name" or
                  // "name, short name"
                  if (strShortName.empty() && cSource::IsCable(channel->Source()))
                  {
                    size_t pos = strName.find('>'); // fix for UPC Wien
                    if (pos == string::npos)
                      pos = strName.find(','); // fix for "Kabel Deutschland"

                    if (pos != string::npos)
                    {
                      strShortName = strName.substr(pos + 1);
                      strName = strName.substr(0, pos);
                    }
                  }

                  StringUtils::Trim(strName);
                  StringUtils::Trim(strProviderName);
                  StringUtils::Trim(strShortName);

                  channel->SetName(strName, strShortName, strProviderName);
                  channels.push_back(channel);
                }
                default:
                  break;
              }
            }
            break;
            case SI::NVODReferenceDescriptorTag:
            case SI::BouquetNameDescriptorTag:
            case SI::MultilingualServiceNameDescriptorTag:
            case SI::ServiceIdentifierDescriptorTag:
            case SI::ServiceAvailabilityDescriptorTag:
            case SI::DefaultAuthorityDescriptorTag:
            case SI::AnnouncementSupportDescriptorTag:
            case SI::DataBroadcastDescriptorTag:
            case SI::TelephoneDescriptorTag:
            case SI::CaIdentifierDescriptorTag:
            case SI::PrivateDataSpecifierDescriptorTag:
            case SI::ContentDescriptorTag:
            case 0x80 ... 0xFE: // user defined //
              break;
            default:
              dsyslog("SDT: unknown descriptor 0x%.2x", d->getDescriptorTag());
              break;
          }
          DELETENULL(d);
        }
      }
    }
  }

  return channels;
}

}
