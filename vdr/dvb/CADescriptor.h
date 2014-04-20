/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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
#pragma once

#include "Types.h"

#include <stdint.h>
#include <vector>

namespace VDR
{
class cCaDescriptor
{
public:
  cCaDescriptor(uint16_t caSystem, uint16_t caPid, uint16_t esPid, const std::vector<uint8_t>& privateData);
  virtual ~cCaDescriptor(void) { }

  bool operator==(const cCaDescriptor& arg) const;

  uint16_t CaSystem(void) const { return m_caSystem; }
  uint16_t EsPid(void) const    { return m_esPid; }

  const std::vector<uint8_t>& Data(void) const { return m_data; }

private:
  uint16_t             m_caSystem; // Conditional access type
  uint16_t             m_esPid;    // Elementary stream packet ID, or 0 if not stream-specific
  std::vector<uint8_t> m_data;
};

typedef VDR::shared_ptr<cCaDescriptor> CaDescriptorPtr;
typedef std::vector<CaDescriptorPtr>   CaDescriptorVector;
}
