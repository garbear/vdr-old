/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#define PSI_MAX_SIZE (4096)

namespace VDR
{
  class cPsiBuffer
  {
  public:
    cPsiBuffer(void);
    virtual ~cPsiBuffer(void) {}

    const uint8_t* AddTsData(const uint8_t* data, size_t len);
    void Reset(void);
    size_t Size(void) const { return m_cursize; }

  private:
    uint8_t m_data[PSI_MAX_SIZE];
    size_t  m_cursize;
    size_t  m_sectionSize;
    bool    m_start;
    bool    m_copy;
  };
}
