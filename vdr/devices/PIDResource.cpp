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

#include "PIDResource.h"
#include "dvb/PsiBuffer.h"

namespace VDR
{

cPidResource::cPidResource(uint16_t pid)
 : m_pid(pid),
   m_tid(0xFF),
   m_buffer(cPsiBuffers::Get().Allocate(this))
{
}

cPidResource::cPidResource(uint16_t pid, uint8_t tid)
 : m_pid(pid),
   m_tid(tid),
   m_buffer(cPsiBuffers::Get().Allocate(this))
{
}

cPidResource::~cPidResource(void)
{
  cPsiBuffers::Get().Release(m_buffer);
}

cPsiBuffer* cPidResource::Buffer(void) const
{
  PLATFORM::CLockObject lock(m_mutex);
  return m_buffer;
}

cPsiBuffer* cPidResource::AllocateBuffer(void)
{
  PLATFORM::CLockObject lock(m_mutex);
  if (!m_buffer)
    m_buffer = cPsiBuffers::Get().Allocate(this);
  return m_buffer;
}

}
