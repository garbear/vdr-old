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
#include "epg/EPG.h"
#include "epg/Event.h"
#include "epg/Schedule.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/CRC32.h"
#include "utils/log/Log.h"
//#include "utils/UTF8Utils.h"
//#include "timers.h"

#include "libsi/si.h" // For AC3DescriptorTag

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <tinyxml.h>

using namespace std;

namespace VDR
{

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))
#endif

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

#define STRDIFF 0x01
#define VALDIFF 0x02

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

ChannelPtr cChannel::EmptyChannel;

cChannel::cChannel()
 : m_nid(0),
   m_tid(0),
   m_sid(0),
   m_rid(0),
   m_channelData(), // value-initialize
   m_modification(CHANNELMOD_NONE),
   m_schedule(NULL),
   //m_linkChannels(NULL), // TODO
   m_refChannel(NULL),
   m_channelHash(0)
{
}

cChannel::cChannel(const cChannel &channel)
 : m_schedule(NULL),
   //m_linkChannels(NULL),
   m_refChannel(NULL)
{
  *this = channel;
}

cChannel::~cChannel()
{
  //delete linkChannels;
  //linkChannels = NULL; // more than one channel can link to this one, so we need the following loop

  /* TODO
  for (cChannel *Channel = Channels.First(); Channel; Channel = Channels.Next(Channel))
  {
    if (Channel->m_linkChannels)
    {
      for (cLinkChannel *lc = Channel->m_linkChannels->First(); lc; lc = Channel->m_linkChannels->Next(lc))
      {
        if (lc->Channel() == this)
        {
          Channel->m_linkChannels->Del(lc);
          break;
        }
      }

      if (Channel->m_linkChannels->Count() == 0)
      {
        delete Channel->m_linkChannels;
        Channel->m_linkChannels = NULL;
      }
    }
  }
  */
}

cChannel& cChannel::operator=(const cChannel &channel)
{
  m_name        = channel.m_name;
  m_shortName   = channel.m_shortName;
  m_provider    = channel.m_provider;
  m_portalName  = channel.m_portalName;

  m_nid = channel.m_nid;
  m_tid = channel.m_tid;
  m_sid = channel.m_sid;
  m_rid = channel.m_rid;

  m_videoStream     = channel.m_videoStream;
  m_audioStreams    = channel.m_audioStreams;
  m_dataStreams     = channel.m_dataStreams;
  m_subtitleStreams = channel.m_subtitleStreams;
  m_teletextStream  = channel.m_teletextStream;

  m_caDescriptors   = channel.m_caDescriptors;

  m_channelData = channel.m_channelData;
  m_parameters  = channel.m_parameters;

  // m_modification
  // m_schedule
  // m_linkChannels
  // m_refChannel
  // m_channelHash

  SetChanged();

  return *this;
}

ChannelPtr cChannel::Clone() const
{
  return ChannelPtr(new cChannel(*this));
}

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

uint16_t cChannel::GetCaId(unsigned int i) const
{
  return i < m_caDescriptors.size() ? m_caDescriptors[i]->CaSystem() : 0;
}

vector<uint16_t> cChannel::GetCaIds() const
{
  vector<uint16_t> caIds;
  caIds.reserve(m_caDescriptors.size());

  for (vector<CaDescriptorPtr>::const_iterator it = m_caDescriptors.begin(); it != m_caDescriptors.end(); ++it)
    caIds.push_back((*it)->CaSystem());

  return caIds;
}

void cChannel::SetName(const string &strName, const string &strShortName, const string &strProvider)
{
  bool nn = (m_name != strName);
  bool ns = (m_shortName != strShortName);
  bool np = (m_provider != strProvider);
  if (nn || ns || np)
  {
    if (Number())
    {
      /* TODO
      dsyslog("changing name of channel %d from '%s,%s;%s' to '%s,%s;%s'",
          Number(), m_name.c_str(), m_shortName.c_str(), m_provider.c_str(),
          strName.c_str(), strShortName.c_str(), strProvider.c_str());
      */
      m_modification |= CHANNELMOD_NAME;
      SetChanged();
    }
    if (nn)
    {
      m_name = strName;
      SetChanged();
    }
    if (ns)
    {
      m_shortName = strShortName;
      SetChanged();
    }
    if (np)
    {
      m_provider = strProvider;
      SetChanged();
    }
  }
}

