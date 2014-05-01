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

#include <shared_ptr/shared_ptr.hpp>
#include <stdint.h>

namespace VDR
{

/*!
 * A system maintains resources for all sections being read. Implementations
 * should override this class to track implementation-specific resources (like
 * POSIX file descriptors).
 */
class cFilterResource
{
public:
  cFilterResource(uint16_t pid, uint8_t tid, uint8_t mask);
  virtual ~cFilterResource() { }

  /*!
   * A resource equals another resource if their pid, tid and mask match.
   */
  bool operator==(const cFilterResource& rhs) const;

  /*!
   * A resource matches another resource by comparing masked tids.
   */
  bool Matches(const cFilterResource& rhs) const;

  uint16_t GetPid()  const { return m_pid; }
  uint8_t  GetTid()  const { return m_tid; }
  uint8_t  GetMask() const { return m_mask; }

private:
  uint16_t m_pid;  // Packet ID
  uint8_t  m_tid;  // Table ID
  uint8_t  m_mask; // Mask defining which bits of tid will be used to find matching
                   // sections. E.g. a tid of 0x4E and mask of 0xFE will match
                   // table ids 0x4E and 0x4F.
};

}
