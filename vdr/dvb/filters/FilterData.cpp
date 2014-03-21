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

#include "FilterData.h"

namespace VDR
{

cFilterData::cFilterData(void)
 : m_pid(0),
   m_tid(0),
   m_mask(0),
   m_bSticky(false)
{
}

cFilterData::cFilterData(u_short pid, u_char tid, u_char mask, bool bSticky)
: m_pid(pid),
  m_tid(tid),
  m_mask(mask),
  m_bSticky(bSticky)
{
}

bool cFilterData::operator==(const cFilterData& rhs) const
{
  return Equals(rhs.m_pid, rhs.m_tid, rhs.m_mask);
}

bool cFilterData::Equals(u_short pid, u_char tid, u_char mask) const
{
  return m_pid == pid && m_tid == tid && m_mask == mask;
}

bool cFilterData::Matches(u_short pid, u_char tid) const
{
  return m_pid == pid && m_tid == (tid & m_mask);
}

}
