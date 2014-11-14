/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2007 Chris Tallon
 *      Portions Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Portions Copyright (C) 2010, 2011 Alexander Pipelka
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

#include "VideoBuffer.h"
#include "RecPlayer.h"
#include "devices/Remux.h"
#include "filesystem/Videodir.h"
#include "lib/platform/threads/mutex.h"
#include "recordings/Recording.h"
#include "settings/Settings.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"
#include "utils/Ringbuffer.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace PLATFORM;

namespace VDR
{

class cVideoBufferSimple : public cVideoBuffer
{
friend class cVideoBuffer;
public:
  virtual void Receive(const uint16_t pid, const uint8_t* data, const size_t len);
  virtual int ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime);

protected:
  cVideoBufferSimple();
  virtual ~cVideoBufferSimple();
  cRingBufferLinear *m_Buffer;
  int m_BytesConsumed;
};

cVideoBufferSimple::cVideoBufferSimple()
{
  m_Buffer = new cRingBufferLinear(MEGABYTE(3), TS_SIZE * 2, false);
  m_Buffer->SetTimeouts(0, 100);
  m_BytesConsumed = 0;
}

cVideoBufferSimple::~cVideoBufferSimple()
{
  if (m_Buffer)
    delete m_Buffer;
}

void cVideoBufferSimple::Receive(const uint16_t pid, const uint8_t* data, const size_t len)
{
  m_Buffer->Put(data, len);
}

int cVideoBufferSimple::ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime)
{
  int  readBytes;
  if (m_BytesConsumed)
  {
    m_Buffer->Del(m_BytesConsumed);
  }
  m_BytesConsumed = 0;
  *buf = m_Buffer->Get(readBytes);
  if (!(*buf) || readBytes < TS_SIZE)
  {
    usleep(100);
    return 0;
  }
  /* Make sure we are looking at a TS packet */
  while (readBytes > TS_SIZE)
  {
    if ((*buf)[0] == TS_SYNC_BYTE && (*buf)[TS_SIZE] == TS_SYNC_BYTE)
      break;
    m_BytesConsumed++;
    (*buf)++;
    readBytes--;
  }

  if ((*buf)[0] != TS_SYNC_BYTE)
  {
    m_Buffer->Del(m_BytesConsumed);
    m_BytesConsumed = 0;
    return 0;
  }

  m_BytesConsumed += TS_SIZE;
  endTime = 0;
  wrapTime = 0;
  return TS_SIZE;
}

//-----------------------------------------------------------------------------

#define MARGIN 40000

class cVideoBufferTimeshift : public cVideoBuffer
{
friend class cVideoBuffer;
public:
  virtual off_t GetPosMin();
  virtual off_t GetPosMax();
  virtual off_t GetPosCur();
  virtual void GetPositions(off_t *cur, off_t *min, off_t *max);
  virtual bool HasBuffer() { return true; };

protected:
  cVideoBufferTimeshift();
  virtual bool Init() = 0;
  virtual off_t Available();
  off_t m_BufferSize;
  off_t m_WritePtr;
  off_t m_ReadPtr;
  bool m_BufferFull;
  unsigned int m_Margin;
  unsigned int m_BytesConsumed;
  CMutex m_Mutex;
};

cVideoBufferTimeshift::cVideoBufferTimeshift()
{
  m_Margin = TS_SIZE*2;
  m_BufferFull = false;
  m_ReadPtr = 0;
  m_WritePtr = 0;
  m_BytesConsumed = 0;
  m_BufferSize = 0;
}

off_t cVideoBufferTimeshift::GetPosMin()
{
  off_t ret;
  if (!m_BufferFull)
    return 0;

  ret = m_WritePtr + MARGIN * 2;
  if (ret >= m_BufferSize)
    ret -= m_BufferSize;

  return ret;
}

off_t cVideoBufferTimeshift::GetPosMax()
{
   off_t ret = m_WritePtr;
   if (ret < GetPosMin())
     ret += m_BufferSize;
   return ret;
}

