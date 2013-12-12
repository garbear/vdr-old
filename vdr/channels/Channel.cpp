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
#include "utils/StringUtils.h"
#include "utils/Tools.h"
//#include "utils/UTF8Utils.h"
//#include "epg.h"
//#include "timers.h"

#include "libsi/si.h" // For AC3DescriptorTag

#include <ctype.h> // For toupper
#include <stdio.h>

using namespace std;

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

#define STRDIFF 0x01
#define VALDIFF 0x02

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

cChannel::cChannel()
 : m_channelData(), // value-initialize
   m_modification(CHANNELMOD_NONE),
   m_schedule(NULL),
   //m_linkChannels(NULL), // TODO
   m_refChannel(NULL)
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

  return *this;
}

string cChannel::ShortName(bool bOrName /* = false */) const
{
  if (bOrName && m_shortName.empty())
    return Name();

  return m_shortName;
}

string cChannel::ToText(const cChannel &channel)
{
  char FullName[channel.m_name.size() + 1 + channel.m_shortName.size() + 1 + channel.m_provider.size() + 1 + 10]; // +10: paranoia
  char *q = FullName;
  q += sprintf(q, "%s", channel.m_name.c_str());

  if (!channel.m_channelData.groupSep)
  {
    if (!channel.m_shortName.empty())
      q += sprintf(q, ",%s", channel.m_shortName.c_str());
    else if (channel.m_name.find(',') != string::npos)
      q += sprintf(q, ",");
    if (!channel.m_provider.empty())
      q += sprintf(q, ";%s", channel.m_provider.c_str());
  }
  *q = 0;

  //strreplace(FullName, ':', '|'); // TODO
  char *p = FullName; // TODO
  while (*p) { if (*p == ':') *p = '|'; p++; } // TODO

  string buffer;
  if (channel.m_channelData.groupSep)
  {
    if (channel.m_channelData.number)
      buffer = StringUtils::Format(":@%d %s\n", channel.m_channelData.number, FullName);
    else
      buffer = StringUtils::Format(":%s\n", FullName);
  }
  else
  {
    char vpidbuf[32];
    char *q = vpidbuf;
    q += snprintf(q, sizeof(vpidbuf), "%d", channel.m_channelData.vpid);
    if (channel.m_channelData.ppid && channel.m_channelData.ppid != channel.m_channelData.vpid)
      q += snprintf(q, sizeof(vpidbuf) - (q - vpidbuf), "+%d", channel.m_channelData.ppid);
    if (channel.m_channelData.vpid && channel.m_channelData.vtype)
      q += snprintf(q, sizeof(vpidbuf) - (q - vpidbuf), "=%d", channel.m_channelData.vtype);
    *q = 0;

    const int ABufferSize = (MAXAPIDS + MAXDPIDS) * (5 + 1 + MAXLANGCODE2 + 5) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod@type', +10: paranoia
    char apidbuf[ABufferSize];
    q = apidbuf;
    q += IntArrayToString(q, channel.m_channelData.apids, 10, channel.m_channelData.alangs, channel.m_channelData.atypes);
    if (channel.m_channelData.dpids[0])
    {
      *q++ = ';';
      q += IntArrayToString(q, channel.m_channelData.dpids, 10, channel.m_channelData.dlangs, channel.m_channelData.dtypes);
    }
    *q = 0;

    const int TBufferSize = MAXSPIDS * (5 + 1 + MAXLANGCODE2) + 10; // 5 digits plus delimiting ',' or ';' plus optional '=cod+cod', +10: paranoia and tpid
    char tpidbuf[TBufferSize];
    q = tpidbuf;
    q += snprintf(q, sizeof(tpidbuf), "%d", channel.m_channelData.tpid);
    if (channel.m_channelData.spids[0])
    {
      *q++ = ';';
      q += IntArrayToString(q, channel.m_channelData.spids, 10, channel.m_channelData.slangs);
    }
    char caidbuf[MAXCAIDS * 5 + 10]; // 5: 4 digits plus delimiting ',', 10: paranoia
    q = caidbuf;
    q += IntArrayToString(q, channel.m_channelData.caids, 16);
    *q = 0;

    buffer = StringUtils::Format("%s:%d:%s:%s:%d:%s:%s:%s:%s:%d:%d:%d:%d\n",
        FullName, channel.m_channelData.frequency, channel.m_parameters.c_str(), cSource::ToString(channel.m_channelData.source).c_str(), channel.m_channelData.srate,
        vpidbuf, apidbuf, tpidbuf, caidbuf, channel.m_channelData.sid, channel.m_channelData.nid, channel.m_channelData.tid, channel.m_channelData.rid);
  }

  return buffer;
}