void cChannel::SetPortalName(const string &strPortalName)
{
  if (!strPortalName.empty() && m_portalName != strPortalName)
  {
    if (Number())
    {
      //dsyslog("changing portal name of channel %d from '%s' to '%s'", Number(), m_portalName.c_str(), strPortalName.c_str());
      m_modification |= CHANNELMOD_NAME;
      SetChanged();
    }
    m_portalName = strPortalName;
  }
}

void cChannel::SetId(uint16_t nid, uint16_t tid, uint16_t sid, uint16_t rid /* = 0 */)
{
  if (m_nid != nid || m_tid != tid || m_sid != sid || m_rid != rid)
  {
    if (Number())
    {
      //dsyslog("changing id of channel %d from %d-%d-%d-%d to %d-%d-%d-%d", Number(), m_nid, m_tid, m_sid, m_rid, nid, tid, sid, rid);
      m_modification |= CHANNELMOD_ID;
      //Channels.UnhashChannel(this); // TODO
    }
    m_nid = nid;
    m_tid = tid;
    m_sid = sid;
    m_rid = rid;
    if (Number())
      ;//Channels.HashChannel(this); // TODO
    m_schedule = cSchedules::EmptySchedule;
    SetChanged();
  }
}

void cChannel::SetStreams(const VideoStream& videoStream,
                          const vector<AudioStream>& audioStreams,
                          const vector<DataStream>& dataStreams,
                          const vector<SubtitleStream>& subtitleStreams,
                          const TeletextStream& teletextStream)
{
  eChannelMod mod = CHANNELMOD_NONE;
  if (m_videoStream     != videoStream ||
      m_audioStreams    != audioStreams ||
      m_dataStreams     != dataStreams ||
      m_subtitleStreams != subtitleStreams ||
      m_teletextStream  != teletextStream)
  {
    //TODO: check if PIDs or languages changed
    mod = (eChannelMod)(CHANNELMOD_PIDS | CHANNELMOD_LANGS);
  }

  if (mod != CHANNELMOD_NONE)
  {
    if (Number())
      ;//dsyslog("changing pids of channel %d from %d+%d=%d:%s:%s:%d to %d+%d=%d:%s:%s:%d", Number(), m_channelData.vpid, m_channelData.ppid, m_channelData.vtype, OldApidsBuf, OldSpidsBuf, m_channelData.tpid, vpid, ppid, vtype, NewApidsBuf, NewSpidsBuf, tpid);

    m_videoStream     = videoStream;
    m_audioStreams    = audioStreams;;
    m_dataStreams     = dataStreams;
    m_subtitleStreams = subtitleStreams;
    m_teletextStream  = teletextStream;

    m_modification |= mod;
    SetChanged();
  }
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

void cChannel::SetCaDescriptors(const CaDescriptorVector& caDescriptors)
{
  // TODO: Why is this check performed?
  if (!m_caDescriptors.empty() && m_caDescriptors[0]->CaSystem() <= CA_USER_MAX)
    return; // special values will not be overwritten

  if (m_caDescriptors != caDescriptors)
  {
    if (Number())
      ;//dsyslog("changing caids of channel %d from %s to %s", Number(), OldCaIdsBuf, NewCaIdsBuf);

    m_caDescriptors = caDescriptors;

    m_modification |= CHANNELMOD_CA;
    SetChanged();
  }
}

bool cChannel::SerialiseChannel(TiXmlNode *node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *channelElement = node->ToElement();
  if (channelElement == NULL)
    return false;

  channelElement->SetAttribute(CHANNEL_XML_ATTR_NAME,      m_name);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SHORTNAME, m_shortName);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_PROVIDER,  m_provider);
  // m_portalName

  channelElement->SetAttribute(CHANNEL_XML_ATTR_NID, m_nid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_TID, m_tid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SID, m_sid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_RID, m_rid);

  channelElement->SetAttribute(CHANNEL_XML_ATTR_VPID,  m_videoStream.vpid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_PPID,  m_videoStream.ppid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_VTYPE, m_videoStream.vtype);

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
    TiXmlElement apidsElement(CHANNEL_XML_ELM_APIDS);
    TiXmlNode *apidsNode = channelElement->InsertEndChild(apidsElement);
    if (apidsNode)
    {
      for (vector<DataStream>::const_iterator it = m_dataStreams.begin(); it != m_dataStreams.end(); ++it)
      {
        const DataStream& dataStream = *it;
        TiXmlElement apidElement(CHANNEL_XML_ELM_APID);
        TiXmlNode *apidNode = apidsNode->InsertEndChild(apidElement);
        if (apidNode)
        {
          TiXmlElement *apidElem = apidNode->ToElement();
          if (apidElem)
          {
            TiXmlText *apidText = new TiXmlText(StringUtils::Format("%d", dataStream.dpid));
            if (apidText)
            {
              apidElem->LinkEndChild(apidText);
              apidElem->SetAttribute(CHANNEL_XML_ATTR_ALANG, dataStream.dlang);
              apidElem->SetAttribute(CHANNEL_XML_ATTR_ATYPE, dataStream.dtype);
            }
          }
        }
      }
    }
  }

  if (!m_subtitleStreams.empty())
  {
    TiXmlElement apidsElement(CHANNEL_XML_ELM_APIDS);
    TiXmlNode *apidsNode = channelElement->InsertEndChild(apidsElement);
    if (apidsNode)
    {
      for (vector<SubtitleStream>::const_iterator it = m_subtitleStreams.begin(); it != m_subtitleStreams.end(); ++it)
      {
        const SubtitleStream& subtitleStream = *it;
        TiXmlElement apidElement(CHANNEL_XML_ELM_APID);
        TiXmlNode *apidNode = apidsNode->InsertEndChild(apidElement);
        if (apidNode)
        {
          TiXmlElement *apidElem = apidNode->ToElement();
          if (apidElem)
          {
            TiXmlText *apidText = new TiXmlText(StringUtils::Format("%d", subtitleStream.spid));
            if (apidText)
            {
              apidElem->LinkEndChild(apidText);
              apidElem->SetAttribute(CHANNEL_XML_ATTR_ALANG, subtitleStream.slang);
              // TODO: Should we also store subtitlingType, compositionPageId and ancillaryPageId?
            }
          }
        }
      }
    }
  }

  channelElement->SetAttribute(CHANNEL_XML_ATTR_TPID, m_teletextStream.tpid);

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

  TiXmlElement transponderElement(CHANNEL_XML_ELM_PARAMETERS);
  TiXmlNode *transponderNode = channelElement->InsertEndChild(transponderElement);
  if (transponderNode)
  {
    bool success = false;
    if (IsAtsc())
      success = m_parameters.Serialise(DVB_ATSC, transponderNode);
    else if (IsCable())
      success = m_parameters.Serialise(DVB_CABLE, transponderNode);
    else if (IsSat())
      success = m_parameters.Serialise(DVB_SAT, transponderNode);
    else if (IsTerr())
      success = m_parameters.Serialise(DVB_TERR, transponderNode);
    if (!success)
      return false;
  }

  channelElement->SetAttribute(CHANNEL_XML_ATTR_FREQUENCY, m_channelData.iFrequencyHz);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SOURCE,    cSource::ToString(m_channelData.source));
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SRATE,     m_channelData.srate);
  return true;
}

