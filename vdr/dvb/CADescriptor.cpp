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

#include "CADescriptor.h"

#include <libsi/si.h>

cCaDescriptor::cCaDescriptor(int caSystem, int caPid, int esPid, int length, const uint8_t* data)
 : m_caSystem(caSystem),
   m_esPid(esPid)
{
  m_data.reserve(length + 6);
  m_data[0] = SI::CaDescriptorTag;
  m_data[1] = length + 4;
  m_data[2] = (m_caSystem >> 8) & 0xFF;
  m_data[3] =  m_caSystem       & 0xFF;
  m_data[4] = ((caPid   >> 8) & 0x1F) | 0xE0;
  m_data[5] =   caPid         & 0xFF;
  m_data.insert(m_data.begin() + 6, data, data + length);
}

bool cCaDescriptor::operator== (const cCaDescriptor &arg) const
{
  return m_esPid == arg.m_esPid && m_data == arg.m_data;
}
