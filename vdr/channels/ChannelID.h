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
#pragma once

#include <string>

struct tChannelID
{
public:
  tChannelID();
  tChannelID(int source, int nid, int tid, int sid, int rid = 0);

  bool operator==(const tChannelID &arg) const;

  bool Valid() const { return (m_nid || m_tid) && m_sid; } // rid is optional and source may be 0//XXX source may not be 0???

  //tChannelID &ClrRid() { m_rid = 0; return *this; }
  void ClrRid() { m_rid = 0; }

  //tChannelID &ClrPolarization();
  void ClrPolarization();

  int Source() const { return m_source; }
  int Nid()    const { return m_nid; }
  int Tid()    const { return m_tid; }
  int Sid()    const { return m_sid; }
  int Rid()    const { return m_rid; }

  std::string Serialize() const;
  static tChannelID Deserialize(const std::string &str);

  static const tChannelID InvalidID;

private:
  int m_source;
  int m_nid; ///< actually the "original" network id
  int m_tid;
  int m_sid;
  int m_rid;
};
