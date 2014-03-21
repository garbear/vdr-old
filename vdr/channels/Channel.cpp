/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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
#include "utils/StringUtils.h"
#include "utils/CRC32.h"
#include "filesystem/File.h"
//#include "utils/UTF8Utils.h"
#include "epg/EPG.h"
//#include "timers.h"

#include "libsi/si.h" // For AC3DescriptorTag

#include <algorithm>
#include <ctype.h> // For toupper
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

ChannelPtr cChannel::EmptyChannel;

static int IntArraysDiffer(const int *a, const int *b, const char na[][MAXLANGCODE2] = NULL, const char nb[][MAXLANGCODE2] = NULL)
{
  int result = 0;
  for (int i = 0; a[i] || b[i]; i++)
  {
    if (!a[i] || !b[i])
    {
      result |= VALDIFF;
      break;
    }
    if (na && nb && strcmp(na[i], nb[i]) != 0)
      result |= STRDIFF;
    if (a[i] != b[i])
      result |= VALDIFF;
  }
  return result;
}

static int IntArrayToString(char *s, const int *a, int Base = 10, const char n[][MAXLANGCODE2] = NULL, const int *t = NULL)
{
  char *q = s;
  int i = 0;
  while (a[i] || i == 0)
  {
    q += sprintf(q, Base == 16 ? "%s%X" : "%s%d", i ? "," : "", a[i]);
    const char *Delim = "=";
    if (a[i])
    {
      if (n && *n[i])
      {
        q += sprintf(q, "%s%s", Delim, n[i]);
        Delim = "";
      }
      if (t && t[i])
        q += sprintf(q, "%s@%d", Delim, t[i]);
    }

    if (!a[i])
      break;
    i++;
  }

  *q = 0;
  return q - s;
}

static string IntArrayToString(const int array[], bool hex = false, const char langs[][MAXLANGCODE2] = NULL, const int *types = NULL)
{
  string s;
  unsigned int i = 0;
  do
  {
    if (i)
      s += ",";
    s += StringUtils::Format(hex ? "%X" : "%d", array[i]);
    if (array[i] && langs && (langs[i][0] != '\0' || (types && types[i] != 0)))
    {
      s += "=";
      s += langs[i];
      if (types && types[i])
        s += "@" + StringUtils::Format("%d", types[i]);
    }
  } while (array[i++]);

  return s;
}

cChannel::cChannel()
 : m_channelData(), // value-initialize
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
  m_channelData = channel.m_channelData;
  m_parameters  = channel.m_parameters;

  SetChanged();

  return *this;
}

ChannelPtr cChannel::Clone() const
{
  return ChannelPtr(new cChannel(*this));
}

string cChannel::ShortName(bool bOrName /* = false */) const
{
  if (bOrName && m_shortName.empty())
    return Name();

  return m_shortName;
}