off_t cVideoBufferTimeshift::GetPosCur()
{
  off_t ret = m_ReadPtr;
  if (ret < GetPosMin())
    ret += m_BufferSize;
  return ret;
}

void cVideoBufferTimeshift::GetPositions(off_t *cur, off_t *min, off_t *max)
{
  CLockObject lock(m_Mutex);

  *cur = GetPosCur();
  *min = GetPosMin();
  *min = (*min > *cur) ? *cur : *min;
  *max = GetPosMax();
}

off_t cVideoBufferTimeshift::Available()
{
  CLockObject lock(m_Mutex);

  off_t ret;
  if (m_ReadPtr <= m_WritePtr)
    ret = m_WritePtr - m_ReadPtr;
  else
    ret = m_BufferSize - (m_ReadPtr - m_WritePtr);

  return ret;
}
//-----------------------------------------------------------------------------

class cVideoBufferRAM : public cVideoBufferTimeshift
{
friend class cVideoBuffer;
public:
  virtual void Receive(const uint16_t pid, const uint8_t* data, const size_t len);
  virtual int ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime);
  virtual void SetPos(off_t pos);

protected:
  cVideoBufferRAM();
  virtual ~cVideoBufferRAM();
  virtual bool Init();
  uint8_t *m_Buffer;
  uint8_t *m_BufferPtr;
};

cVideoBufferRAM::cVideoBufferRAM()
{
  m_Buffer = 0;
  m_BufferPtr = 0;
}

cVideoBufferRAM::~cVideoBufferRAM()
{
  if (m_Buffer)
    free(m_Buffer);
}

bool cVideoBufferRAM::Init()
{
  m_BufferSize = (off_t)cSettings::Get().m_TimeshiftBufferSize*100*1000*1000;
  isyslog("allocated timeshift buffer with size: %ld", m_BufferSize);
  m_Buffer = (uint8_t*)malloc(m_BufferSize + m_Margin);
  m_BufferPtr = m_Buffer + m_Margin;
  if (!m_Buffer)
    return false;
  else
    return true;
}

void cVideoBufferRAM::SetPos(off_t pos)
{
  CLockObject lock(m_Mutex);

  m_ReadPtr = pos;
  if (m_ReadPtr >= m_BufferSize)
    m_ReadPtr -= m_BufferSize;
  m_BytesConsumed = 0;
}

void cVideoBufferRAM::Receive(const uint16_t pid, const uint8_t* data, const size_t len)
{
  const uint8_t* buf  = data;
  size_t         size = len;

  if (Available() + MARGIN >= m_BufferSize)
  {
    return;
  }

  if ((m_BufferSize - m_WritePtr) <= size)
  {
    int bytes = m_BufferSize - m_WritePtr;
    memcpy(m_BufferPtr+m_WritePtr, buf, bytes);
    size -= bytes;
    buf += bytes;
    CLockObject lock(m_Mutex);
    m_WritePtr = 0;
  }

  memcpy(m_BufferPtr+m_WritePtr, buf, size);

  CLockObject lock(m_Mutex);

  m_WritePtr += size;
  if (!m_BufferFull)
  {
    if ((m_WritePtr + 2*MARGIN) > m_BufferSize)
    {
      m_BufferFull = true;
      time(&m_bufferWrapTime);
    }
  }

  time(&m_bufferEndTime);
}

