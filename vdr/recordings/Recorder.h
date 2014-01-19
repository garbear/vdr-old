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

#include "devices/Receiver.h"
#include "Recording.h"
#include "devices/Remux.h"
#include "utils/Ringbuffer.h"
#include "platform/threads/threads.h"

class cRecorder : public cReceiver, PLATFORM::CThread {
private:
  cRingBufferLinear *ringBuffer;
  cFrameDetector *frameDetector;
  cPatPmtGenerator patPmtGenerator;
  cFileName *fileName;
  cIndexFile *index;
  cUnbufferedFile *recordFile;
  char *recordingName;
  off_t fileSize;
  time_t lastDiskSpaceCheck;
  bool RunningLowOnDiskSpace(void);
  bool NextFile(void);
protected:
  virtual void Activate(bool On);
  virtual void Receive(uchar *Data, int Length);
  virtual void* Process(void);
public:
  cRecorder(const char *FileName, ChannelPtr Channel, int Priority);
               // Creates a new recorder for the given Channel and
               // the given Priority that will record into the file FileName.
  virtual ~cRecorder();
  };

#endif //__RECORDER_H