bool cChannel::SerialiseChannel(TiXmlNode *node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *channelElement = node->ToElement();
  if (channelElement == NULL)
    return false;

  channelElement->SetAttribute(CHANNEL_XML_ATTR_NAME,       m_name);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SHORTNAME,  m_shortName);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_PROVIDER,   m_provider);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_FREQUENCY,  m_channelData.iFrequencyHz);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SOURCE,     cSource::ToString(m_channelData.source));
  channelElement->SetAttribute(CHANNEL_XML_ATTR_SRATE,      m_channelData.srate);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_VPID,       m_channelData.vpid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_PPID,       m_channelData.ppid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_VTYPE,      m_channelData.vtype);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_TPID,       m_channelData.tpid);

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

  if (m_channelData.apids[0])
  {
    TiXmlElement apidsElement(CHANNEL_XML_ELM_APIDS);
    TiXmlNode *apidsNode = channelElement->InsertEndChild(apidsElement);
    if (apidsNode)
    {
      for (unsigned int i = 0; m_channelData.apids[i]; i++)
      {
        TiXmlElement apidElement(CHANNEL_XML_ELM_APID);
        TiXmlNode *apidNode = apidsNode->InsertEndChild(apidElement);
        if (apidNode)
        {
          TiXmlElement *apidElem = apidNode->ToElement();
          if (apidElem)
          {
            TiXmlText *apidText = new TiXmlText(StringUtils::Format("%d", m_channelData.apids[i]));
            if (apidText)
            {
              apidElem->LinkEndChild(apidText);
              apidElem->SetAttribute(CHANNEL_XML_ATTR_ALANG, m_channelData.alangs[i]);
              apidElem->SetAttribute(CHANNEL_XML_ATTR_ATYPE, m_channelData.atypes[i]);
            }
          }
        }
      }
    }
  }

  if (m_channelData.dpids[0])
  {
    TiXmlElement dpidsElement(CHANNEL_XML_ELM_DPIDS);
    TiXmlNode *dpidsNode = channelElement->InsertEndChild(dpidsElement);
    if (dpidsNode)
    {
      for (unsigned int i = 0; m_channelData.dpids[i]; i++)
      {
        TiXmlElement dpidElement(CHANNEL_XML_ELM_DPID);
        TiXmlNode *dpidNode = dpidsNode->InsertEndChild(dpidElement);
        if (dpidNode)
        {
          TiXmlElement *dpidElem = dpidNode->ToElement();
          if (dpidElem)
          {
            TiXmlText *dpidText = new TiXmlText(StringUtils::Format("%d", m_channelData.dpids[i]));
            if (dpidText)
            {
              dpidElem->LinkEndChild(dpidText);
              dpidElem->SetAttribute(CHANNEL_XML_ATTR_DLANG, m_channelData.dlangs[i]);
              dpidElem->SetAttribute(CHANNEL_XML_ATTR_DTYPE, m_channelData.dtypes[i]);
            }
          }
        }
      }
    }
  }

  if (m_channelData.spids[0])
  {
    TiXmlElement spidsElement(CHANNEL_XML_ELM_SPIDS);
    TiXmlNode *spidsNode = channelElement->InsertEndChild(spidsElement);
    if (spidsNode)
    {
      for (unsigned int i = 0; m_channelData.spids[i]; i++)
      {
        TiXmlElement spidElement(CHANNEL_XML_ELM_SPID);
        TiXmlNode *spidNode = spidsNode->InsertEndChild(spidElement);
        if (spidNode)
        {
          TiXmlElement *spidElem = spidNode->ToElement();
          if (spidElem)
          {
            TiXmlText *spidText = new TiXmlText(StringUtils::Format("%d", m_channelData.spids[i]));
            if (spidText)
            {
              spidElem->LinkEndChild(spidText);
              spidElem->SetAttribute(CHANNEL_XML_ATTR_SLANG, m_channelData.slangs[i]);
            }
          }
        }
      }
    }
  }

  if (m_channelData.caids[0])
  {
    TiXmlElement caidsElement(CHANNEL_XML_ELM_CAIDS);
    TiXmlNode *caidsNode = channelElement->InsertEndChild(caidsElement);
    if (caidsNode)
    {
      for (unsigned int i = 0; m_channelData.caids[i] != 0; i++)
      {
        TiXmlElement caidElement(CHANNEL_XML_ELM_CAID);
        TiXmlNode *caidNode = caidsNode->InsertEndChild(caidElement);
        if (caidNode)
        {
          TiXmlElement *caidElem = caidNode->ToElement();
          if (caidElem)
          {
            TiXmlText *caidText = new TiXmlText(StringUtils::Format("%d", m_channelData.caids[i]));
            if (caidText)
              caidElem->LinkEndChild(caidText);
          }
        }
      }
    }
  }

  channelElement->SetAttribute(CHANNEL_XML_ATTR_SID, m_channelData.sid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_NID, m_channelData.nid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_TID, m_channelData.tid);
  channelElement->SetAttribute(CHANNEL_XML_ATTR_RID, m_channelData.rid);

  return true;
}

bool cChannel::SerialiseSep(TiXmlNode *node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *separatorElement = node->ToElement();
  if (separatorElement == NULL)
    return false;

  separatorElement->SetAttribute(CHANNEL_XML_ATTR_NAME,       m_name);
  separatorElement->SetAttribute(CHANNEL_XML_ATTR_NUMBER,     m_channelData.number);

  return true;
}

