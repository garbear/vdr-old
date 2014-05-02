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

#include "Recording.h"
#include "channels/ChannelTypes.h"
#include "devices/Receiver.h"
#include "devices/Remux.h"
#include "platform/threads/threads.h"
#include "utils/Ringbuffer.h"

#include <stdint.h>

namespace VDR
{
class cFileName;
class cIndexFile;
class cRecordingInfo;

class cRecorder : public cReceiver, PLATFORM::CThread
{
public:
  cRecorder(const std::string& strFileName, ChannelPtr Channel, int Priority);
               // Creates a new recorder for the given Channel and
               // the given Priority that will record into the file FileName.
  virtual ~cRecorder();

  virtual void SetEvent(const cEvent* event);

protected:
  virtual void Activate(bool On);
  virtual void Receive(uint8_t *Data, int Length);
  virtual void* Process(void);

private:
  bool RunningLowOnDiskSpace(void);
  bool NextFile(void);

  cRingBufferLinear* m_ringBuffer;
  cFrameDetector*    m_frameDetector;
  cPatPmtGenerator   m_patPmtGenerator;
  cFileName*         m_fileName;
  cIndexFile*        m_index;
  CVideoFile*        m_recordFile;
  std::string        m_strRecordingName;
  off_t              m_fileSize;
  time_t             m_lastDiskSpaceCheck;
  cRecordingInfo*    m_recordingInfo;
};
}
