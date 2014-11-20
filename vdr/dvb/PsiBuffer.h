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
#include <vector>

#define PSI_MAX_SIZE    (4096)

namespace VDR
{
  class cPidResource;

  class cPsiBuffer
  {
  public:
    cPsiBuffer(unsigned int position);
    virtual ~cPsiBuffer(void) {}

    bool AddTsData(const uint8_t* data, size_t len, const uint8_t** outdata, size_t* outlen);
    void Reset(void);
    size_t Size(void) const { return m_cursize; }

    uint8_t* Data(void) { return m_data; }
    const uint8_t* Data(void) const { return m_data; }
    size_t Position(void) const { return m_position; }
    void SetResource(cPidResource* resource) { m_resource = resource; }
    cPidResource* Resource(void) const { return m_resource; }
    void IncUsed(void) { ++m_used; }
    void DecUsed(void) { --m_used; }
    bool Used(void) const { return m_used != 0; }

  private:
    uint8_t  m_data[PSI_MAX_SIZE];
    size_t   m_cursize;
    size_t   m_sectionSize;
    bool     m_start;
    bool     m_copy;
    const size_t m_position;
    cPidResource* m_resource;
    unsigned int m_used;
  };

  class cPsiBuffers
  {
  public:
    static cPsiBuffers& Get(void);
    virtual ~cPsiBuffers(void);

    cPsiBuffer* Allocate(cPidResource* resource);
    void Release(cPsiBuffer* buffer);

  private:
    void EnsureSize(size_t size);

    cPsiBuffers(void);
    std::vector<cPsiBuffer*>   m_buffers;
    std::map<cPidResource*, size_t> m_resourceMap;
    PLATFORM::CMutex           m_mutex;
  };
}