bool cChannel::Deserialise(const TiXmlNode *node, bool bSeparator /* = false */)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (bSeparator)
  {
    // <separator> element
    const char *name = elem->Attribute(CHANNEL_XML_ATTR_NAME);
    if (name != NULL)
      m_name = name;

    const char *number = elem->Attribute(CHANNEL_XML_ATTR_NUMBER);
    if (number != NULL)
      m_channelData.number = StringUtils::IntVal(number);
  }
  else
  {
    // <channel> element
    const char *name = elem->Attribute(CHANNEL_XML_ATTR_NAME);
    if (name != NULL)
      m_name = name;

    const char *shortName = elem->Attribute(CHANNEL_XML_ATTR_SHORTNAME);
    if (shortName != NULL)
      m_shortName = shortName;

    const char *provider = elem->Attribute(CHANNEL_XML_ATTR_PROVIDER);
    if (provider != NULL)
      m_provider = provider;

    const char *frequency = elem->Attribute(CHANNEL_XML_ATTR_FREQUENCY);
    if (frequency != NULL)
      m_channelData.iFrequencyHz = StringUtils::IntVal(frequency);

    const char *source = elem->Attribute(CHANNEL_XML_ATTR_SOURCE);
    if (source != NULL)
      m_channelData.source = cSource::FromString(source);

    const char *srate = elem->Attribute(CHANNEL_XML_ATTR_SRATE);
    if (srate != NULL)
      m_channelData.srate = StringUtils::IntVal(srate);

    const char *vpid = elem->Attribute(CHANNEL_XML_ATTR_VPID);
    if (vpid != NULL)
      m_channelData.vpid = StringUtils::IntVal(vpid);

    const char *ppid = elem->Attribute(CHANNEL_XML_ATTR_PPID);
    if (ppid != NULL)
      m_channelData.ppid = StringUtils::IntVal(ppid);

    const char *vtype = elem->Attribute(CHANNEL_XML_ATTR_VTYPE);
    if (vtype != NULL)
      m_channelData.vtype = StringUtils::IntVal(vtype);

    const char *tpid = elem->Attribute(CHANNEL_XML_ATTR_TPID);
    if (tpid != NULL)
      m_channelData.tpid = StringUtils::IntVal(tpid);

    m_parameters.Reset();
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

    const TiXmlNode *apidsNode = elem->FirstChild(CHANNEL_XML_ELM_APIDS);
    if (apidsNode)
    {
      const TiXmlNode *apidNode = apidsNode->FirstChild(CHANNEL_XML_ELM_APID);
      unsigned int i = 0;
      while (apidNode && i < ARRAY_SIZE(m_channelData.apids))
      {
        const TiXmlElement *apidElem = apidNode->ToElement();
        if (apidElem != NULL)
        {
          m_channelData.apids[i] = StringUtils::IntVal(apidElem->GetText());
          const char *alang = apidElem->Attribute(CHANNEL_XML_ATTR_ALANG);
          if (alang != NULL)
            strncpy(m_channelData.alangs[i], alang, sizeof(m_channelData.alangs[i]));
          else
            m_channelData.alangs[i][0] = '\0';
          const char *atype = apidElem->Attribute(CHANNEL_XML_ATTR_ATYPE);
          m_channelData.atypes[i] = StringUtils::IntVal(atype ? atype : "");
        }
        apidNode = apidNode->NextSibling(CHANNEL_XML_ELM_APID);
        i++;
      }
    }

    const TiXmlNode *dpidsNode = elem->FirstChild(CHANNEL_XML_ELM_DPIDS);
    if (dpidsNode)
    {
      const TiXmlNode *dpidNode = dpidsNode->FirstChild(CHANNEL_XML_ELM_DPID);
      unsigned int i = 0;
      while (dpidNode && i < ARRAY_SIZE(m_channelData.dpids))
      {
        const TiXmlElement *dpidElem = dpidNode->ToElement();
        if (dpidElem != NULL)
        {
          m_channelData.dpids[i] = StringUtils::IntVal(dpidElem->GetText());
          const char *dlang = dpidElem->Attribute(CHANNEL_XML_ATTR_DLANG);
          if (dlang != NULL)
            strncpy(m_channelData.dlangs[i], dlang, sizeof(m_channelData.dlangs[i]));
          else
            m_channelData.dlangs[i][0] = '\0';
          const char *dtype = dpidElem->Attribute(CHANNEL_XML_ATTR_DTYPE);
          m_channelData.dtypes[i] = StringUtils::IntVal(dtype ? dtype : "");
        }
        dpidNode = dpidNode->NextSibling(CHANNEL_XML_ELM_DPID);
        i++;
      }
    }

    const TiXmlNode *spidsNode = elem->FirstChild(CHANNEL_XML_ELM_SPIDS);
    if (spidsNode)
    {
      const TiXmlNode *spidNode = spidsNode->FirstChild(CHANNEL_XML_ELM_SPID);
      unsigned int i = 0;
      while (spidNode && i < ARRAY_SIZE(m_channelData.spids))
      {
        const TiXmlElement *spidElem = spidNode->ToElement();
        if (spidElem != NULL)
        {
          m_channelData.spids[i] = StringUtils::IntVal(spidElem->GetText());
          const char *slang = spidElem->Attribute(CHANNEL_XML_ATTR_SLANG);
          if (slang != NULL)
            strncpy(m_channelData.slangs[i], slang, sizeof(m_channelData.slangs[i]));
          else
            m_channelData.slangs[i][0] = '\0';
        }
        spidNode = spidNode->NextSibling(CHANNEL_XML_ELM_SPID);
        i++;
      }
    }

    const TiXmlNode *caidsNode = elem->FirstChild(CHANNEL_XML_ELM_CAIDS);
    if (caidsNode)
    {
      const TiXmlNode *caidNode = caidsNode->FirstChild(CHANNEL_XML_ELM_CAID);
      unsigned int i = 0;
      while (caidNode && i < ARRAY_SIZE(m_channelData.caids))
      {
        const TiXmlElement *caidElem = caidNode->ToElement();
        if (caidElem != NULL)
          m_channelData.caids[i] = StringUtils::IntVal(caidElem->GetText());
        caidNode = caidNode->NextSibling(CHANNEL_XML_ELM_CAID);
        i++;
      }
    }

    const char *sid = elem->Attribute(CHANNEL_XML_ATTR_SID);
    if (sid != NULL)
      m_channelData.sid = StringUtils::IntVal(sid);

    const char *nid = elem->Attribute(CHANNEL_XML_ATTR_NID);
    if (nid != NULL)
      m_channelData.nid = StringUtils::IntVal(nid);

    const char *tid = elem->Attribute(CHANNEL_XML_ATTR_TID);
    if (tid != NULL)
      m_channelData.tid = StringUtils::IntVal(tid);

    const char *rid = elem->Attribute(CHANNEL_XML_ATTR_RID);
    if (rid != NULL)
      m_channelData.rid = StringUtils::IntVal(rid);
  }

  m_channelHash = GetChannelID().Hash();
  return true;
}

