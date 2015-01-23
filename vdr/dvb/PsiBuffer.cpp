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

#include <libsi/si.h>
#include <stddef.h>
#include "PsiBuffer.h"
#include "devices/Remux.h"
#include "utils/log/Log.h"

namespace VDR
{

cPsiBuffer::cPsiBuffer(unsigned int position) :
    m_cursize(0),
    m_sectionSize(0),
    m_start(false),
    m_copy(true),
    m_position(position),
    m_resource(NULL),
    m_used(0)
{
  Reset();
}

void cPsiBuffer::Reset(void)
{
  if (m_copy)
    memset(m_data, 0, sizeof(m_data));
  m_cursize     = 0;
  m_sectionSize = 0;
  m_start       = false;
  m_copy        = false;
}

bool cPsiBuffer::AddTsData(const uint8_t* data, size_t len, const uint8_t** outdata, size_t* outlen)
{
  bool bnew(false);
  if (TsError(data))
  {
    /** skip invalid data */
    Reset();
    return false;
  }

  if (!TsHasPayload(data))
  {
    /** need payload */
    return false;
  }

  if (TsPayloadStart(data))
  {
    /** new data */
    Reset();
    bnew = true;
  }
  else if (!m_sectionSize)
  {
    /** need section start */
    return false;
  }

  int payloadoffset = TsPayloadOffset(data);
  const uint8_t* payload = data + payloadoffset;
  size_t payloadlen = len - payloadoffset;
  while (m_cursize + payloadlen > PSI_MAX_SIZE)
    payloadlen -= PSI_MAX_SIZE - m_cursize;
  if (bnew)
    m_sectionSize = SI::Section::getLength(payload + 1);

  if (len != TS_SIZE)
    m_copy = true;

  if (!m_copy && payloadlen >= m_sectionSize)
  {
    m_cursize = payloadlen;
    *outdata = payload;
    *outlen  = payloadlen;
    return true;
  }

  m_copy = true;
  memcpy(m_data + m_cursize, payload, payloadlen);
  m_cursize += payloadlen;

  if (m_cursize >= m_sectionSize)
  {
    *outdata = m_data;
    *outlen  = m_cursize;
    m_cursize = 0;
    return true;
  }
  return false;
}

cPsiBuffers::cPsiBuffers(void)
{
}

cPsiBuffers::~cPsiBuffers(void)
{
  for (std::vector<cPsiBuffer*>::iterator it = m_buffers.begin(); it != m_buffers.end(); ++it)
    delete *it;
}

cPsiBuffers& cPsiBuffers::Get(void)
{
  static cPsiBuffers _instance;
  return _instance;
}

cPsiBuffer* cPsiBuffers::Allocate(cPidResource* resource)
{
  PLATFORM::CLockObject lock(m_mutex);
  std::map<cPidResource*, size_t>::iterator it = m_resourceMap.find(resource);
  if (it != m_resourceMap.end())
  {
    m_buffers[it->second]->IncUsed();
    return m_buffers[it->second];
  }

  size_t ptr = 0;
  while (ptr < m_buffers.size() && m_buffers[ptr]->Used())
    ++ptr;

  EnsureSize(ptr + 1);
  m_buffers[ptr]->IncUsed();
  m_buffers[ptr]->SetResource(resource);
  m_resourceMap[resource] = ptr;
  return m_buffers[ptr];
}

void cPsiBuffers::Release(cPsiBuffer* buffer)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (buffer->Position() >= 0 && buffer->Position() < m_buffers.size())
  {
    if (m_buffers[buffer->Position()]->Used())
      m_buffers[buffer->Position()]->DecUsed();

    if (!m_buffers[buffer->Position()]->Used())
    {
      m_resourceMap.erase(buffer->Resource());
      buffer->Reset();
    }
  }
}

void cPsiBuffers::EnsureSize(size_t size)
{
  if (size > m_buffers.size())
  {
    const size_t oldSize = m_buffers.size();
    m_buffers.resize(size);
    for (size_t ptr = oldSize; ptr < m_buffers.size(); ++ptr)
      m_buffers[ptr] = new cPsiBuffer(ptr);
  }
}

}
