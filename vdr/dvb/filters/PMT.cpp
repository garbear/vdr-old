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

#include "PMT.h"
#include "channels/Channel.h"
#include "Config.h"
#include "dvb/CADescriptor.h"
#include "settings/Settings.h"
#include "utils/CommonMacros.h"
#include "utils/I18N.h"
#include "utils/log/Log.h"

#include <algorithm>
#include <assert.h>
#include <libsi/si_ext.h>
#include <libsi/descriptor.h>
#include <libsi/section.h>
#include <sstream>

using namespace SI;
using namespace SI_EXT;
using namespace std;

#define MAX_AUDIO_LANGS    2
#define MAX_SUBTITLE_LANGS 2

namespace VDR
{

/*!
 * Helper function for logging purposes.
 */
const char* GetVtype(uint8_t vtype)
{
  switch (vtype)
  {
  case STREAMTYPE_11172_VIDEO:          return "STREAMTYPE_11172_VIDEO";
  case STREAMTYPE_13818_VIDEO:          return "STREAMTYPE_13818_VIDEO";
  case STREAMTYPE_14496_H264_VIDEO:     return "STREAMTYPE_14496_H264_VIDEO";
  case STREAMTYPE_13818_USR_PRIVATE_80: return "STREAMTYPE_13818_USR_PRIVATE_80";
  }

  static string err(StringUtils::Format("<Not a video type: 0x%02x!>"), vtype);
  return err.c_str();
}

void LogVtype(uint8_t oldVtype, uint8_t newVtype)
{
  esyslog("Section contains more than 1 video stream, replacing %s with %s",
      GetVtype(oldVtype), GetVtype(newVtype));
}

cPmt::cPmt(cDevice* device, uint16_t tid, uint16_t sid, uint16_t pid)
 : cFilter(device),
   m_tid(tid),
   m_sid(sid)
{
  OpenResource(pid, TableIdPMT);
}

ChannelPtr cPmt::GetChannel()
{
  ChannelPtr channel;

  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  if (GetSection(pid, data))
  {
    SI::PMT pmt(data.data(), false);
    if (pmt.CheckCRCAndParse())
    {
      if (m_sid == pmt.getServiceId())
        channel = CreateChannel(pmt);
      else
        esyslog("PMT service ID mismatch! Expected %u, found %d", m_sid, pmt.getServiceId());
    }
  }

  return channel;
}

ChannelPtr cPmt::CreateChannel(/* const */ SI::PMT& pmt) const // TODO: libsi fails at const-correctness
{
  // Create a new channel to return
  ChannelPtr channel = ChannelPtr(new cChannel);
  assert(GetCurrentlyTunedTransponder() != NULL); // TODO
  channel->CopyTransponderData(*GetCurrentlyTunedTransponder());

  SetIds(channel);
  SetStreams(channel, pmt);
  SetCaDescriptors(channel, pmt);

  // Log a comma-separated list of streams we found in the channel
  stringstream logStreams;
  logStreams << "PMT: Found channel with SID=" << m_sid << " and streams: " << channel->GetVideoStream().vpid << " (Video)";
  for (vector<AudioStream>::const_iterator it = channel->GetAudioStreams().begin(); it != channel->GetAudioStreams().end(); ++it)
    logStreams << ", " << it->apid << " (Audio)";
  for (vector<DataStream>::const_iterator it = channel->GetDataStreams().begin(); it != channel->GetDataStreams().end(); ++it)
    logStreams << ", " << it->dpid << " (Data)";
  for (vector<SubtitleStream>::const_iterator it = channel->GetSubtitleStreams().begin(); it != channel->GetSubtitleStreams().end(); ++it)
    logStreams << ", " << it->spid << " (Sub)";
  if (channel->GetTeletextStream().tpid != 0)
    logStreams << ", " << channel->GetTeletextStream().tpid << " (Teletext)";
  dsyslog("%s", logStreams.str().c_str());

  return channel;
}

void cPmt::SetIds(ChannelPtr channel) const
{
  const uint16_t nit = 0; // TODO: How to find the NIT of a new channel?
  channel->SetId(nit, m_tid, m_sid);
}

void cPmt::SetStreams(ChannelPtr channel, /* const */ SI::PMT& pmt) const // TODO: libsi fails at const-correctness
{
  VideoStream            videoStream = { };
  vector<AudioStream>    audioStreams;
  vector<DataStream>     dataStreams;
  vector<SubtitleStream> subtitleStreams;
  TeletextStream         teletextStream = { };

  SI::PMT::Stream stream;
  for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it); )
  {
    switch (stream.getStreamType())
    {
      case STREAMTYPE_11172_VIDEO:
      case STREAMTYPE_13818_VIDEO:
      case STREAMTYPE_14496_H264_VIDEO:
      {
        if (videoStream.vpid)
          LogVtype(videoStream.vtype, stream.getPid());

        videoStream.vpid  = stream.getPid();
        videoStream.vtype = stream.getStreamType();
        videoStream.ppid  = pmt.getPCRPid();
        break;
      }
      case STREAMTYPE_11172_AUDIO:
      case STREAMTYPE_13818_AUDIO:
      case STREAMTYPE_13818_AUDIO_ADTS:
      case STREAMTYPE_14496_AUDIO_LATM:
      {
        AudioStream as;
        as.apid = stream.getPid();
        as.atype = stream.getStreamType();

        SI::Descriptor* d;
        for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
        {
          switch (d->getDescriptorTag())
          {
            case SI::ISO639LanguageDescriptorTag:
            {
              SI::ISO639LanguageDescriptor* ld = (SI::ISO639LanguageDescriptor*)d;
              SI::ISO639LanguageDescriptor::Language l;
              unsigned int langCount = 0;
              for (SI::Loop::Iterator it; ld->languageLoop.getNext(l, it) && langCount < MAX_AUDIO_LANGS; langCount++)
              {
                // some use "---" to indicate "none"
                if (ld->languageCode[0] == '-')
                  continue;

                if (langCount > 0)
                  as.alang += '+';

                as.alang += I18nNormalizeLanguageCode(l.languageCode);
              }
              break;
            }
            default:
              break;
          }
          DELETENULL(d);
        }

        audioStreams.push_back(as);
        break;
      }
      case STREAMTYPE_13818_PRIVATE:
      case STREAMTYPE_13818_PES_PRIVATE:
      {
        DataStream ds = { };

        SI::Descriptor* d;
        for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
        {
          switch (d->getDescriptorTag())
          {
            case AC3DescriptorTag:
            case EnhancedAC3DescriptorTag:
            case DTSDescriptorTag:
            case AACDescriptorTag:
            {
              ds.dpid  = stream.getPid();
              ds.dtype = d->getDescriptorTag();
              break;
            }
            case SubtitlingDescriptorTag:
            {
              SubtitleStream ss = { };
              ss.spid = stream.getPid();

              SI::SubtitlingDescriptor* sd = (SI::SubtitlingDescriptor*)d;
              SI::SubtitlingDescriptor::Subtitling sub;
              unsigned int subCount = 0;
              for (SI::Loop::Iterator it; sd->subtitlingLoop.getNext(sub, it) && subCount < MAX_SUBTITLE_LANGS; subCount++)
              {
                if (sub.languageCode[0] != '\0')
                {
                  // TODO: Should these be overwritten if subCount >= 2?
                  ss.subtitlingType    = sub.getSubtitlingType();
                  ss.compositionPageId = sub.getCompositionPageId();
                  ss.ancillaryPageId   = sub.getAncillaryPageId();

                  if (subCount > 0)
                    ss.slang += "+";

                  ss.slang += I18nNormalizeLanguageCode(sub.languageCode);
                }
              }
              break;
            }
            case SI::TeletextDescriptorTag:
            {
              teletextStream.tpid = stream.getPid();
              break;
            }
            case SI::ISO639LanguageDescriptorTag:
            {
              SI::ISO639LanguageDescriptor* ld = (SI::ISO639LanguageDescriptor*)d;
              ds.dlang = I18nNormalizeLanguageCode(ld->languageCode);
              break;
            }
            default:
              break;
          }
          DELETENULL(d);
        }

        if (ds.dpid)
          dataStreams.push_back(ds);

        break;
      }
      case STREAMTYPE_13818_USR_PRIVATE_80:
        if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE) // DigiCipher II VIDEO (ANSI/SCTE 57)
        {
          if (videoStream.vpid)
            LogVtype(videoStream.vtype, stream.getPid());

          videoStream.vpid  = stream.getPid();
          videoStream.vtype = STREAMTYPE_13818_VIDEO; // compression based upon MPEG-2
          videoStream.ppid  = pmt.getPCRPid();
          break;
        }
        // no break
      case STREAMTYPE_13818_USR_PRIVATE_81:
      {
        if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE || // ATSC A/53 AUDIO (ANSI/SCTE 57)
            channel->IsAtsc())                                             // ATSC AC-3; kls && Alex Lasnier
        {
          DataStream ds;
          ds.dpid  = stream.getPid();
          ds.dtype = SI::AC3DescriptorTag;

          SI::Descriptor* d;
          for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
          {
            switch (d->getDescriptorTag())
            {
              case SI::ISO639LanguageDescriptorTag:
              {
                SI::ISO639LanguageDescriptor* ld = (SI::ISO639LanguageDescriptor*)d;
                ds.dlang = I18nNormalizeLanguageCode(ld->languageCode);
                break;
              }
              default:
                break;
            }
            DELETENULL(d);
          }

          dataStreams.push_back(ds);
          break;
        }
        // no break
      }
      case 0x82: // STREAMTYPE_USER_PRIVATE
      {
        if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE) // STANDARD SUBTITLE (ANSI/SCTE 27)
        {
          // TODO
          break;
        }
        // no break
      }
      case 0x83 ... 0xFF: // STREAMTYPE_USER_PRIVATE
      {
        DataStream ds;
        ds.dpid  = stream.getPid();
        ds.dtype = SI::AC3DescriptorTag;

        bool bIsAc3 = false;

        SI::Descriptor* d;
        for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
        {
          switch (d->getDescriptorTag())
          {
          case SI::RegistrationDescriptorTag:
          {
            SI::RegistrationDescriptor* rd = (SI::RegistrationDescriptor*)d;
            // http://www.smpte-ra.org/mpegreg/mpegreg.html
            switch (rd->getFormatIdentifier())
            {
            case 0x41432D33: // 'AC-3'
              bIsAc3 = true;
              break;
            default:
              //printf("Format identifier: 0x%08X (pid: %d)\n", rd->getFormatIdentifier(), stream.getPid());
              break;
            }
            break;
          }
          case SI::ISO639LanguageDescriptorTag:
          {
            SI::ISO639LanguageDescriptor* ld = (SI::ISO639LanguageDescriptor*)d;
            ds.dlang = I18nNormalizeLanguageCode(ld->languageCode);
            break;
          }
          default:
            break;
          }
          DELETENULL(d);
        }

        if (bIsAc3)
          dataStreams.push_back(ds);

        break;
      }
      default:
        break;
    }
  }

  channel->SetStreams(videoStream, audioStreams, dataStreams, subtitleStreams, teletextStream);
}

