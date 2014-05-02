/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#include "CADescriptor.h"

#include <libsi/si.h>

using namespace std;

namespace VDR
{

cCaDescriptor::cCaDescriptor(uint16_t caSystem)
 : m_caSystem(caSystem),
   m_esPid(0)
{
  vector<uint8_t> privateData;
  SetData(caSystem, 0, privateData);
}

cCaDescriptor::cCaDescriptor(uint16_t caSystem, uint16_t caPid, uint16_t esPid, const vector<uint8_t>& privateData)
 : m_caSystem(caSystem),
   m_esPid(esPid)
{
  SetData(caSystem, caPid, privateData);
}

cCaDescriptor::cCaDescriptor(const SI::CaDescriptor& caDescriptor, uint16_t esPid)
: m_caSystem(caDescriptor.getCaType()),
  m_esPid(esPid)
{
  vector<uint8_t> privateData;
  privateData.assign(caDescriptor.privateData.getData(),
                     caDescriptor.privateData.getData() + caDescriptor.privateData.getLength());
  SetData(caDescriptor.getCaType(), caDescriptor.getCaPid(), privateData);
}

void cCaDescriptor::SetData(uint16_t caSystem, uint16_t caPid, const vector<uint8_t>& privateData)
{
  m_data.reserve(privateData.size() + 6);
  m_data[0] = SI::CaDescriptorTag;
  m_data[1] = privateData.size() + 4;
  m_data[2] = (caSystem >> 8) & 0xFF;
  m_data[3] =  caSystem       & 0xFF;
  m_data[4] = ((caPid   >> 8) & 0x1F) | 0xE0;
  m_data[5] =   caPid         & 0xFF;
  m_data.insert(m_data.begin() + 6, privateData.begin(), privateData.end());
}

bool cCaDescriptor::operator==(const cCaDescriptor &arg) const
{
  return m_esPid == arg.m_esPid && m_data == arg.m_data;
}

}
