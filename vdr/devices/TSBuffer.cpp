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

#include "TSBuffer.h"
#include "filesystem/Poller.h"
#include "utils/CommonMacros.h"
#include "utils/Ringbuffer.h"
#include "Types.h"

namespace VDR
{

// Copied from Remux.h (TODO)
#define TS_SYNC_BYTE          0x47
#define TS_SIZE               188

cTSBuffer::cTSBuffer(int file, unsigned int size, int cardIndex)
 : m_file(file),
   m_cardIndex(cardIndex),
   m_bDelivered(false)
{
  m_ringBuffer = new cRingBufferLinear(size, TS_SIZE, true, "TS");
  m_ringBuffer->SetTimeouts(100, 100);
  m_ringBuffer->SetIoThrottle();

  CreateThread();
}

cTSBuffer::~cTSBuffer()
{
  StopThread(3000);
  delete m_ringBuffer;
}

void *cTSBuffer::Process()
{
  dsyslog("created TS buffer on device %d", m_cardIndex);

  if (m_ringBuffer)
  {
    bool firstRead = true;
    cPoller Poller(m_file);
    while (!IsStopped())
    {
      if (firstRead || Poller.Poll(100))
      {
        firstRead = false;
        int r = m_ringBuffer->Read(m_file);
        if (r < 0 && FATALERRNO)
        {
          if (errno == EOVERFLOW)
            esyslog("ERROR: driver buffer overflow on device %d", m_cardIndex);
          else
          {
            //LOG_ERROR;
            break;
          }
        }
      }
    }
  }
  return NULL;
}

uint8_t* cTSBuffer::Get()
{
  int Count = 0;
  if (m_bDelivered)
  {
    m_ringBuffer->Del(TS_SIZE);
    m_bDelivered = false;
  }
  uint8_t *p = m_ringBuffer->Get(Count);
  if (p && Count >= TS_SIZE)
  {
    if (*p != TS_SYNC_BYTE)
    {
      for (int i = 1; i < Count; i++)
      {
        if (p[i] == TS_SYNC_BYTE)
        {
          Count = i;
          break;
        }
      }
      m_ringBuffer->Del(Count);
      esyslog("ERROR: skipped %d bytes to sync on TS packet on device %d", Count, m_cardIndex);
      return NULL;
    }
    m_bDelivered = true;
    return p;
  }
  return NULL;
}

}
