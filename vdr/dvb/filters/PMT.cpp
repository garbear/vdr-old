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
#include "channels/ChannelManager.h"
#include "Config.h"
#include "devices/Device.h"
#include "devices/Remux.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceScanSubsystem.h"
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
  case STREAM_TYPE_11172_VIDEO:          return "STREAM_TYPE_11172_VIDEO";
  case STREAM_TYPE_13818_VIDEO:          return "STREAM_TYPE_13818_VIDEO";
  case STREAM_TYPE_14496_H264_VIDEO:     return "STREAM_TYPE_14496_H264_VIDEO";
  case STREAM_TYPE_13818_USR_PRIVATE_80: return "STREAM_TYPE_13818_USR_PRIVATE_80";
  }

  static string err(StringUtils::Format("<Not a video type: 0x%02x!>"), vtype);
  return err.c_str();
}

void LogVtype(uint8_t oldVtype, uint8_t newVtype)
{
  esyslog("Section contains more than 1 video stream, replacing %s with %s",
      GetVtype(oldVtype), GetVtype(newVtype));
}

cPmt::cPmt(cDevice* device)
 : cScanReceiver(device, "PMT")
{
}

bool cPmt::AddTransport(uint16_t tsid, uint16_t sid, uint16_t pid)
{
  PLATFORM::CLockObject lock(m_mutex);
  for (std::vector<PMTFilter>::const_iterator it = m_filters.begin(); it != m_filters.end(); ++it)
    if ((*it).pid == pid && (*it).tsid == tsid && (*it).sid == sid)
      return true;

  PMTFilter newfilter;
  newfilter.tsid = tsid;
  newfilter.sid  = sid;
  newfilter.pid  = pid;

  bool retval = true;
  ResetScanned();
  m_filters.push_back(newfilter);
  AddPid(pid);
  return retval;
}

bool cPmt::HasPid(uint16_t pid) const
{
  PLATFORM::CLockObject lock(m_mutex);
  for (std::vector<PMTFilter>::const_iterator it = m_filters.begin(); it != m_filters.end(); ++it)
    if ((*it).pid == pid)
      return true;
  return false;
}

void cPmt::ReceivePacket(uint16_t pid, const uint8_t* data)
{
  SI::PMT pmt(data);
  if (pmt.CheckCRCAndParse() && pmt.getTableId() == TableIdPMT)
  {
    for (std::vector<PMTFilter>::iterator it = m_filters.begin(); it != m_filters.end(); ++it)
    {
      if ((*it).pid == pid && (*it).sid == pmt.getServiceId())
      {
        m_device->Scan()->OnChannelPropsScanned(CreateChannel(pmt, (*it).tsid));
        m_filters.erase(it);
        RemovePid(pid);
        break;
      }
    }

    if (m_filters.empty())
      SetScanned();
  }
}

ChannelPtr cPmt::CreateChannel(/* const */ SI::PMT& pmt, uint16_t tsid) const // TODO: libsi fails at const-correctness
{
  ChannelPtr channel = ChannelPtr(new cChannel);

  channel->SetId(0, tsid, pmt.getServiceId()); // TODO: How to find the NIT of a new channel?
  SetStreams(channel, pmt);
  SetCaDescriptors(channel, pmt);
  channel->SetTransponder(m_device->Channel()->GetCurrentlyTunedTransponder());

  // Log a comma-separated list of streams we found in the channel
  stringstream logStreams;
  logStreams << "PMT: Found channel with SID=" << pmt.getServiceId() << " and streams: " << channel->GetVideoStream().vpid << " (Video)";
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

void cPmt::SetStreams(const ChannelPtr& channel, /* const */ SI::PMT& pmt) const // TODO: libsi fails at const-correctness
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
      case STREAM_TYPE_11172_VIDEO:
      case STREAM_TYPE_13818_VIDEO:
      case STREAM_TYPE_14496_H264_VIDEO:
      {
        if (videoStream.vpid)
          LogVtype(videoStream.vtype, stream.getPid());

        videoStream.vpid  = stream.getPid();
        videoStream.vtype = (STREAM_TYPE)stream.getStreamType();
        videoStream.ppid  = pmt.getPCRPid();
        break;
      }
      case STREAM_TYPE_11172_AUDIO:
      case STREAM_TYPE_13818_AUDIO:
      case STREAM_TYPE_13818_AUDIO_ADTS:
      case STREAM_TYPE_14496_AUDIO_LATM:
      {
        AudioStream as;
        as.apid = stream.getPid();
        as.atype = (STREAM_TYPE)stream.getStreamType();

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
      case STREAM_TYPE_13818_PRIVATE:
      case STREAM_TYPE_13818_PES_PRIVATE:
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
              ds.dtype = (STREAM_TYPE)d->getDescriptorTag(); // TODO: STREAM_TYPE OR DESCRIPTOR_TAG???
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
      case STREAM_TYPE_13818_USR_PRIVATE_80:
        if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE) // DigiCipher II VIDEO (ANSI/SCTE 57)
        {
          if (videoStream.vpid)
            LogVtype(videoStream.vtype, stream.getPid());

          videoStream.vpid  = stream.getPid();
          videoStream.vtype = STREAM_TYPE_13818_VIDEO; // compression based upon MPEG-2
          videoStream.ppid  = pmt.getPCRPid();
          break;
        }
        // no break
      case STREAM_TYPE_13818_USR_PRIVATE_81:
      {
        if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE || // ATSC A/53 AUDIO (ANSI/SCTE 57)
            channel->GetTransponder().Type() == TRANSPONDER_ATSC)          // ATSC AC-3; kls && Alex Lasnier
        {
          DataStream ds;
          ds.dpid  = stream.getPid();
          ds.dtype = (STREAM_TYPE)SI::AC3DescriptorTag; // TODO: STREAM_TYPE OR DESCRIPTOR_TAG???

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
      case STREAM_TYPE_13818_USR_PRIVATE_82:
      {
        if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE) // STANDARD SUBTITLE (ANSI/SCTE 27)
        {
          // TODO
          break;
        }
        // no break
      }
      case 0x83 ... 0xFF: // STREAM_TYPE_USER_PRIVATE
      {
        DataStream ds;
        ds.dpid  = stream.getPid();
        ds.dtype = (STREAM_TYPE)SI::AC3DescriptorTag; // TODO: STREAM_TYPE OR DESCRIPTOR_TAG???

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

void cPmt::SetCaDescriptors(const ChannelPtr& channel, /* const */ SI::PMT& pmt) const // TODO: libsi fails at const-correctness
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

void cPmt::Detach(void)
{
  cScanReceiver::Detach();
  m_filters.clear();
  RemovePids();
}

}
