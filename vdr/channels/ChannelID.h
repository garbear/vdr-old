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

#include <stdint.h>
#include <string>

class TiXmlNode;

namespace VDR
{

#define ATSC_SOURCE_ID_NONE (-1)

class cChannelID
{
public:
  cChannelID(uint16_t nid          = 0,
             uint16_t tsid         = 0,
             uint16_t sid          = 0,
             int32_t  atscSourceId = ATSC_SOURCE_ID_NONE);

  bool operator==(const cChannelID &rhs) const;
  bool operator!=(const cChannelID &rhs) const { return !(*this == rhs); }
  bool operator<(const cChannelID& rhs) const;

  /*!
   * Checks the validity of this channel ID. A channel ID is valid if it has
   * an SID (non-zero) and either a TID or an NID.
   */
  bool IsValid(void) const;

  uint16_t Nid()  const { return m_nid; }
  uint16_t Tsid() const { return m_tsid; }
  uint16_t Sid()  const { return m_sid; }
  int32_t  ATSCSourceId(void) const { return m_atscSourceId; }

  void SetID(uint16_t nid, uint16_t tsid, uint16_t sid);
  void SetAtscSourceID(uint16_t sourceId);

  bool Serialise(TiXmlNode* node) const;
  bool Deserialise(const TiXmlNode* node);

  uint32_t Hash(void) const;

  /*!
   * Convert this channel ID to a string. Guaranteed to be a valid filename
   * (no spaces or slashes) or empty if invalid.
   */
  std::string ToString(void) const;

  static const cChannelID InvalidID;

private:
  uint16_t        m_nid;  // Actually the "original" network ID
  uint16_t        m_tsid; // Transport stream ID
  uint16_t        m_sid;  // Service ID
  int32_t         m_atscSourceId; // Source ID for ATSC
};

}
