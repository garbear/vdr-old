/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "SDTScanner.h"
#include "channels/Channel.h"

#include <assert.h>
#include <libsi/si_ext.h>
#include <libsi/descriptor.h>
#include <libsi/section.h>

using namespace SI_EXT;

namespace VDR
{

#ifndef Utf8BufSize
#define Utf8BufSize(s)  ((s) * 4)
#endif

cSdtScanner::cSdtScanner(iSdtScannerCallback* callback, bool bUseOtherTable /* = false */)
 : m_callback(callback),
   m_tableId(bUseOtherTable ? TABLE_ID_SDT_OTHER : TABLE_ID_SDT_ACTUAL)
{
  assert(m_callback);
  m_sectionSyncer.Reset();
  Set(PID_SDT, m_tableId, 0xFF); // SDT
}

cSdtScanner::~cSdtScanner()
{
  Del(PID_SDT, m_tableId);
}

void cSdtScanner::ProcessData(u_short Pid, u_char Tid, const u_char * Data, int Length)
{
  if (!(Source() && Transponder()))
    return;

  SI::SDT sdt(Data, false);
  if (!sdt.CheckCRCAndParse())
    return;

  if (!m_sectionSyncer.Sync(sdt.getVersionNumber(), sdt.getSectionNumber(), sdt.getLastSectionNumber()))
    return;

  dsyslog("   Transponder %d", Transponder());

  //HEXDUMP(Data, Length);

  SI::SDT::Service SiSdtService;
  for (SI::Loop::Iterator it; sdt.serviceLoop.getNext(SiSdtService, it);)
  {
    ChannelPtr channel = m_callback->SdtFoundService(Channel(), sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId());
    if (!channel)
      continue;

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
                            //---television---
            case digital_television_service:
            case MPEG2_HD_digital_television_service:
            case advanced_codec_SD_digital_television_service:
            case advanced_codec_HD_digital_television_service:
                            //---radio---
            case digital_radio_sound_service:
            case advanced_codec_digital_radio_sound_service:
                            //---references---
            case digital_television_NVOD_reference_service:
            case advanced_codec_SD_NVOD_reference_service:
            case advanced_codec_HD_NVOD_reference_service:
                            //---time shifted services---
            case digital_television_NVOD_timeshifted_service:
            case advanced_codec_SD_NVOD_timeshifted_service:
            case advanced_codec_HD_NVOD_timeshifted_service:
            {
              char  NameBuf[Utf8BufSize(1024)];
              char  ShortNameBuf[Utf8BufSize(1024)];
              char  ProviderNameBuf[Utf8BufSize(1024)];
              sd->serviceName.getText(NameBuf, ShortNameBuf, sizeof(NameBuf), sizeof(ShortNameBuf));
              char* pn = compactspace(NameBuf);
              char* ps = compactspace(ShortNameBuf);
              if (!*ps && cSource::IsCable(Source()))
              {
                  // Some cable providers don't mark short channel names according to the
                  // standard, but rather go their own way and use "name>short name" or
                  // "name, short name":
                  char* p = strchr(pn, '>'); // fix for UPC Wien
                  if (!p)
                    p = strchr(pn, ',');      // fix for "Kabel Deutschland"

                  if (p && p > pn)
                  {
                    *p++ = 0;
                    strcpy(ShortNameBuf, skipspace(p));
                  }
              }
              sd->providerName.getText(ProviderNameBuf, sizeof(ProviderNameBuf));
              char* pp = compactspace(ProviderNameBuf);

              // TODO: Need to lock channel somehow
              if (channel)
              {
                if (channel->Name() == CHANNEL_NAME_UNKNOWN)
                  channel->SetName(pn, ps, pp);
              }
              else
              {
                ChannelPtr newChannel = ChannelPtr(new cChannel);
                newChannel->CopyTransponderData(*Channel());
                m_callback->SdtFoundChannel(newChannel, sdt.getOriginalNetworkId(), sdt.getTransportStreamId(), SiSdtService.getServiceId(), pn, ps, pp);
              }
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