bool cChannel::DeserialiseConf(const string &str)
{
  bool ok = true;
  const char *s = str.c_str();
  if (*s == ':')
  {
    m_channelData.groupSep = true;
    if (*++s == '@' && *++s)
    {
      char *p = NULL;
      errno = 0;
      int n = strtol(s, &p, 10);
      if (!errno && p != s && n > 0)
      {
        m_channelData.number = n;
        s = p;
      }
    }
    m_name = s;
    StringUtils::TrimLeft(m_name);
    StringUtils::Replace(m_name, '|', ':');
  }
  else
  {
    m_channelData.groupSep = false;
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
        m_channelData.sid = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_channelData.sid = StringUtils::IntVal(strcopy);
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_channelData.nid = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_channelData.nid = StringUtils::IntVal(strcopy);
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_channelData.tid = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_channelData.tid = StringUtils::IntVal(strcopy);
        strcopy.clear();
      }
    }

    if (!strcopy.empty())
    {
      fields++;
      if ((pos = strcopy.find(':')) != string::npos)
      {
        m_channelData.rid = StringUtils::IntVal(strcopy.substr(0, pos));
        strcopy = strcopy.substr(pos + 1);
      }
      else
      {
        m_channelData.rid = StringUtils::IntVal(strcopy);
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

      m_channelData.vpid = m_channelData.ppid = 0;
      m_channelData.vtype = 0;
      m_channelData.apids[0] = 0;
      m_channelData.atypes[0] = 0;
      m_channelData.dpids[0] = 0;
      m_channelData.dtypes[0] = 0;
      m_channelData.spids[0] = 0;

      ok = false;

      if (parambuf && sourcebuf && vpidbuf && apidbuf)
      {
        m_parameters.Deserialise(parambuf);
        ok = (m_channelData.source = cSource::FromString(sourcebuf)) >= 0;

        char *p;
        if ((p = strchr(vpidbuf, '=')) != NULL)
        {
          *p++ = 0;
          if (sscanf(p, "%d", &m_channelData.vtype) != 1)
            return false;
        }
        if ((p = strchr(vpidbuf, '+')) != NULL)
        {
          *p++ = 0;
          if (sscanf(p, "%d", &m_channelData.ppid) != 1)
            return false;
        }
        if (sscanf(vpidbuf, "%d", &m_channelData.vpid) != 1)
          return false;
        if (!m_channelData.ppid)
          m_channelData.ppid = m_channelData.vpid;
        if (m_channelData.vpid && !m_channelData.vtype)
          m_channelData.vtype = 2; // default is MPEG-2

        char *dpidbuf = strchr(apidbuf, ';');
        if (dpidbuf)
          *dpidbuf++ = 0;
        p = apidbuf;
        char *q;
        int NumApids = 0;
        char *strtok_next;
        while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
        {
          if (NumApids < MAXAPIDS)
          {
            m_channelData.atypes[NumApids] = 4; // backwards compatibility
            char *l = strchr(q, '=');
            if (l)
            {
              *l++ = 0;
              char *t = strchr(l, '@');
              if (t)
              {
                *t++ = 0;
                m_channelData.atypes[NumApids] = strtol(t, NULL, 10);
              }
              strn0cpy(m_channelData.alangs[NumApids], l, MAXLANGCODE2);
            }
            else
              *m_channelData.alangs[NumApids] = 0;

            if ((m_channelData.apids[NumApids] = strtol(q, NULL, 10)) != 0)
              NumApids++;
          }
          else
            ;//esyslog("ERROR: too many APIDs!"); // no need to set ok to 'false'

          p = NULL;
        }

        m_channelData.apids[NumApids] = 0;
        m_channelData.atypes[NumApids] = 0;

        if (dpidbuf)
        {
          char *p = dpidbuf;
          char *q;
          int NumDpids = 0;
          char *strtok_next;
          while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
          {
            if (NumDpids < MAXDPIDS)
            {
              m_channelData.dtypes[NumDpids] = SI::AC3DescriptorTag; // backwards compatibility
              char *l = strchr(q, '=');
              if (l)
              {
                *l++ = 0;
                char *t = strchr(l, '@');
                if (t)
                {
                  *t++ = 0;
                  m_channelData.dtypes[NumDpids] = strtol(t, NULL, 10);
                }
                strn0cpy(m_channelData.dlangs[NumDpids], l, MAXLANGCODE2);
              }
              else
                *m_channelData.dlangs[NumDpids] = 0;

              if ((m_channelData.dpids[NumDpids] = strtol(q, NULL, 10)) != 0)
                NumDpids++;
            }
            else
              ;//esyslog("ERROR: too many DPIDs!"); // no need to set ok to 'false'

            p = NULL;
          }
          m_channelData.dpids[NumDpids] = 0;
          m_channelData.dtypes[NumDpids] = 0;
        }

        int NumSpids = 0;
        if ((p = strchr(tpidbuf, ';')) != NULL)
        {
          *p++ = 0;
          char *q;
          char *strtok_next;
          while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
          {
            if (NumSpids < MAXSPIDS)
            {
              char *l = strchr(q, '=');
              if (l)
              {
                *l++ = 0;
                strn0cpy(m_channelData.slangs[NumSpids], l, MAXLANGCODE2);
              }
              else
                *m_channelData.slangs[NumSpids] = 0;

              m_channelData.spids[NumSpids++] = strtol(q, NULL, 10);
            }
            else
              ;//esyslog("ERROR: too many SPIDs!"); // no need to set ok to 'false'

            p = NULL;
          }
          m_channelData.spids[NumSpids] = 0;
        }

        if (sscanf(tpidbuf, "%d", &m_channelData.tpid) != 1)
          return false;

        if (caidbuf)
        {
          char *p = caidbuf;
          char *q;
          int NumCaIds = 0;
          char *strtok_next;
          while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
          {
            if (NumCaIds < MAXCAIDS)
            {
              m_channelData.caids[NumCaIds++] = strtol(q, NULL, 16) & 0xFFFF;
              if (NumCaIds == 1 && m_channelData.caids[0] <= CA_USER_MAX)
                break;
            }
            else
              ;//esyslog("ERROR: too many CA ids!"); // no need to set ok to 'false'
            p = NULL;
          }
          m_channelData.caids[NumCaIds] = 0;
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

int cChannel::Transponder() const
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
  if (m_channelData.nid || m_channelData.tid)
    return tChannelID(m_channelData.source, m_channelData.nid, m_channelData.tid, m_channelData.sid, m_channelData.rid);
  else
    return tChannelID(m_channelData.source, m_channelData.nid, Transponder(), m_channelData.sid, m_channelData.rid);
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

int cChannel::Modification(int mask /* = CHANNELMOD_ALL */)
{
  int result = m_modification & mask;
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

void cChannel::SetId(int nid, int tid, int sid, int rid /* = 0 */)
{
  if (m_channelData.nid != nid || m_channelData.tid != tid || m_channelData.sid != sid || m_channelData.rid != rid)
  {
    if (Number())
    {
      //dsyslog("changing id of channel %d from %d-%d-%d-%d to %d-%d-%d-%d", Number(), m_channelData.nid, m_channelData.tid, m_channelData.sid, m_channelData.rid, nid, tid, sid, rid);
      m_modification |= CHANNELMOD_ID;
      //Channels.UnhashChannel(this); // TODO
    }
    m_channelData.nid = nid;
    m_channelData.tid = tid;
    m_channelData.sid = sid;
    m_channelData.rid = rid;
    if (Number())
      ;//Channels.HashChannel(this); // TODO
    m_schedule = cSchedules::EmptySchedule;
    SetChanged();
  }
}

void cChannel::SetName(const string &strName, const string &strShortName, const string &strProvider)
{
  if (!strName.empty())
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

void cChannel::SetPids(int vpid, int ppid, int vtype, int *apids, int *atypes, char aLangs[][MAXLANGCODE2],
                                                      int *dpids, int *dtypes, char dLangs[][MAXLANGCODE2],
                                                      int *spids, char sLangs[][MAXLANGCODE2], int tpid)
{
  int mod = CHANNELMOD_NONE;
  if (m_channelData.vpid != vpid || m_channelData.ppid != ppid || m_channelData.vtype != vtype || m_channelData.tpid != tpid)
    mod |= CHANNELMOD_PIDS;

  int m = IntArraysDiffer(m_channelData.apids, apids, m_channelData.alangs, aLangs) |
          IntArraysDiffer(m_channelData.atypes, atypes) |
          IntArraysDiffer(m_channelData.dpids, dpids, m_channelData.dlangs, dLangs) |
          IntArraysDiffer(m_channelData.dtypes, dtypes) | IntArraysDiffer(m_channelData.spids, spids, m_channelData.slangs, sLangs);

  if (m & STRDIFF)
    mod |= CHANNELMOD_LANGS;

  if (m & VALDIFF)
    mod |= CHANNELMOD_PIDS;

  if (mod)
  {
    const int BufferSize = (MAXAPIDS + MAXDPIDS) * (5 + 1 + MAXLANGCODE2 + 5) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod@type', +10: paranoia
    char OldApidsBuf[BufferSize];
    char NewApidsBuf[BufferSize];
    char *q = OldApidsBuf;
    q += IntArrayToString(q, m_channelData.apids, 10, m_channelData.alangs, m_channelData.atypes);
    if (m_channelData.dpids[0])
    {
      *q++ = ';';
      q += IntArrayToString(q, m_channelData.dpids, 10, m_channelData.dlangs, m_channelData.dtypes);
    }

    *q = 0;
    q = NewApidsBuf;
    q += IntArrayToString(q, apids, 10, aLangs, atypes);
    if (dpids[0])
    {
      *q++ = ';';
      q += IntArrayToString(q, dpids, 10, dLangs, dtypes);
    }

    *q = 0;
    const int SBufferSize = MAXSPIDS * (5 + 1 + MAXLANGCODE2) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod', +10: paranoia
    char OldSpidsBuf[SBufferSize];
    char NewSpidsBuf[SBufferSize];

    q = OldSpidsBuf;
    q += IntArrayToString(q, m_channelData.spids, 10, m_channelData.slangs);
    *q = 0;
    q = NewSpidsBuf;
    q += IntArrayToString(q, spids, 10, sLangs);
    *q = 0;
    if (Number())
      ;//dsyslog("changing pids of channel %d from %d+%d=%d:%s:%s:%d to %d+%d=%d:%s:%s:%d", Number(), m_channelData.vpid, m_channelData.ppid, m_channelData.vtype, OldApidsBuf, OldSpidsBuf, m_channelData.tpid, vpid, ppid, vtype, NewApidsBuf, NewSpidsBuf, tpid);

    m_channelData.vpid = vpid;
    m_channelData.ppid = ppid;
    m_channelData.vtype = vtype;

    for (int i = 0; i < MAXAPIDS; i++)
    {
      m_channelData.apids[i] = apids[i];
      m_channelData.atypes[i] = atypes[i];
      strn0cpy(m_channelData.alangs[i], aLangs[i], MAXLANGCODE2);
    }
    m_channelData.apids[MAXAPIDS] = 0;

    for (int i = 0; i < MAXDPIDS; i++)
    {
      m_channelData.dpids[i] = dpids[i];
      m_channelData.dtypes[i] = dtypes[i];
      strn0cpy(m_channelData.dlangs[i], dLangs[i], MAXLANGCODE2);
    }
    m_channelData.dpids[MAXDPIDS] = 0;

    for (int i = 0; i < MAXSPIDS; i++)
    {
      m_channelData.spids[i] = spids[i];
      strn0cpy(m_channelData.slangs[i], sLangs[i], MAXLANGCODE2);
    }
    m_channelData.spids[MAXSPIDS] = 0;

    m_channelData.tpid = tpid;
    m_modification |= mod;
    SetChanged();
  }
}

void cChannel::SetSubtitlingDescriptors(uchar *subtitlingTypes, uint16_t *compositionPageIds, uint16_t *ancillaryPageIds)
{
  if (subtitlingTypes)
  {
    for (int i = 0; i < MAXSPIDS; i++)
      m_channelData.subtitlingTypes[i] = subtitlingTypes[i];
  }

  if (compositionPageIds)
  {
    for (int i = 0; i < MAXSPIDS; i++)
      m_channelData.compositionPageIds[i] = compositionPageIds[i];
  }

  if (ancillaryPageIds)
  {
    for (int i = 0; i < MAXSPIDS; i++)
      m_channelData.ancillaryPageIds[i] = ancillaryPageIds[i];
  }

  if (subtitlingTypes || compositionPageIds || ancillaryPageIds)
    SetChanged();
}

void cChannel::SetCaIds(const int *caIds)
{
  if (m_channelData.caids[0] && m_channelData.caids[0] <= CA_USER_MAX)
    return; // special values will not be overwritten

  if (IntArraysDiffer(m_channelData.caids, caIds))
  {
    char OldCaIdsBuf[MAXCAIDS * 5 + 10]; // 5: 4 digits plus delimiting ',', 10: paranoia
    char NewCaIdsBuf[MAXCAIDS * 5 + 10];
    IntArrayToString(OldCaIdsBuf, m_channelData.caids, 16);
    IntArrayToString(NewCaIdsBuf, caIds, 16);
    if (Number())
      ;//dsyslog("changing caids of channel %d from %s to %s", Number(), OldCaIdsBuf, NewCaIdsBuf);
    for (int i = 0; i <= MAXCAIDS; i++) // <= to copy the terminating 0
    {
      m_channelData.caids[i] = caIds[i];
      if (!caIds[i])
        break;
    }
    m_modification |= CHANNELMOD_CA;
    SetChanged();
  }
}

void cChannel::SetCaDescriptors(int level)
{
  if (level > 0)
  {
    m_modification |= CHANNELMOD_CA;
    SetChanged();
    if (Number() && level > 1)
      ;//dsyslog("changing ca descriptors of channel %d", Number());
  }
}
/*
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
*/

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
