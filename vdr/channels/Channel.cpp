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

#include "Channel.h"
#include "ChannelDefinitions.h"
#include "dvb/CADescriptor.h"
#include "epg/Event.h"
#include "epg/Schedule.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/CRC32.h"
#include "utils/log/Log.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <tinyxml.h>

using namespace std;

namespace VDR
{

bool operator==(const VideoStream& lhs, const VideoStream& rhs)
{
  return lhs.vpid  == rhs.vpid  &&
         lhs.vtype == rhs.vtype &&
         lhs.ppid  == rhs.ppid;
}

bool operator==(const AudioStream& lhs, const AudioStream& rhs)
{
  return lhs.apid  == rhs.apid  &&
         lhs.atype == rhs.atype &&
         lhs.alang == rhs.alang;
}

bool operator==(const DataStream& lhs, const DataStream& rhs)
{
  return lhs.dpid  == rhs.dpid  &&
         lhs.dtype == rhs.dtype &&
         lhs.dlang == rhs.dlang;
}

bool operator==(const SubtitleStream& lhs, const SubtitleStream& rhs)
{
  return lhs.spid  == rhs.spid &&
         lhs.slang == rhs.slang;
}

bool operator==(const TeletextStream& lhs, const TeletextStream& rhs)
{
  return lhs.tpid == rhs.tpid;
}

bool operator!=(const VideoStream&    lhs, const VideoStream& rhs)    { return !(lhs == rhs); }
bool operator!=(const AudioStream&    lhs, const AudioStream& rhs)    { return !(lhs == rhs); }
bool operator!=(const DataStream&     lhs, const DataStream& rhs)     { return !(lhs == rhs); }
bool operator!=(const SubtitleStream& lhs, const SubtitleStream& rhs) { return !(lhs == rhs); }
bool operator!=(const TeletextStream& lhs, const TeletextStream& rhs) { return !(lhs == rhs); }

bool ISTRANSPONDER(int frequencyMHz1, int frequencyMHz2)
{
  return std::abs(frequencyMHz1 - frequencyMHz2) < 4;
}

// --- cChannel ----------------------------------------------------------------

const ChannelPtr cChannel::EmptyChannel;

// --- Construct/deconstruct channel object ------------------------------------

cChannel::cChannel(void)
 : m_number(0),
   m_subNumber(0)
{
}

ChannelPtr cChannel::Clone(void) const
{
  ChannelPtr newChannel(new cChannel());
  *newChannel = *this;
  return newChannel;
}

cChannel& cChannel::operator=(const cChannel& rhs)
{
  if (this != &rhs)
  {
    SetId(rhs.m_channelId);
    SetName(rhs.m_name, rhs.m_shortName, rhs.m_provider);
    SetPortalName(rhs.m_portalName);
    SetNumber(rhs.m_number, rhs.m_subNumber);
    SetStreams(rhs.m_videoStream, rhs.m_audioStreams, rhs.m_dataStreams, rhs.m_subtitleStreams, rhs.m_teletextStream);
    SetCaDescriptors(rhs.m_caDescriptors);
    SetTransponder(rhs.m_transponder);
  }
  return *this;
}

// --- identifiers -------------------------------------------------------------

void cChannel::SetId(const cChannelID& newId)
{
  if (m_channelId != newId)
  {
    m_channelId = newId;
    SetChanged();
  }
}

void cChannel::SetId(uint16_t nid          /* = 0 */,
                     uint16_t tsid         /* = 0 */,
                     uint16_t sid          /* = 0 */,
                     int32_t  atscSourceId /* = ATSC_SOURCE_ID_NONE */)
{
  SetId(cChannelID(nid, tsid, sid, atscSourceId));
}

void cChannel::SetATSCSourceId(int32_t atscSourceId)
{
  if (m_channelId.ATSCSourceId() != atscSourceId)
  {
    m_channelId.SetAtscSourceID(atscSourceId);
    SetChanged();
  }
}

// --- properties -----------------------------------------------------

void cChannel::SetName(const string& strName, const string& strShortName, const string& strProvider)
{
  if (m_name != strName || m_shortName != strShortName || m_provider != strProvider)
  {
    m_name      = strName;
    m_shortName = strShortName;
    m_provider  = strProvider;
    isyslog("updating channel name of channel %d: %s%s", Sid(), strName.c_str(), strShortName != strName ? StringUtils::Format(" (%s)", strShortName.c_str()).c_str() : "");
    SetChanged();
  }
}

void cChannel::SetPortalName(const string& strPortalName)
{
  if (m_portalName != strPortalName)
  {
    m_portalName = strPortalName;
    SetChanged();
  }
}

void cChannel::SetNumber(unsigned int number, unsigned int subNumber /* = 0 */)
{
  if (m_number != number || m_subNumber != subNumber)
  {
    m_number    = number;
    m_subNumber = subNumber;
    SetChanged();
  }
}

// --- streams -----------------------------------------------------------------

const AudioStream& cChannel::GetAudioStream(unsigned int index) const
{
  static AudioStream empty = { };
  return index < m_audioStreams.size() ? m_audioStreams[index] : empty;
}

const DataStream& cChannel::GetDataStream(unsigned int index) const
{
  static DataStream empty = { };
  return index < m_dataStreams.size() ? m_dataStreams[index] : empty;
}

const SubtitleStream& cChannel::GetSubtitleStream(unsigned int index) const
{
  static SubtitleStream empty = { };
  return index < m_subtitleStreams.size() ? m_subtitleStreams[index] : empty;
}

uint16_t cChannel::GetCaId(unsigned int index) const
{
  return index < m_caDescriptors.size() ? m_caDescriptors[index]->CaSystem() : 0;
}

vector<uint16_t> cChannel::GetCaIds() const
{
  vector<uint16_t> caIds;
  caIds.reserve(m_caDescriptors.size());

  for (vector<CaDescriptorPtr>::const_iterator it = m_caDescriptors.begin(); it != m_caDescriptors.end(); ++it)
    caIds.push_back((*it)->CaSystem());

  return caIds;
}

set<uint16_t> cChannel::GetPids(void) const
{
  set<uint16_t> pids;
  if (GetVideoStream().vpid)
    pids.insert(GetVideoStream().vpid);
  if (GetVideoStream().ppid != GetVideoStream().vpid)
    pids.insert(GetVideoStream().ppid);
  for (vector<AudioStream>::const_iterator it = GetAudioStreams().begin(); it != GetAudioStreams().end(); ++it)
    pids.insert(it->apid);
  for (vector<DataStream>::const_iterator it = GetDataStreams().begin(); it != GetDataStreams().end(); ++it)
    pids.insert(it->dpid);
  for (vector<SubtitleStream>::const_iterator it = GetSubtitleStreams().begin(); it != GetSubtitleStreams().end(); ++it)
    pids.insert(it->spid);
  if (GetTeletextStream().tpid)
    pids.insert(GetTeletextStream().tpid);
  return pids;
}

bool cChannel::SetStreams(const VideoStream& videoStream,
                          const vector<AudioStream>& audioStreams,
                          const vector<DataStream>& dataStreams,
                          const vector<SubtitleStream>& subtitleStreams,
                          const TeletextStream& teletextStream)
{
  if (m_videoStream     != videoStream ||
      m_audioStreams    != audioStreams ||
      m_dataStreams     != dataStreams ||
      m_subtitleStreams != subtitleStreams ||
      m_teletextStream  != teletextStream)
  {
    m_videoStream     = videoStream;
    m_audioStreams    = audioStreams;;
    m_dataStreams     = dataStreams;
    m_subtitleStreams = subtitleStreams;
    m_teletextStream  = teletextStream;
    SetChanged();
    return true;
  }
  return false;
}

void cChannel::SetSubtitlingDescriptors(const vector<SubtitleStream>& subtitleStreams)
{
  // TODO: From inspection, it appears that the list of subtitle streams should
  // be obtained with GetSubtitleStreams(), the subtitlingType, compositionPageId
  // and ancillaryPageId fields should be filled in, then the completed array
  // handed to SetSubtitlingDescriptors().
  if (m_subtitleStreams != subtitleStreams)
  {
    m_subtitleStreams = subtitleStreams;
    SetChanged();
  }
}

bool cChannel::SetCaDescriptors(const CaDescriptorVector& caDescriptors)
{
  if (m_caDescriptors != caDescriptors)
  {
    m_caDescriptors = caDescriptors;
    SetChanged();
    return true;
  }
  return false;
}

// --- transponder -------------------------------------------------------------

void cChannel::SetTransponder(const cTransponder& transponder)
{
  if (m_transponder != transponder)
  {
    m_transponder = transponder;
    SetChanged();
  }
}

bool cChannel::Serialise(TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement* channelElement = node->ToElement();
  if (channelElement == NULL)
    return false;

  if (!m_channelId.Serialise(node))
    return false;

  TiXmlElement transponderElement(CHANNEL_XML_ELM_TRANSPONDER);
  if (!m_transponder.Serialise(channelElement->InsertEndChild(transponderElement)))
    return false;

  channelElement->SetAttribute(CHANNEL_XML_ATTR_NAME,      m_name);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SHORTNAME, m_shortName);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_PROVIDER,  m_provider);
  // m_portalName
  channelElement->SetAttribute(CHANNEL_XML_ATTR_NUMBER,    m_number);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SUBNUMBER, m_subNumber);

  if (m_videoStream.vpid != 0)
  {
    TiXmlElement vpidElement(CHANNEL_XML_ELM_VPID);
    TiXmlNode *vpidNode = channelElement->InsertEndChild(vpidElement);
    if (vpidNode)
    {
      TiXmlElement *vpidElem = vpidNode->ToElement();
      if (vpidElem)
      {
        TiXmlText *vpidText = new TiXmlText(StringUtils::Format("%d", m_videoStream.vpid));
        if (vpidText)
        {
          vpidElem->LinkEndChild(vpidText);
          vpidElem->SetAttribute(CHANNEL_XML_ATTR_PPID, m_videoStream.ppid);
          vpidElem->SetAttribute(CHANNEL_XML_ATTR_VTYPE, m_videoStream.vtype);
        }
      }
    }
  }

  if (!m_audioStreams.empty())
  {
    TiXmlElement apidsElement(CHANNEL_XML_ELM_APIDS);
    TiXmlNode *apidsNode = channelElement->InsertEndChild(apidsElement);
    if (apidsNode)
    {
      for (vector<AudioStream>::const_iterator it = m_audioStreams.begin(); it != m_audioStreams.end(); ++it)
      {
        const AudioStream& audioStream = *it;
        TiXmlElement apidElement(CHANNEL_XML_ELM_APID);
        TiXmlNode *apidNode = apidsNode->InsertEndChild(apidElement);
        if (apidNode)
        {
          TiXmlElement *apidElem = apidNode->ToElement();
          if (apidElem)
          {
            TiXmlText *apidText = new TiXmlText(StringUtils::Format("%d", audioStream.apid));
            if (apidText)
            {
              apidElem->LinkEndChild(apidText);
              apidElem->SetAttribute(CHANNEL_XML_ATTR_ALANG, audioStream.alang);
              apidElem->SetAttribute(CHANNEL_XML_ATTR_ATYPE, audioStream.atype);
            }
          }
        }
      }
    }
  }

  if (!m_dataStreams.empty())
  {
    TiXmlElement dpidsElement(CHANNEL_XML_ELM_DPIDS);
    TiXmlNode *dpidsNode = channelElement->InsertEndChild(dpidsElement);
    if (dpidsNode)
    {
      for (vector<DataStream>::const_iterator it = m_dataStreams.begin(); it != m_dataStreams.end(); ++it)
      {
        const DataStream& dataStream = *it;
        TiXmlElement dpidElement(CHANNEL_XML_ELM_DPID);
        TiXmlNode *dpidNode = dpidsNode->InsertEndChild(dpidElement);
        if (dpidNode)
        {
          TiXmlElement *dpidElem = dpidNode->ToElement();
          if (dpidElem)
          {
            TiXmlText *dpidText = new TiXmlText(StringUtils::Format("%d", dataStream.dpid));
            if (dpidText)
            {
              dpidElem->LinkEndChild(dpidText);
              dpidElem->SetAttribute(CHANNEL_XML_ATTR_DLANG, dataStream.dlang);
              dpidElem->SetAttribute(CHANNEL_XML_ATTR_DTYPE, dataStream.dtype);
            }
          }
        }
      }
    }
  }

  if (!m_subtitleStreams.empty())
  {
    TiXmlElement spidsElement(CHANNEL_XML_ELM_SPIDS);
    TiXmlNode *spidsNode = channelElement->InsertEndChild(spidsElement);
    if (spidsNode)
    {
      for (vector<SubtitleStream>::const_iterator it = m_subtitleStreams.begin(); it != m_subtitleStreams.end(); ++it)
      {
        const SubtitleStream& subtitleStream = *it;
        TiXmlElement spidElement(CHANNEL_XML_ELM_SPID);
        TiXmlNode *spidNode = spidsNode->InsertEndChild(spidElement);
        if (spidNode)
        {
          TiXmlElement *spidElem = spidNode->ToElement();
          if (spidElem)
          {
            TiXmlText *spidText = new TiXmlText(StringUtils::Format("%d", subtitleStream.spid));
            if (spidText)
            {
              spidElem->LinkEndChild(spidText);
              spidElem->SetAttribute(CHANNEL_XML_ATTR_SLANG, subtitleStream.slang);
              // TODO: Should we also store subtitlingType, compositionPageId and ancillaryPageId?
            }
          }
        }
      }
    }
  }

  if (m_teletextStream.tpid != 0)
  {
    TiXmlElement tpidElement(CHANNEL_XML_ELM_TPID);
    TiXmlNode *tpidNode = channelElement->InsertEndChild(tpidElement);
    if (tpidNode)
    {
      TiXmlElement *tpidElem = tpidNode->ToElement();
      if (tpidElem)
      {
        TiXmlText *tpidText = new TiXmlText(StringUtils::Format("%d", m_teletextStream.tpid));
        if (tpidText)
          tpidElem->LinkEndChild(tpidText);
      }
    }
  }

  if (!m_caDescriptors.empty())
  {
    TiXmlElement caidsElement(CHANNEL_XML_ELM_CAIDS);
    TiXmlNode *caidsNode = channelElement->InsertEndChild(caidsElement);
    if (caidsNode)
    {
      for (vector<CaDescriptorPtr>::const_iterator it = m_caDescriptors.begin(); it != m_caDescriptors.end(); ++it)
      {
        uint16_t caId = (*it)->CaSystem();
        TiXmlElement caidElement(CHANNEL_XML_ELM_CAID);
        TiXmlNode *caidNode = caidsNode->InsertEndChild(caidElement);
        if (caidNode)
        {
          TiXmlElement *caidElem = caidNode->ToElement();
          if (caidElem)
          {
            TiXmlText *caidText = new TiXmlText(StringUtils::Format("%u", caId));
            if (caidText)
              caidElem->LinkEndChild(caidText);
          }
        }
      }
    }
  }

  return true;
}