int cVideoBufferRAM::ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime)
{
  // move read pointer
  if (m_BytesConsumed)
  {
    CLockObject lock(m_Mutex);
    m_ReadPtr += m_BytesConsumed;
    if (m_ReadPtr >= m_BufferSize)
      m_ReadPtr -= m_BufferSize;

    endTime = m_bufferEndTime;
    wrapTime = m_bufferWrapTime;
  }
  m_BytesConsumed = 0;

  // check if we have anything to read
  off_t readBytes = Available();
  if (readBytes < m_Margin)
  {
    return 0;
  }

  // if we are close to end, copy margin to front
  if (m_ReadPtr > (m_BufferSize - m_Margin))
  {
    int bytesToCopy = m_BufferSize - m_ReadPtr;
    memmove(m_Buffer + (m_Margin - bytesToCopy), m_Buffer + m_ReadPtr, bytesToCopy);
    *buf = m_Buffer + (m_Margin - bytesToCopy);
  }
  else
    *buf = m_BufferPtr + m_ReadPtr;

  // Make sure we are looking at a TS packet
  while (readBytes > TS_SIZE)
  {
    if ((*buf)[0] == TS_SYNC_BYTE && (*buf)[TS_SIZE] == TS_SYNC_BYTE)
      break;
    m_BytesConsumed++;
    (*buf)++;
    readBytes--;
  }

  if ((*buf)[0] != TS_SYNC_BYTE)
  {
    return 0;
  }

  m_BytesConsumed += TS_SIZE;
  return TS_SIZE;
}

//-----------------------------------------------------------------------------

class cVideoBufferFile : public cVideoBufferTimeshift
{
friend class cVideoBuffer;
public:
  virtual off_t GetPosMax();
  virtual void Receive(const uint16_t pid, const uint8_t* data, const size_t len);
  virtual int ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime);
  virtual void SetPos(off_t pos);

protected:
  cVideoBufferFile();
  cVideoBufferFile(int clientID);
  virtual ~cVideoBufferFile();
  virtual bool Init();
  virtual int ReadBytes(uint8_t *buf, off_t pos, unsigned int size);
  int m_ClientID;
  std::string m_Filename;
  CFile m_file;
  uint8_t *m_ReadCache;
  unsigned int m_ReadCachePtr;
  unsigned int m_ReadCacheSize;
  unsigned int m_ReadCacheMaxSize;
};

cVideoBufferFile::cVideoBufferFile()
{
  m_ReadCache = NULL;
  m_ReadCachePtr = 0;
  m_ReadCacheSize = 0;
  m_ReadCacheMaxSize = 0;
  m_ClientID = 0;
}

cVideoBufferFile::cVideoBufferFile(int clientID)
{
  m_ClientID = clientID;
  m_ReadCacheSize = 0;
  m_ReadCache = NULL;
  m_ReadCachePtr = 0;
  m_ReadCacheMaxSize = 0;
}

cVideoBufferFile::~cVideoBufferFile()
{
  m_file.Close();
  CFile::Delete(m_Filename);
  free(m_ReadCache);
}

bool cVideoBufferFile::Init()
{
  m_ReadCache = 0;
  m_ReadCacheMaxSize = 32000;

  m_ReadCache = (uint8_t*)malloc(m_ReadCacheMaxSize);
  if (!m_ReadCache)
    return false;

  m_BufferSize = (off_t)cSettings::Get().m_TimeshiftBufferFileSize*1000*1000*1000;

  std::string strTimeshiftBufferDir = cSettings::Get().m_TimeshiftBufferDir;
  if (!strTimeshiftBufferDir.empty() && CDirectory::Exists(strTimeshiftBufferDir.c_str()))
  {
    if (StringUtils::EndsWith(strTimeshiftBufferDir, "/"))
      m_Filename = StringUtils::Format("%sTimeshift-%d.vnsi", strTimeshiftBufferDir.c_str(), m_ClientID);
    else
      m_Filename = StringUtils::Format("%s/Timeshift-%d.vnsi", strTimeshiftBufferDir.c_str(), m_ClientID);
  }
  else
  {
    m_Filename = m_Filename = StringUtils::Format("%s/Timeshift-%d.vnsi", VideoDirectory, m_ClientID);
  }

  if (m_file.OpenForWrite(m_Filename, true))
  {
    esyslog("Could not open file: %s", m_Filename.c_str());
    return false;
  }
  m_WritePtr = m_file.Seek(m_BufferSize - 1, SEEK_SET);
  if (m_WritePtr == -1)
  {
    esyslog("(Init) Could not seek file: %s", m_Filename.c_str());
    return false;
  }
  char tmp = '0';
  if (m_file.Write(&tmp, 1) < 0)
  {
    esyslog("(Init) Could not write to file: %s", m_Filename.c_str());
    return false;
  }

  m_WritePtr = 0;
  m_ReadPtr = 0;
  m_ReadCacheSize = 0;
  return true;
}