bool cChannel::Deserialise(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  const char *name = elem->Attribute(CHANNEL_XML_ATTR_NAME);
  if (name != NULL)
    m_name = name;

  const char *shortName = elem->Attribute(CHANNEL_XML_ATTR_SHORTNAME);
  if (shortName != NULL)
    m_shortName = shortName;

  const char *provider = elem->Attribute(CHANNEL_XML_ATTR_PROVIDER);
  if (provider != NULL)
    m_provider = provider;

  const char *nid = elem->Attribute(CHANNEL_XML_ATTR_NID);
  if (nid != NULL)
    m_nid = StringUtils::IntVal(nid);

  const char *tid = elem->Attribute(CHANNEL_XML_ATTR_TID);
  if (tid != NULL)
    m_tid = StringUtils::IntVal(tid);

  const char *sid = elem->Attribute(CHANNEL_XML_ATTR_SID);
  if (sid != NULL)
    m_sid = StringUtils::IntVal(sid);

  const char *rid = elem->Attribute(CHANNEL_XML_ATTR_RID);
  if (rid != NULL)
    m_rid = StringUtils::IntVal(rid);

  VideoStream vs = { };
  const char *vpid  = elem->Attribute(CHANNEL_XML_ATTR_VPID);
  const char *ppid  = elem->Attribute(CHANNEL_XML_ATTR_PPID);
  const char *vtype = elem->Attribute(CHANNEL_XML_ATTR_VTYPE);
  if (vpid != NULL && ppid != NULL && vtype != NULL)
  {
    vs.vpid  = StringUtils::IntVal(vpid);
    vs.ppid  = StringUtils::IntVal(ppid);
    vs.vtype = StringUtils::IntVal(vtype);
  }
  m_videoStream = vs;

  m_audioStreams.clear(); // TODO: Move to cChannel::Reset()
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
          as.atype = StringUtils::IntVal(atype);

        m_audioStreams.push_back(as);
      }
    }
  }

  m_dataStreams.clear(); // TODO: Move to cChannel::Reset()
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
          ds.dtype = StringUtils::IntVal(dtype);

        m_dataStreams.push_back(ds);
      }
    }
  }

  m_subtitleStreams.clear(); // TODO: Move to cChannel::Reset()
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

  TeletextStream ts = { };
  const char *tpid = elem->Attribute(CHANNEL_XML_ATTR_TPID);
  if (tpid != NULL)
  {
    ts.tpid = StringUtils::IntVal(tpid);
  }
  m_teletextStream = ts;

  m_caDescriptors.clear(); // TODO: Move to cChannel::Reset()
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

  const char *frequency = elem->Attribute(CHANNEL_XML_ATTR_FREQUENCY);
  if (frequency != NULL)
    m_channelData.iFrequencyHz = StringUtils::IntVal(frequency);

  const char *source = elem->Attribute(CHANNEL_XML_ATTR_SOURCE);
  if (source != NULL)
    m_channelData.source = cSource::FromString(source);

  const char *srate = elem->Attribute(CHANNEL_XML_ATTR_SRATE);
  if (srate != NULL)
    m_channelData.srate = StringUtils::IntVal(srate);

  m_parameters.Reset(); // TODO: Move to cChannel::Reset()
  const TiXmlNode *transponderNode = elem->FirstChild(CHANNEL_XML_ELM_PARAMETERS);
  if (transponderNode)
  {
    if (!m_parameters.Deserialise(transponderNode))
      return false;
  }
  else
  {
    // Backwards compatibility: look for parameters attribute
    const char *parameters = elem->Attribute(CHANNEL_XML_ATTR_PARAMETERS);
    if (parameters != NULL)
    {
      string strParameters(parameters);
      if (!m_parameters.Deserialise(strParameters))
        return false;
    }
  }

  m_channelHash = GetChannelID().Hash();
  return true;
}

