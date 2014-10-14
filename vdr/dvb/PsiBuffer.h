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

#include <platform/threads/mutex.h>
#include <map>

#define PSI_MAX_SIZE    (4096)
#define PSI_MAX_BUFFERS (40)

namespace VDR
{
  class cPsiBuffer
  {
  public:
    cPsiBuffer(void);
    virtual ~cPsiBuffer(void) {}

    bool AddTsData(const uint8_t* data, size_t len, const uint8_t** outdata, size_t* outlen);
    void Reset(void);
    size_t Size(void) const { return m_cursize; }

    void SetPosition(size_t position) { m_position = position; }
    size_t Position(void) const { return m_position; }
    void SetPid(uint16_t pid) { m_pid = pid; }
    uint16_t Pid(void) const { return m_pid; }

  private:
    uint8_t  m_data[PSI_MAX_SIZE];
    size_t   m_cursize;
    size_t   m_sectionSize;
    bool     m_start;
    bool     m_copy;
    size_t   m_position;
    uint16_t m_pid;
  };

  class cPsiBuffers
  {
  public:
    static cPsiBuffers& Get(void);
    virtual ~cPsiBuffers(void) {}

    cPsiBuffer* Allocate(uint16_t pid);
    void Release(cPsiBuffer* buffer);

  private:
    cPsiBuffers(void);
    cPsiBuffer                 m_buffers[PSI_MAX_BUFFERS];
    unsigned int               m_used[PSI_MAX_BUFFERS];
    std::map<uint16_t, size_t> m_pidMap;
    PLATFORM::CMutex           m_mutex;
  };
}