void cVideoBufferFile::SetPos(off_t pos)
{
  CLockObject lock(m_Mutex);

  m_ReadPtr = pos;
  if (m_ReadPtr >= m_BufferSize)
    m_ReadPtr -= m_BufferSize;
  m_BytesConsumed = 0;
  m_ReadCacheSize = 0;
}

off_t cVideoBufferFile::GetPosMax()
{
  off_t posMax = cVideoBufferTimeshift::GetPosMax();
  if (posMax >= m_ReadCacheMaxSize)
    posMax -= m_ReadCacheMaxSize;
  else
    posMax = 0;
  return posMax;
}

void cVideoBufferFile::Receive(const uint16_t pid, const uint8_t* data, const size_t len)
{
  const uint8_t* buf  = data;
  size_t         size = len;

  if (Available() + MARGIN >= m_BufferSize)
  {
    return;
  }

  if ((m_BufferSize - m_WritePtr) <= size)
  {
    int bytes = m_BufferSize - m_WritePtr;

    int p = 0;
    off_t ptr = m_WritePtr;
    while(bytes > 0)
    {
      m_file.Seek(ptr, SEEK_SET);
      p = m_file.Write(buf, bytes);
      if (p < 0)
      {
        esyslog("Could not write to file: %s", m_Filename.c_str());
        return;
      }
      size -= p;
      bytes -= p;
      buf += p;
      ptr += p;
    }
    CLockObject lock(m_Mutex);
    m_WritePtr = 0;
  }

  off_t ptr = m_WritePtr;
  int bytes = size;
  int p;
  while(bytes > 0)
  {
    m_file.Seek(ptr, SEEK_SET);
    p = m_file.Write(buf, bytes);
    if (p < 0)
    {
      esyslog("Could not write to file: %s", m_Filename.c_str());
      return;
    }
    bytes -= p;
    buf += p;
    ptr += p;
  }

  CLockObject lock(m_Mutex);

  m_WritePtr += size;
  if (!m_BufferFull)
  {
    if ((m_WritePtr + 2*MARGIN) > m_BufferSize)
    {
      m_BufferFull = true;
      time(&m_bufferWrapTime);
    }
  }

  time(&m_bufferEndTime);
}

int cVideoBufferFile::ReadBytes(uint8_t *buf, off_t pos, unsigned int size)
{
  int p;
  for (;;)
  {
    m_file.Seek(pos, SEEK_SET);
    p = m_file.Read(buf, size);
    if (p < 0 && errno == EINTR)
    {
      continue;
    }
    return p;
  }
  return VIDEOBUFFER_NO_DATA;
}

