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

#include "utils/CommonIncludes.h" // off_t problems on x86
#include "RecordingTypes.h"
#include "channels/ChannelTypes.h"
#include "devices/Receiver.h"
#include "devices/Remux.h"
#include "filesystem/File.h"
#include "lib/platform/threads/threads.h"
#include "lib/platform/util/timeutils.h"
#include "utils/Observer.h"
#include "utils/Ringbuffer.h"

#include <string>

namespace VDR
{

class CDateTimeSpan;
class cRecording;

class cRecorder : public    iReceiver,
                  protected PLATFORM::CThread,
                  public    Observer
{
public:
  cRecorder(cRecording* recording);
  virtual ~cRecorder(void);

  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Receive(const uint16_t pid, const uint8_t* data, const size_t len, ts_crc_check_t& crcvalid);

  virtual void LostPriority(void);

  virtual void Notify(const Observable& obs, const ObservableMessage msg);

protected:
  virtual void* Process(void);

private:
  bool RunningLowOnDiskSpace(void);

  cRecording* const  m_recording;
  std::string        m_strRecordingPath;
  cFrameDetector     m_frameDetector;
  CFile              m_file;
  cRingBufferLinear  m_ringBuffer;
  off_t              m_fileSize;
  PLATFORM::CTimeout m_checkDiskSpaceTimeout;
  cPatPmtGenerator   m_patPmtGenerator;
  const PLATFORM::CTimeout m_endTimer;
};

}