bool cChannel::CanBePlayed(void) const
{
  /** just filter out scrambled channels for now */
  return GetCaIds().empty() &&
      (IsVideoChannel() || IsRadioChannel());
}

bool cChannel::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  if (!m_channelId.Deserialise(node))
    return false;

  const TiXmlElement* elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (!m_transponder.Deserialise(elem->FirstChild(CHANNEL_XML_ELM_TRANSPONDER)))
    return false;

  const char* name = elem->Attribute(CHANNEL_XML_ATTR_NAME);
  m_name = name ? name : "";

  const char* shortName = elem->Attribute(CHANNEL_XML_ATTR_SHORTNAME);
  m_shortName = shortName ? shortName : "";

  const char* provider = elem->Attribute(CHANNEL_XML_ATTR_PROVIDER);
  m_provider = provider ? provider : "";

  const char* portalName = elem->Attribute(CHANNEL_XML_ATTR_PROVIDER);
  m_portalName = portalName ? portalName : "";

  const char* number = elem->Attribute(CHANNEL_XML_ATTR_NUMBER);
  m_number = number ? StringUtils::IntVal(number) : 0;

  const char* subNumber = elem->Attribute(CHANNEL_XML_ATTR_SUBNUMBER);
  m_subNumber = subNumber ? StringUtils::IntVal(subNumber) : 0;

  const TiXmlNode *vpidNode = elem->FirstChild(CHANNEL_XML_ELM_VPID);
  if (vpidNode)
  {
    const TiXmlElement *vpidElem = vpidNode->ToElement();
    if (vpidElem != NULL)
    {
      VideoStream vs = { };

      vs.vpid = StringUtils::IntVal(vpidElem->GetText());

      const char *ppid = vpidElem->Attribute(CHANNEL_XML_ATTR_PPID);
      if (ppid != NULL)
        vs.ppid = StringUtils::IntVal(ppid);

      const char *vtype = vpidElem->Attribute(CHANNEL_XML_ATTR_VTYPE);
      if (vtype != NULL)
        vs.vtype = (STREAM_TYPE)StringUtils::IntVal(vtype);

      m_videoStream = vs;
    }
  }

  const TiXmlNode *apidsNode = elem->FirstChild(CHANNEL_XML_ELM_APIDS);
  if (apidsNode)
  {
    const TiXmlNode *apidNode = apidsNode->FirstChild(CHANNEL_XML_ELM_APID);
    for (unsigned int i = 0; apidNode; i++, apidNode = apidNode->NextSibling(CHANNEL_XML_ELM_APID))
    {
      const TiXmlElement *apidElem = apidNode->ToElement();
      if (apidElem != NULL)
      {
        AudioStream as = { };

        as.apid = StringUtils::IntVal(apidElem->GetText());

        const char *alang = apidElem->Attribute(CHANNEL_XML_ATTR_ALANG);
        if (alang != NULL)
          as.alang = alang;

        const char *atype = apidElem->Attribute(CHANNEL_XML_ATTR_ATYPE);
        if (atype != NULL)
          as.atype = (STREAM_TYPE)StringUtils::IntVal(atype);

        m_audioStreams.push_back(as);
      }
    }
  }

  const TiXmlNode *dpidsNode = elem->FirstChild(CHANNEL_XML_ELM_DPIDS);
  if (dpidsNode)
  {
    const TiXmlNode *dpidNode = dpidsNode->FirstChild(CHANNEL_XML_ELM_DPID);
    for (unsigned int i = 0; dpidNode; i++, dpidNode = dpidNode->NextSibling(CHANNEL_XML_ELM_DPID))
    {
      const TiXmlElement *dpidElem = dpidNode->ToElement();
      if (dpidElem != NULL)
      {
        DataStream ds = { };

        ds.dpid = StringUtils::IntVal(dpidElem->GetText());

        const char *dlang = dpidElem->Attribute(CHANNEL_XML_ATTR_DLANG);
        if (dlang != NULL)
          ds.dlang = dlang;

        const char *dtype = dpidElem->Attribute(CHANNEL_XML_ATTR_DTYPE);
        if (dtype != NULL)
          ds.dtype = (STREAM_TYPE)StringUtils::IntVal(dtype);

        m_dataStreams.push_back(ds);
      }
    }
  }

  const TiXmlNode *spidsNode = elem->FirstChild(CHANNEL_XML_ELM_SPIDS);
  if (spidsNode)
  {
    const TiXmlNode *spidNode = spidsNode->FirstChild(CHANNEL_XML_ELM_SPID);
    for (unsigned int i = 0; spidNode; i++, spidNode = spidNode->NextSibling(CHANNEL_XML_ELM_SPID))
    {
      const TiXmlElement *spidElem = spidNode->ToElement();
      if (spidElem != NULL)
      {
        SubtitleStream ss = { };

        ss.spid = StringUtils::IntVal(spidElem->GetText());

        const char *slang = spidElem->Attribute(CHANNEL_XML_ATTR_SLANG);
        if (slang != NULL)
          ss.slang = slang;

        m_subtitleStreams.push_back(ss);
      }
    }
  }

  const TiXmlNode *tpidNode = elem->FirstChild(CHANNEL_XML_ELM_TPID);
  if (tpidNode)
  {
    const TiXmlElement *tpidElem = tpidNode->ToElement();
    if (tpidElem != NULL)
    {
      TeletextStream ts = { };

      ts.tpid = StringUtils::IntVal(tpidElem->GetText());

      m_teletextStream = ts;
    }
  }

  const TiXmlNode *caidsNode = elem->FirstChild(CHANNEL_XML_ELM_CAIDS);
  if (caidsNode)
  {
    const TiXmlNode *caidNode = caidsNode->FirstChild(CHANNEL_XML_ELM_CAID);
    for (unsigned int i = 0; caidNode; i++, caidNode = caidNode->NextSibling(CHANNEL_XML_ELM_CAID))
    {
      const TiXmlElement *caidElem = caidNode->ToElement();
      if (caidElem != NULL)
      {
        uint16_t caSystemId = StringUtils::IntVal(caidElem->GetText());
        m_caDescriptors.push_back(CaDescriptorPtr(new cCaDescriptor(caSystemId)));
      }
    }
  }

  return true;
}

}
