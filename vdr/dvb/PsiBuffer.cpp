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

namespace VDR
{

cPsiBuffer::cPsiBuffer(void) :
    m_cursize(0),
    m_sectionSize(0),
    m_start(false),
    m_copy(true)
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

const uint8_t* cPsiBuffer::AddTsData(const uint8_t* data, size_t len)
{
  bool bnew(false);
  if (TsError(data))
  {
    /** skip invalid data */
    Reset();
    return NULL;
  }

  if (!TsHasPayload(data))
  {
    /** need payload */
    return NULL;
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
    return NULL;
  }

  int payloadoffset = TsPayloadOffset(data);
  const uint8_t* payload = data + payloadoffset;
  size_t payloadlen = len - payloadoffset;
  if (m_cursize + payloadlen > PSI_MAX_SIZE)
    payloadlen -= PSI_MAX_SIZE - m_cursize;
  if (bnew)
    m_sectionSize = SI::Section::getLength(payload + 1);

  if (!m_copy && payloadlen >= m_sectionSize)
  {
    m_cursize = payloadlen;
    return payload;
  }

  m_copy = true;
  memcpy(m_data + m_cursize, payload, payloadlen);
  m_cursize += payloadlen;

  return m_cursize >= m_sectionSize ? m_data : NULL;
}


}
