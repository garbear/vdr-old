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

#include "Types.h"
#include "channels/Channel.h"

#include <sys/types.h>

namespace VDR
{
class cFilterData
{
public:
  cFilterData(void);
  cFilterData(u_short pid, u_char tid, u_char mask, bool bSticky);

  u_short Pid() const { return m_pid; }
  u_char Tid() const { return m_tid; }
  u_char Mask() const { return m_mask; }
  bool Sticky() const { return m_bSticky; }

  bool operator==(const cFilterData& rhs) const;
  bool Equals(u_short pid, u_char tid, u_char mask) const;
  bool Matches(u_short pid, u_char tid) const;

private:
  u_short m_pid;
  u_char  m_tid;
  u_char  m_mask;
  bool    m_bSticky;
};

}