bool cChannel::DeserialiseConf(const string &str)
{
  if (str.empty())
    return false;

  bool ok = true;

  if (str[0] == ':')
    return false; // Group separator
  else
  {
    const char *s = str.c_str();
    string strnamebuf;
    string strsourcebuf;
    string strparambuf;
    string strvpidbuf;
    string strapidbuf;
    string strtpidbuf;
    string strcaidbuf;

    /*!
    int fields = sscanf(s, "%a[^:]:%d :%a[^:]:%a[^:] :%d :%a[^:]:%a[^:]:%a[^:]:%a[^:]:%d :%d :%d :%d ",
        &namebuf, &m_channelData.frequency, &parambuf, &sourcebuf, &m_channelData.srate, &vpidbuf, &apidbuf, &tpidbuf, &caidbuf,
        &m_channelData.sid, &m_channelData.nid, &m_channelData.tid, &m_channelData.rid);
    */

    string strcopy = str;
    int fields = 0;
    size_t pos;

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        strnamebuf = strcopy.substr(0, pos);
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        strnamebuf = strcopy;
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_channelData.iFrequencyHz = StringUtils::IntVal(strcopy.substr(0, pos)) * 1000;
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_channelData.iFrequencyHz = StringUtils::IntVal(strcopy) * 1000;
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        strparambuf = strcopy.substr(0, pos);
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        strparambuf = strcopy;
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        strsourcebuf = strcopy.substr(0, pos);
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        strsourcebuf = strcopy;
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_channelData.srate = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_channelData.srate = StringUtils::IntVal(strcopy);
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        strvpidbuf = strcopy.substr(0, pos);
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        strvpidbuf = strcopy;
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        strapidbuf = strcopy.substr(0, pos);
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        strapidbuf = strcopy;
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        strtpidbuf = strcopy.substr(0, pos);
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        strtpidbuf = strcopy;
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        strcaidbuf = strcopy.substr(0, pos);
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        strcaidbuf = strcopy;
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_sid = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_sid = StringUtils::IntVal(strcopy);
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_nid = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_nid = StringUtils::IntVal(strcopy);
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_tid = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_tid = StringUtils::IntVal(strcopy);
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_rid = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_rid = StringUtils::IntVal(strcopy);
        strcopy.clear();
      }
    }

    char *namebuf = const_cast<char*>(strnamebuf.c_str());
    char *sourcebuf = const_cast<char*>(strsourcebuf.c_str());
    char *parambuf = const_cast<char*>(strparambuf.c_str());
    char *vpidbuf = const_cast<char*>(strvpidbuf.c_str());
    char *apidbuf = const_cast<char*>(strapidbuf.c_str());
    char *tpidbuf = const_cast<char*>(strtpidbuf.c_str());
    char *caidbuf = const_cast<char*>(strcaidbuf.c_str());

    if (fields >= 9)
    {
      /*
      if (fields == 9)
      {
        // allow reading of old format
        m_channelData.sid = atoi(caidbuf);
        delete caidbuf;
        caidbuf = NULL;
        if (sscanf(tpidbuf, "%d", &m_channelData.tpid) != 1)
          return false;

        m_channelData.caids[0] = m_channelData.tpid;
        m_channelData.caids[1] = 0;
        m_channelData.tpid = 0;
      }
      */

      // TODO: Move to cChannel::Reset()
      m_videoStream = VideoStream();
      m_audioStreams.clear();
      m_dataStreams.clear();
      m_subtitleStreams.clear();
      m_teletextStream = TeletextStream();
      m_caDescriptors.clear();

      ok = false;

      if (parambuf && sourcebuf && vpidbuf && apidbuf)
      {
        m_parameters.Deserialise(parambuf);
        ok = (m_channelData.source = cSource::FromString(sourcebuf)) >= 0;

        VideoStream vs;

        char *p;
        if ((p = strchr(vpidbuf, '=')) != NULL)
        {
          *p++ = 0;
          unsigned int vtype;
          if (sscanf(p, "%u", &vtype) != 1)
            return false;
          vs.vtype = vtype;
        }
        if ((p = strchr(vpidbuf, '+')) != NULL)
        {
          *p++ = 0;
          unsigned int ppid;
          if (sscanf(p, "%u", &ppid) != 1)
            return false;
          vs.ppid = ppid;
        }
        unsigned int vpid;
        if (sscanf(vpidbuf, "%u", &vpid) != 1)
          return false;
        vs.vpid = vpid;

        if (!vs.ppid)
          vs.ppid = vs.vpid;
        if (vs.vpid && !vs.vtype)
          vs.vtype = 2; // default is MPEG-2

        m_videoStream = vs;

        char *dpidbuf = strchr(apidbuf, ';');
        if (dpidbuf)
          *dpidbuf++ = 0;
        p = apidbuf;
        char *q;
        int NumApids = 0;
        char *strtok_next;
        while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
        {
          AudioStream as = { };

          as.apid = strtol(q, NULL, 10);
          if (as.apid != 0)
          {
            as.atype = 4; // backwards compatibility
            char *l = strchr(q, '=');
            if (l)
            {
              *l++ = 0;
              char *t = strchr(l, '@');
              if (t)
              {
                *t++ = 0;
                as.atype = strtol(t, NULL, 10);
              }
              as.alang = l;
            }
            m_audioStreams.push_back(as);
          }
          p = NULL;
        }

        if (dpidbuf)
        {
          char *p = dpidbuf;
          char *q;
          int NumDpids = 0;
          char *strtok_next;
          while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
          {
            DataStream ds = { };

            ds.dpid = strtol(q, NULL, 10);
            if (ds.dpid != 0)
            {
              ds.dtype = SI::AC3DescriptorTag; // backwards compatibility
              char *l = strchr(q, '=');
              if (l)
              {
                *l++ = 0;
                char *t = strchr(l, '@');
                if (t)
                {
                  *t++ = 0;
                  ds.dtype = strtol(t, NULL, 10);
                }
                ds.dlang = l;
              }
              m_dataStreams.push_back(ds);
            }
            p = NULL;
          }
        }

        if ((p = strchr(tpidbuf, ';')) != NULL)
        {
          *p++ = 0;
          char *q;
          char *strtok_next;
          while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
          {
            SubtitleStream ss = { };

            ss.spid = strtol(q, NULL, 10);
            char *l = strchr(q, '=');
            if (l)
            {
              *l++ = 0;
              ss.slang = l;
            }

            m_subtitleStreams.push_back(ss);
          }
        }

        unsigned int tpid;
        if (sscanf(tpidbuf, "%u", &tpid) != 1)
          return false;

        TeletextStream ts;
        ts.tpid = tpid;
        m_teletextStream = ts;

        if (caidbuf)
        {
          char *p = caidbuf;
          char *q;
          int NumCaIds = 0;
          char *strtok_next;
          while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
          {
            uint16_t caId = strtol(q, NULL, 16) & 0xFFFF;
            m_caDescriptors.push_back(CaDescriptorPtr(new cCaDescriptor(caId)));

            if (!m_caDescriptors.empty() && m_caDescriptors[0]->CaSystem() <= CA_USER_MAX)
              break;

            p = NULL;
          }
        }
      }

      //strreplace(namebuf, '|', ':'); // TODO
      char *p2 = namebuf; // TODO
      while (*p2) { if (*p2 == '|') *p2 = ':'; p2++; } // TODO

      char *p = strchr(namebuf, ';');
      if (p)
      {
        *p++ = 0;
        m_provider = p;
      }
      p = strrchr(namebuf, ','); // long name might contain a ',', so search for the rightmost one
      if (p)
      {
        *p++ = 0;
        m_shortName = p;
      }

      m_name = namebuf;

      if (!GetChannelID().Valid())
      {
        //esyslog("ERROR: channel data results in invalid ID!");
        return false;
      }
    }
    else
      return false;
  }

  if (ok)
    m_channelHash = GetChannelID().Hash();

  return ok;
}