int cVideoBufferFile::ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime)
{
  // move read pointer
  if (m_BytesConsumed)
  {
    CLockObject lock(m_Mutex);
    m_ReadPtr += m_BytesConsumed;
    if (m_ReadPtr >= m_BufferSize)
      m_ReadPtr -= m_BufferSize;
    m_ReadCachePtr += m_BytesConsumed;

    endTime = m_bufferEndTime;
    wrapTime = m_bufferWrapTime;
  }
  m_BytesConsumed = 0;

  // check if we have anything to read
  off_t readBytes;
  if (m_ReadCacheSize && ((m_ReadCachePtr + m_Margin) <= m_ReadCacheSize))
  {
    readBytes = m_ReadCacheSize - m_ReadCachePtr;
    *buf = m_ReadCache + m_ReadCachePtr;
  }
  else if ((readBytes = Available()) >= m_ReadCacheMaxSize)
  {
    if (m_ReadPtr + m_ReadCacheMaxSize <= m_BufferSize)
    {
      m_ReadCacheSize = ReadBytes(m_ReadCache, m_ReadPtr, m_ReadCacheMaxSize);
      if (m_ReadCacheSize < 0)
      {
        esyslog("Could not read file: %s", m_Filename.c_str());
        return 0;
      }
      if (m_ReadCacheSize < m_Margin)
      {
        esyslog("Could not read file (margin): %s , read: %d", m_Filename.c_str(), m_ReadCacheSize);
        m_ReadCacheSize = 0;
        return 0;
      }
      readBytes = m_ReadCacheSize;
      *buf = m_ReadCache;
      m_ReadCachePtr = 0;
    }
    else
    {
      m_ReadCacheSize = ReadBytes(m_ReadCache, m_ReadPtr, m_BufferSize - m_ReadPtr);
      if ((m_ReadCacheSize < m_Margin) && (m_ReadCacheSize != (m_BufferSize - m_ReadPtr)))
      {
        esyslog("Could not read file (end): %s", m_Filename.c_str());
        m_ReadCacheSize = 0;
        return 0;
      }
      readBytes = ReadBytes(m_ReadCache + m_ReadCacheSize, 0, m_ReadCacheMaxSize - m_ReadCacheSize);
      if (readBytes < 0)
      {
        esyslog("Could not read file (end): %s", m_Filename.c_str());
        m_ReadCacheSize = 0;
        return 0;
      }
      m_ReadCacheSize += readBytes;
      if (m_ReadCacheSize < m_Margin)
      {
        esyslog("Could not read file (margin): %s", m_Filename.c_str());
        m_ReadCacheSize = 0;
        return 0;
      }
      readBytes = m_ReadCacheSize;
      *buf = m_ReadCache;
      m_ReadCachePtr = 0;
    }
  }
  else
    return 0;

  // Make sure we are looking at a TS packet
  while (readBytes > TS_SIZE)
  {
    if ((*buf)[0] == TS_SYNC_BYTE && (*buf)[TS_SIZE] == TS_SYNC_BYTE)
      break;
    m_BytesConsumed++;
    (*buf)++;
    readBytes--;
  }

  if ((*buf)[0] != TS_SYNC_BYTE)
  {
    return 0;
  }

  m_BytesConsumed += TS_SIZE;
  return TS_SIZE;
}

//-----------------------------------------------------------------------------

class cVideoBufferRecording : public cVideoBufferFile
{
friend class cVideoBuffer;
public:
  virtual off_t GetPosMax();
  virtual void Receive(const uint16_t pid, const uint8_t* data, const size_t len);
  virtual int ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime);
  virtual time_t GetRefTime();

protected:
  cVideoBufferRecording(cRecording *rec);
  virtual ~cVideoBufferRecording();
  virtual bool Init();
  virtual off_t Available();
  off_t GetPosEnd();
  cRecPlayer *m_RecPlayer;
  cRecording *m_Recording;
  cTimeMs m_ScanTimer;
};

cVideoBufferRecording::cVideoBufferRecording(cRecording *rec)
{
  m_Recording = rec;
  m_ReadCacheSize = 0;
  m_ReadCache = 0;
  m_RecPlayer = NULL;
}

cVideoBufferRecording::~cVideoBufferRecording()
{
  isyslog("delete cVideoBufferRecording");
  if (m_RecPlayer)
    delete m_RecPlayer;
}

off_t cVideoBufferRecording::GetPosMax()
{
  m_RecPlayer->reScan();
  m_WritePtr = m_RecPlayer->getLengthBytes();
  return cVideoBufferFile::GetPosMax();
}