bool cChannel::Parse(const string &str)
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
    char *namebuf = NULL;
    char *sourcebuf = NULL;
    char *parambuf = NULL;
    char *vpidbuf = NULL;
    char *apidbuf = NULL;
    char *tpidbuf = NULL;
    char *caidbuf = NULL;

    int fields = sscanf(s, "%a[^:]:%d :%a[^:]:%a[^:] :%d :%a[^:]:%a[^:]:%a[^:]:%a[^:]:%d :%d :%d :%d ",
        &namebuf, &m_channelData.frequency, &parambuf, &sourcebuf, &m_channelData.srate, &vpidbuf, &apidbuf, &tpidbuf, &caidbuf,
        &m_channelData.sid, &m_channelData.nid, &m_channelData.tid, &m_channelData.rid);
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
        m_parameters = parambuf;
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

      free(parambuf);
      free(sourcebuf);
      free(vpidbuf);
      free(apidbuf);
      free(tpidbuf);
      free(caidbuf);
      free(namebuf);

      if (!GetChannelID().Valid())
      {
        //esyslog("ERROR: channel data results in invalid ID!");
        return false;
      }
    }
    else
      return false;
  }

  return ok;
}

bool cChannel::Save(FILE *f) const
{
  return fprintf(f, "%s", ToText().c_str()) > 0;
}

int cChannel::Transponder() const
{
  int transponderFreq = m_channelData.frequency;
  while (transponderFreq > 20000)
    transponderFreq /= 1000;

  if (IsSat())
  {
    for (unsigned int i = 0; i < m_parameters.size(); i++)
    {
      // Check for lowercase for backwards compatibility
      char polarization = toupper(m_parameters[i]);
      if (polarization == 'H' ||
          polarization == 'V' ||
          polarization == 'L' ||
          polarization == 'R')
      {
        transponderFreq = Transponder(transponderFreq, polarization);
        break;
      }
    }
  }
  return transponderFreq;
}

unsigned int cChannel::Transponder(unsigned int frequency, char polarization)
{
  // some satellites have transponders at the same frequency, just with different polarization:
  switch (toupper(polarization))
  {
  case 'H': frequency += 100000; break;
  case 'V': frequency += 200000; break;
  case 'L': frequency += 300000; break;
  case 'R': frequency += 400000; break;
  default: /*esyslog("ERROR: invalid value for Polarization '%c'", polarization);*/ break;
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
  m_channelData.frequency = channel.m_channelData.frequency;
  m_channelData.source    = channel.m_channelData.source;
  m_channelData.srate     = channel.m_channelData.srate;
  m_parameters            = channel.m_parameters;
}

bool cChannel::SetTransponderData(int source, int frequency, int srate, const string &strParameters, bool bQuiet /* = false */)
{
  if (strParameters.find(':') != string::npos)
  {
    //esyslog("ERROR: parameter string '%s' contains ':'", strParameters.c_str());
    return false;
  }

  // Workarounds for broadcaster stupidity:
  // Some providers broadcast the transponder frequency of their channels with two different
  // values (like 12551 and 12552), so we need to allow for a little tolerance here
  if (abs(m_channelData.frequency - frequency) <= 1)
    frequency = m_channelData.frequency;
  // Sometimes the transponder frequency is set to 0, which is just wrong
  if (frequency == 0)
    return false;
  // Sometimes the symbol rate is off by one
  if (abs(m_channelData.srate - srate) <= 1)
    srate = m_channelData.srate;

  if (m_channelData.source != source || m_channelData.frequency != frequency || m_channelData.srate != srate || m_parameters != strParameters)
  {
    string oldTransponderData = TransponderDataToString();
    m_channelData.source = source;
    m_channelData.frequency = frequency;
    m_channelData.srate = srate;
    m_parameters = strParameters;
    m_schedule = NULL;

    if (Number() && !bQuiet)
    {
      //dsyslog("changing transponder data of channel %d from %s to %s", Number(), oldTransponderData.c_str(), TransponderDataToString().c_str());
      m_modification |= CHANNELMOD_TRANSP;
      SetChanged();
    }
  }
  return true;
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
    m_schedule = NULL;
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
string cChannel::TransponderDataToString() const
{
  if (cSource::IsTerr(m_channelData.source))
    return StringUtils::Format("%d:%s:%s", m_channelData.frequency, m_parameters.c_str(), cSource::ToString(m_channelData.source).c_str());
  return StringUtils::Format("%d:%s:%s:%d", m_channelData.frequency, m_parameters.c_str(), cSource::ToString(m_channelData.source).c_str(), m_channelData.srate);
}