int cChannel::TransponderFrequency() const
{
  int transponderFreq = FrequencyKHz();
  while (transponderFreq > 20000) // XXX
    transponderFreq /= 1000;

  if (IsSat())
    transponderFreq = Transponder(transponderFreq, m_parameters.Polarization());

  return transponderFreq;
}

unsigned int cChannel::Transponder(unsigned int frequency, fe_polarization polarization)
{
  // some satellites have transponders at the same frequency, just with different polarization:
  switch (polarization)
  {
  case POLARIZATION_HORIZONTAL:     frequency += 100000; break;
  case POLARIZATION_VERTICAL:       frequency += 200000; break;
  case POLARIZATION_CIRCULAR_LEFT:  frequency += 300000; break;
  case POLARIZATION_CIRCULAR_RIGHT: frequency += 400000; break;
  }
  return frequency;
}

tChannelID cChannel::GetChannelID() const
{
  if (m_nid || m_tid)
    return tChannelID(m_channelData.source, m_nid, m_tid, m_sid, m_rid);
  else
    return tChannelID(m_channelData.source, m_nid, TransponderFrequency(), m_sid, m_rid);
}

bool cChannel::HasTimer(const vector<cTimer2> &timers) const // TODO" cTimer2
{
  for (vector<cTimer2>::const_iterator timerIt = timers.begin(); timerIt != timers.end(); ++timerIt)
  {
    if (timerIt->Channel() == this)
      return true;
  }
  return false;
}