void cVideoBufferRecording::Receive(const uint16_t pid, const uint8_t* data, const size_t len)
{

}

bool cVideoBufferRecording::Init()
{
  m_ReadCacheMaxSize = 32000;

  m_ReadCache = (uint8_t*)malloc(m_ReadCacheMaxSize);
  if (!m_ReadCache)
    return false;

  m_RecPlayer = new cRecPlayer(m_Recording, true);
  if (!m_RecPlayer)
    return false;

  m_WritePtr = 0;
  m_ReadPtr = 0;
  m_ReadCacheSize = 0;
  m_InputAttached = false;
  m_ScanTimer.Set(0);

  return true;
}

time_t cVideoBufferRecording::GetRefTime()
{
  return m_Recording->Start();
}

off_t cVideoBufferRecording::Available()
{
  if (m_ScanTimer.TimedOut())
  {
    m_RecPlayer->reScan();
    m_ScanTimer.Set(1000);
  }
  m_BufferSize = m_WritePtr = m_RecPlayer->getLengthBytes();
  return cVideoBufferTimeshift::Available();
}

int cVideoBufferRecording::ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime)
{
  // move read pointer
  if (m_BytesConsumed)
  {
    m_ReadPtr += m_BytesConsumed;
    if (m_ReadPtr >= m_BufferSize)
    {
      m_ReadPtr -= m_BufferSize;
      esyslog("cVideoBufferRecording::ReadBlock - unknown error");
    }
    m_ReadCachePtr += m_BytesConsumed;
  }
  m_BytesConsumed = 0;

  // check if we have anything to read
  off_t readBytes;
  if (m_ReadCacheSize && ((m_ReadCachePtr + m_Margin) <= m_ReadCacheSize))
  {
    readBytes = m_ReadCacheSize - m_ReadCachePtr;
    *buf = m_ReadCache + m_ReadCachePtr;
  }
  else if ((readBytes = Available()) >= m_ReadCacheMaxSize)
  {
    if (m_ReadPtr + m_ReadCacheMaxSize <= m_BufferSize)
    {
      m_ReadCacheSize = m_RecPlayer->getBlock(m_ReadCache, m_ReadPtr, m_ReadCacheMaxSize);
      if (m_ReadCacheSize < 0)
      {
        esyslog("Could not read file, size:  %d", m_ReadCacheSize);
        m_ReadCacheSize = 0;
        return 0;
      }
      readBytes = m_ReadCacheSize;
      *buf = m_ReadCache;
      m_ReadCachePtr = 0;
    }
    else
    {
      esyslog("cVideoBufferRecording::ReadBlock - unknown error");
      return 0;
    }
  }
  else
    return 0;

  // Make sure we are looking at a TS packet
  while (readBytes > TS_SIZE)
  {
    if ((*buf)[0] == TS_SYNC_BYTE && (*buf)[TS_SIZE] == TS_SYNC_BYTE)
      break;
    m_BytesConsumed++;
    (*buf)++;
    readBytes--;
  }

  if ((*buf)[0] != TS_SYNC_BYTE)
  {
    return 0;
  }

  m_BytesConsumed += TS_SIZE;
  time(&endTime);
  wrapTime = 0;
  return TS_SIZE;
}

//-----------------------------------------------------------------------------

class cVideoBufferTest : public cVideoBufferFile
{
friend class cVideoBuffer;
public:
  virtual off_t GetPosMax();
  virtual void Receive(const uint16_t pid, const uint8_t* data, const size_t len);

protected:
  cVideoBufferTest(const std::string& filename);
  virtual ~cVideoBufferTest();
  virtual bool Init();
  virtual off_t Available();
  off_t GetPosEnd();
};

cVideoBufferTest::cVideoBufferTest(const std::string& filename)
{
  m_Filename = filename;
  m_ReadCacheSize = 0;
}

cVideoBufferTest::~cVideoBufferTest()
{
  m_file.Close();
}