void cPmt::SetCaDescriptors(ChannelPtr channel, /* const */ SI::PMT& pmt) const // TODO: libsi fails at const-correctness
{
  CaDescriptorVector caDescriptors;

  // Scan the common loop
  SI::CaDescriptor* d;
  for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)pmt.commonDescriptors.getNext(it, SI::CaDescriptorTag)); )
  {
    vector<uint8_t> privateData(d->privateData.getData(), d->privateData.getData() + d->privateData.getLength());

    CaDescriptorPtr ca = CaDescriptorPtr(new cCaDescriptor(d->getCaType(), d->getCaPid(), 0, privateData));

    bool bExists = false;
    for (CaDescriptorVector::const_iterator it = caDescriptors.begin(); it != caDescriptors.end(); ++it)
    {
      if (**it == *ca)
      {
        bExists = true;
        break;
      }
    }

    if (!bExists)
      caDescriptors.push_back(ca);

    DELETENULL(d);
  }

  // Scan the stream-specific loops
  SI::PMT::Stream stream;
  for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it); )
  {
    for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)stream.streamDescriptors.getNext(it, SI::CaDescriptorTag)); )
    {
      vector<uint8_t> privateData(d->privateData.getData(), d->privateData.getData() + d->privateData.getLength());

      CaDescriptorPtr ca = CaDescriptorPtr(new cCaDescriptor(d->getCaType(), d->getCaPid(), stream.getPid(), privateData));

      bool bExists = false;
      for (CaDescriptorVector::const_iterator it = caDescriptors.begin(); it != caDescriptors.end(); ++it)
      {
        if (**it == *ca)
        {
          bExists = true;
          break;
        }
      }

      if (!bExists)
        caDescriptors.push_back(ca);

      DELETENULL(d);
    }
  }

  channel->SetCaDescriptors(caDescriptors);
}

}