eChannelMod cChannel::Modification(eChannelMod mask /* = CHANNELMOD_ALL */)
{
  eChannelMod result = (eChannelMod)(m_modification & mask);
  m_modification = CHANNELMOD_NONE;
  return result;
}

void cChannel::CopyTransponderData(const cChannel &channel)
{
  m_channelData.iFrequencyHz = channel.m_channelData.iFrequencyHz;
  m_channelData.source       = channel.m_channelData.source;
  m_channelData.srate        = channel.m_channelData.srate;
  m_parameters               = channel.m_parameters;
  SetChanged();
}

bool cChannel::SetTransponderData(int source, int iFrequencyHz, int srate, const cDvbTransponderParams& parameters, bool bQuiet /* = false */)
{
  // Workarounds for broadcaster stupidity:
  // Some providers broadcast the transponder frequency of their channels with two different
  // values (like 12551000 and 12552000), so we need to allow for a little tolerance here
  if (abs(FrequencyHz() - iFrequencyHz) <= 1000)
    iFrequencyHz = FrequencyHz();
  // Sometimes the transponder frequency is set to 0, which is just wrong
  if (iFrequencyHz == 0)
    return false;
  // Sometimes the symbol rate is off by one
  if (abs(m_channelData.srate - srate) <= 1)
    srate = m_channelData.srate;

  if (m_channelData.source != source || FrequencyHz() != iFrequencyHz || m_channelData.srate != srate || m_parameters != parameters)
  {
    m_channelData.source = source;
    SetFrequencyHz(iFrequencyHz);
    m_channelData.srate = srate;
    m_parameters = parameters;
    m_schedule = cSchedules::EmptySchedule;

    if (Number() && !bQuiet)
    {
      dsyslog("changing transponder data of channel %s (%d)", Name().c_str(), Number());
      m_modification |= CHANNELMOD_TRANSP;
    }

    SetChanged();
    return true;
  }

  return false;
}