off_t cVideoBufferTest::GetPosMax()
{
  m_WritePtr = GetPosEnd();
  return cVideoBufferTimeshift::GetPosMax();
}

off_t cVideoBufferTest::GetPosEnd()
{
  int64_t cur = m_file.GetPosition();
  off_t end = m_file.GetLength();
  m_file.Seek(cur, SEEK_SET);
  return end;
}

void cVideoBufferTest::Receive(const uint16_t pid, const uint8_t* data, const size_t len)
{

}

bool cVideoBufferTest::Init()
{
  m_ReadCache = 0;
  m_ReadCacheMaxSize = 8000;

  m_ReadCache = (uint8_t*)malloc(m_ReadCacheMaxSize);
  if (!m_ReadCache)
    return false;

  if (!m_file.Open(m_Filename))
  {
    esyslog("Could not open file: %s", m_Filename.c_str());
    return false;
  }

  m_WritePtr = 0;
  m_ReadPtr = 0;
  m_ReadCacheSize = 0;
  m_InputAttached = false;
  return true;
}

off_t cVideoBufferTest::Available()
{
  m_BufferSize = m_WritePtr = GetPosEnd();
  return cVideoBufferTimeshift::Available();
}

//-----------------------------------------------------------------------------

cVideoBuffer::cVideoBuffer()
{
  m_CheckEof = false;
  m_InputAttached = true;
  m_bufferEndTime = 0;
  m_bufferWrapTime = 0;
}

bool cVideoBuffer::Start(void)
{
  m_InputAttached = true;
  return true;
}

void cVideoBuffer::Stop(void)
{
  m_InputAttached = false;
}

int cVideoBuffer::Read(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime)
{
  int count = ReadBlock(buf, size, endTime, wrapTime);

  // check for end of file
  if (!m_InputAttached && count != TS_SIZE)
  {
    if (m_CheckEof && m_Timer.TimedOut())
    {
      isyslog("Recoding - end of file");
      return VIDEOBUFFER_EOF;
    }
    else if (!m_CheckEof)
    {
      m_CheckEof = true;
      m_Timer.Set(3000);
    }
  }
  else
    m_CheckEof = false;

  return count;
}

time_t cVideoBuffer::GetRefTime()
{
  time_t t;
  time(&t);
  return t;
}

cVideoBuffer* cVideoBuffer::Create(int clientID, uint8_t timeshift)
{
  // no time shift
  if (cSettings::Get().m_TimeshiftMode == TS_MODE_NONE || timeshift == 0)
  {
    cVideoBufferSimple *buffer = new cVideoBufferSimple();
    return buffer;
  }
  // buffer in ram
  else if (cSettings::Get().m_TimeshiftMode == TS_MODE_RAM)
  {
    cVideoBufferRAM *buffer = new cVideoBufferRAM();
    if (!buffer->Init())
    {
      delete buffer;
      return NULL;
    }
    else
      return buffer;
  }
  // buffer in file
  else if (cSettings::Get().m_TimeshiftMode == TS_MODE_FILE)
  {
    cVideoBufferFile *buffer = new cVideoBufferFile(clientID);
    if (!buffer->Init())
    {
      delete buffer;
      return NULL;
    }
    else
      return buffer;
  }
  else
    return NULL;
}

cVideoBuffer* cVideoBuffer::Create(const std::string& filename)
{
  isyslog("Open recording: %s", filename.c_str());
  cVideoBufferTest *buffer = new cVideoBufferTest(filename);
  if (!buffer->Init())
  {
    delete buffer;
    return NULL;
  }
  else
    return buffer;
}

cVideoBuffer* cVideoBuffer::Create(cRecording *rec)
{
  isyslog("Open recording: %s", rec->FileName().c_str());
  cVideoBufferRecording *buffer = new cVideoBufferRecording(rec);
  if (!buffer->Init())
  {
    delete buffer;
    return NULL;
  }
  else
    return buffer;
}

}
