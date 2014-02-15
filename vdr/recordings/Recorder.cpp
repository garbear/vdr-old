/*
 * recorder.c: The actual DVB recorder
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: recorder.c 2.17.1.1 2013/10/12 12:10:05 kls Exp $
 */

#include "Recorder.h"
#include "RecordingInfo.h"
#include "Recordings.h"
#include "filesystem/IndexFile.h"
#include "filesystem/FileName.h"
#include "filesystem/Directory.h"
#include "utils/Shutdown.h"

#define RECORDERBUFSIZE  (MEGABYTE(20) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE

// The maximum time we wait before assuming that a recorded video data stream
// is broken:
#define MAXBROKENTIMEOUT 30000 // milliseconds

#define MINFREEDISKSPACE    (512) // MB
#define DISKCHECKINTERVAL   100 // seconds

using namespace PLATFORM;

// --- cRecorder -------------------------------------------------------------

cRecorder::cRecorder(const std::string& strFileName, ChannelPtr Channel, int Priority)
:cReceiver(Channel, Priority), CThread()
{
  m_strRecordingName = strFileName;

  // Make sure the disk is up and running:

  SpinUpDisk(strFileName);

  m_ringBuffer = new cRingBufferLinear(RECORDERBUFSIZE, MIN_TS_PACKETS_FOR_FRAME_DETECTOR * TS_SIZE, true, "Recorder");
  m_ringBuffer->SetTimeouts(0, 100);
  m_ringBuffer->SetIoThrottle();

  int Pid = Channel->Vpid();
  int Type = Channel->Vtype();
  if (!Pid && Channel->Apid(0)) {
     Pid = Channel->Apid(0);
     Type = 0x04;
     }
  if (!Pid && Channel->Dpid(0)) {
     Pid = Channel->Dpid(0);
     Type = 0x06;
     }
  m_frameDetector = new cFrameDetector(Pid, Type);
  m_index = NULL;
  m_fileSize = 0;
  m_lastDiskSpaceCheck = time(NULL);
  m_fileName = new cFileName(strFileName, true);
  int PatVersion, PmtVersion;
  if (m_fileName->GetLastPatPmtVersions(PatVersion, PmtVersion))
     m_patPmtGenerator.SetVersions(PatVersion + 1, PmtVersion + 1);
  m_patPmtGenerator.SetChannel(Channel);
  m_recordFile = m_fileName->Open();
  if (!m_recordFile)
     return;
  // Create the index file:
  m_index = new cIndexFile(strFileName, true);
  if (!m_index)
     esyslog("ERROR: can't allocate index");
     // let's continue without index, so we'll at least have the recording
}

cRecorder::~cRecorder()
{
  Detach();
  delete m_index;
  delete m_fileName;
  delete m_frameDetector;
  delete m_ringBuffer;
}

bool cRecorder::RunningLowOnDiskSpace(void)
{
  if (time(NULL) > m_lastDiskSpaceCheck + DISKCHECKINTERVAL) {
     unsigned int total, used, free;
     CDirectory::CalculateDiskSpace(m_fileName->Name(), total, used, free);
     int Free = free;
     m_lastDiskSpaceCheck = time(NULL);
     if (Free < MINFREEDISKSPACE) {
        dsyslog("low disk space (%d MB, limit is %d MB)", Free, MINFREEDISKSPACE);
        return true;
        }
     }
  return false;
}

bool cRecorder::NextFile(void)
{
  if (m_recordFile && m_frameDetector->IndependentFrame()) { // every file shall start with an independent frame
     if (m_fileSize > MEGABYTE(off_t(g_setup.MaxVideoFileSize)) || RunningLowOnDiskSpace()) {
        m_recordFile = m_fileName->NextFile();
        m_fileSize = 0;
        }
     }
  return m_recordFile != NULL;
}

void cRecorder::Activate(bool On)
{
  if (On)
     CreateThread();
  else
     StopThread(3);
}

void cRecorder::Receive(uchar *Data, int Length)
{
  if (!IsStopped()) {
     int p = m_ringBuffer->Put(Data, Length);
     if (p != Length && !IsStopped())
        m_ringBuffer->ReportOverflow(Length - p);
     }
}

void* cRecorder::Process(void)
{
  cTimeMs t(MAXBROKENTIMEOUT);
  bool InfoWritten = false;
  bool FirstIframeSeen = false;
  while (!IsStopped()) {
        int r;
        uchar *b = m_ringBuffer->Get(r);
        if (b) {
           int Count = m_frameDetector->Analyze(b, r);
           if (Count) {
              if (IsStopped() && m_frameDetector->IndependentFrame()) // finish the recording before the next independent frame
                 break;
              if (m_frameDetector->Synced()) {
                 if (!InfoWritten) {
                    cRecordingInfo RecordingInfo(m_strRecordingName);
                    if (RecordingInfo.Read()) {
                       if (m_frameDetector->FramesPerSecond() > 0 && DoubleEqual(RecordingInfo.FramesPerSecond(), DEFAULTFRAMESPERSECOND) && !DoubleEqual(RecordingInfo.FramesPerSecond(), m_frameDetector->FramesPerSecond())) {
                          RecordingInfo.SetFramesPerSecond(m_frameDetector->FramesPerSecond());
                          RecordingInfo.Write();
                          Recordings.UpdateByName(m_strRecordingName);
                          }
                       }
                    InfoWritten = true;
                    }
                 if (FirstIframeSeen || m_frameDetector->IndependentFrame()) {
                    FirstIframeSeen = true; // start recording with the first I-frame
                    if (!NextFile())
                       break;
                    if (m_index && m_frameDetector->NewFrame())
                       m_index->Write(m_frameDetector->IndependentFrame(), m_fileName->Number(), m_fileSize);
                    if (m_frameDetector->IndependentFrame()) {
                       m_recordFile->Write(m_patPmtGenerator.GetPat(), TS_SIZE);
                       m_fileSize += TS_SIZE;
                       int Index = 0;
                       while (uchar *pmt = m_patPmtGenerator.GetPmt(Index)) {
                             m_recordFile->Write(pmt, TS_SIZE);
                             m_fileSize += TS_SIZE;
                             }
                       }
                    if (m_recordFile->Write(b, Count) < 0) {
                       LOG_ERROR_STR(m_fileName->Name().c_str());
                       break;
                       }
                    m_fileSize += Count;
                    t.Set(MAXBROKENTIMEOUT);
                    }
                 }
              m_ringBuffer->Del(Count);
              }
           }
        if (t.TimedOut()) {
           esyslog("ERROR: video data stream broken");
           ShutdownHandler.RequestEmergencyExit();
           t.Set(MAXBROKENTIMEOUT);
           }
        }
  return NULL;
}