void cChannel::SetLinkChannels(cLinkChannels *linkChannels)
{
  if (!m_linkChannels && !linkChannels)
     return;

  if (m_linkChannels && linkChannels)
  {
    cLinkChannel *lca = m_linkChannels->First();
    cLinkChannel *lcb = linkChannels->First();
    while (lca && lcb)
    {
      if (lca->Channel() != lcb->Channel())
      {
        lca = NULL;
        break;
      }
      lca = m_linkChannels->Next(lca);
      lcb = linkChannels->Next(lcb);
    }
    if (!lca && !lcb)
    {
      delete linkChannels;
      return; // linkage has not changed
    }
  }

  char buffer[((m_linkChannels ? m_linkChannels->Count() : 0) + (linkChannels ? linkChannels->Count() : 0)) * 6 + 256]; // 6: 5 digit channel number plus blank, 256: other texts (see below) plus reserve
  char *q = buffer;
  q += sprintf(q, "linking channel %d from", Number());
  if (m_linkChannels)
  {
    for (cLinkChannel *lc = m_linkChannels->First(); lc; lc = m_linkChannels->Next(lc))
    {
      lc->Channel()->SetRefChannel(NULL);
      q += sprintf(q, " %d", lc->Channel()->Number());
    }
    delete m_linkChannels;
  }
  else
    q += sprintf(q, " none");

  q += sprintf(q, " to");

  m_linkChannels = linkChannels;
  if (m_linkChannels)
  {
    for (cLinkChannel *lc = m_linkChannels->First(); lc; lc = m_linkChannels->Next(lc))
    {
      lc->Channel()->SetRefChannel(this);
      q += sprintf(q, " %d", lc->Channel()->Number());
      //dsyslog("link %4d -> %4d: %s", Number(), lc->Channel()->Number(), lc->Channel()->Name());
    }
  }
  else
    q += sprintf(q, " none");

  if (Number())
    dsyslog("%s", buffer);
}

uint32_t cChannel::Hash(void) const
{
  return m_channelHash ? m_channelHash : GetChannelID().Hash();
}

void cChannel::SetSchedule(SchedulePtr schedule)
{
  m_schedule = schedule;
}

bool cChannel::HasSchedule(void) const
{
  return m_schedule;
}

SchedulePtr cChannel::Schedule(void) const
{
  return m_schedule;
}

}
