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

tChannelID tChannelID::Deserialize(const std::string &str)
{
  tChannelID ret = tChannelID::InvalidID;
  char *sourcebuf = NULL;
  int nid;
  int tid;
  int sid;
  int rid = 0;
  int fields = sscanf(str.c_str(), "%a[^-]-%d-%d-%d-%d", &sourcebuf, &nid, &tid, &sid, &rid);

  if (fields == 4 || fields == 5)
  {
    int source = cSource::FromString(sourcebuf);
    if (source >= 0)
      ret = tChannelID(source, nid, tid, sid, rid);
  }

  free(sourcebuf);
  return ret;
}

cString tChannelID::Serialize() const
{
  char buffer[256];
  if (m_rid)
    snprintf(buffer, sizeof(buffer), "%s-%d-%d-%d-%d", cSource::Serialize(m_source).c_str(), m_nid, m_tid, m_sid, m_rid);
  else
    snprintf(buffer, sizeof(buffer), "%s-%d-%d-%d", cSource::Serialize(m_source).c_str(), m_nid, m_tid, m_sid);
  return buffer;
}

//tChannelID &tChannelID::ClrPolarization()
void tChannelID::ClrPolarization()
{
  while (tid > 100000)
    tid -= 100000;
  //return *this;
}
