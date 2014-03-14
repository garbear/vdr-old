/*
 * recorder.h: The actual DVB recorder
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: recorder.h 2.3 2010/12/27 11:17:04 kls Exp $
 */

#ifndef __RECORDER_H
#define __RECORDER_H

#include "Types.h"
#include "devices/Receiver.h"
#include "Recording.h"
#include "devices/Remux.h"
#include "utils/Ringbuffer.h"
#include "platform/threads/threads.h"

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
  virtual void Receive(uchar *Data, int Length);
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

#endif //__RECORDER_H
