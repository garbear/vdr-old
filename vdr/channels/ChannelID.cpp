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

#include "ChannelID.h"
#include "sources/Source.h"
#include "utils/StringUtils.h"

#include <stddef.h>
#include <stdio.h>

using namespace std;

const tChannelID tChannelID::InvalidID;

tChannelID::tChannelID()
 : m_source(0),
   m_nid(0),
   m_tid(0),
   m_sid(0),
   m_rid(0)
{
}

tChannelID::tChannelID(int source, int nid, int tid, int sid, int rid /* = 0 */)
 : m_source(source),
   m_nid(nid),
   m_tid(tid),
   m_sid(sid),
   m_rid(rid)
{
}

bool tChannelID::operator==(const tChannelID &arg) const
{
  return m_source == arg.m_source &&
         m_nid == arg.m_nid &&
         m_tid == arg.m_tid &&
         m_sid == arg.m_sid &&
         m_rid == arg.m_rid;
}

string tChannelID::Serialize() const
{
  if (m_rid)
    return StringUtils::Format("%s-%d-%d-%d-%d", cSource::ToString(m_source).c_str(), m_nid, m_tid, m_sid, m_rid);
  else
    return StringUtils::Format("%s-%d-%d-%d", cSource::ToString(m_source).c_str(), m_nid, m_tid, m_sid);
}

tChannelID tChannelID::Deserialize(const std::string &str)
{
  tChannelID ret = tChannelID::InvalidID;
  string sourcebuf;
  string strcopy(str);
  int nid;
  int tid;
  int sid;
  int rid = 0;

  //int fields = sscanf(str.c_str(), "%a[^-]-%d-%d-%d-%d", &sourcebuf, &nid, &tid, &sid, &rid);

  int fields = 0;
  size_t pos;

  if (!strcopy.empty())
  {
    fields++;
    if ((pos = strcopy.find('-')) != string::npos)
    {
      sourcebuf = strcopy.substr(0, pos);
      strcopy = strcopy.substr(pos + 1);
    }
    else
    {
      sourcebuf = strcopy;
      strcopy.clear();
    }
  }

  if (!strcopy.empty())
  {
    fields++;
    if ((pos = strcopy.find('-')) != string::npos)
    {
      nid = StringUtils::IntVal(strcopy.substr(0, pos));
      strcopy = strcopy.substr(pos + 1);
    }
    else
    {
      nid = StringUtils::IntVal(strcopy);
      strcopy.clear();
    }
  }

  if (!strcopy.empty())
  {
    fields++;
    if ((pos = strcopy.find('-')) != string::npos)
    {
      tid = StringUtils::IntVal(strcopy.substr(0, pos));
      strcopy = strcopy.substr(pos + 1);
    }
    else
    {
      tid = StringUtils::IntVal(strcopy);
      strcopy.clear();
    }
  }

  if (!strcopy.empty())
  {
    fields++;
    if ((pos = strcopy.find('-')) != string::npos)
    {
      sid = StringUtils::IntVal(strcopy.substr(0, pos));
      strcopy = strcopy.substr(pos + 1);
    }
    else
    {
      sid = StringUtils::IntVal(strcopy);
      strcopy.clear();
    }
  }

  if (!strcopy.empty())
  {
    fields++;
    if ((pos = strcopy.find('-')) != string::npos)
    {
      rid = StringUtils::IntVal(strcopy.substr(0, pos));
      strcopy = strcopy.substr(pos + 1);
    }
    else
    {
      rid = StringUtils::IntVal(strcopy);
      strcopy.clear();
    }
  }

  if (fields == 4 || fields == 5)
  {
    int source = cSource::FromString(sourcebuf);
    if (source >= 0)
      ret = tChannelID(source, nid, tid, sid, rid);
  }

  return ret;
}

void tChannelID::ClrPolarization()
{
  while (m_tid > 100000)
    m_tid -= 100000;
}
