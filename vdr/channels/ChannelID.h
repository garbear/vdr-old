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
#pragma once

#include "ChannelSource.h"

#include <stdint.h>
#include <string>

namespace VDR
{

class cChannelID
{
public:
  cChannelID(cChannelSource  source = SOURCE_TYPE_NONE,
             uint16_t        nid    = 0,
             uint16_t        tsid   = 0,
             uint16_t        sid    = 0);

  bool operator==(const cChannelID &rhs) const;
  bool operator!=(const cChannelID &rhs) const { return !(*this == rhs); }

  /*!
   * Checks the validity of this channel ID. A channel ID is valid if it has
   * a source (source != SOURCE_TYPE_NONE), an SID (non-zero), and either a
   * TID or an NID. RID is optional and not considered here.
   */
  bool IsValid(void) const;

  const cChannelSource& Source() const { return m_source; }
  uint16_t              Nid()    const { return m_nid; }
  uint16_t              Tsid()   const { return m_tsid; }
  uint16_t              Sid()    const { return m_sid; }

  //void SetSource(cChannelSource source) { m_source = source; } // TODO: Uncomment when needed
  void SetID(uint16_t nid, uint16_t tsid, uint16_t sid);

  bool Serialise(TiXmlNode* node) const;
  bool Deserialise(const TiXmlNode* node);

  uint32_t Hash(void) const;

  /*!
   * Convert this channel ID to a string. Guaranteed to be a valid filename
   * (no spaces or slashes) or empty if invalid.
   */
  std::string ToString(void) const;

  static const cChannelID InvalidID;

public:// TODO
  cChannelSource  m_source;
private:
  uint16_t        m_nid;  // Actually the "original" network ID
  uint16_t        m_tsid; // Transport stream ID
  uint16_t        m_sid;  // Service ID
};

}
