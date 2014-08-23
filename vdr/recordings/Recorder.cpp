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

#include "Recorder.h"
#include "RecordingInfo.h"
#include "Recordings.h"
//#include "epg/Event.h"
#include "filesystem/IndexFile.h"
#include "filesystem/FileName.h"
#include "filesystem/Directory.h"
#include "filesystem/VideoFile.h"
#include "settings/Settings.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"
#include "utils/Shutdown.h"
#include "utils/Timer.h"

using namespace PLATFORM;

namespace VDR
{

// --- cRecorder -------------------------------------------------------------

cRecorder::cRecorder(const std::string& strFileName, ChannelPtr Channel)
:cReceiver(Channel), CThread()
{
  m_strRecordingName = strFileName;
  m_recordingInfo = NULL;

  // Make sure the disk is up and running:

  SpinUpDisk(strFileName);

  m_ringBuffer = new cRingBufferLinear(RECORDERBUFSIZE, MIN_TS_PACKETS_FOR_FRAME_DETECTOR * TS_SIZE, true, "Recorder");
  m_ringBuffer->SetTimeouts(0, 100);
  m_ringBuffer->SetIoThrottle();

  uint16_t Pid = Channel->GetVideoStream().vpid;
  uint16_t Type = Channel->GetVideoStream().vtype;
  if (!Pid && Channel->GetAudioStream(0).apid) {
     Pid = Channel->GetAudioStream(0).apid;
     Type = 0x04;
     }
  if (!Pid && Channel->GetDataStream(0).dpid) {
     Pid = Channel->GetDataStream(0).dpid;
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
  delete m_index;
  delete m_fileName;
  delete m_frameDetector;
  delete m_ringBuffer;
  delete m_recordingInfo;
}

bool cRecorder::RunningLowOnDiskSpace(void)
{
  if (time(NULL) > m_lastDiskSpaceCheck + DISKCHECKINTERVAL)
  {
    disk_space_t space;
    CDirectory::CalculateDiskSpace(m_fileName->Name(), space);
    m_lastDiskSpaceCheck = time(NULL);
    if (space.free < MINFREEDISKSPACE)
    {
      dsyslog("low disk space (%d MB, limit is %d MB)", space.free, MINFREEDISKSPACE / MEGABYTE(1));
      return true;
    }
  }
  return false;
}

bool cRecorder::NextFile(void)
{
  if (m_recordFile && m_frameDetector->IndependentFrame()) { // every file shall start with an independent frame
     if (m_fileSize > MEGABYTE(off_t(cSettings::Get().m_iMaxVideoFileSizeMB)) || RunningLowOnDiskSpace()) {
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

void cRecorder::Receive(const std::vector<uint8_t>& data)
{
  if (!IsStopped()) {
     int p = m_ringBuffer->Put(data.data(), data.size());
     if (p != data.size() && !IsStopped())
        m_ringBuffer->ReportOverflow(data.size() - p);
     }
}

void cRecorder::SetEvent(const EventPtr& event)
{
  if (m_recordingInfo)
    m_recordingInfo->SetEvent(event);
}

void* cRecorder::Process(void)
{
  cTimeMs t(MAXBROKENTIMEOUT);
  bool InfoWritten = false;
  bool FirstIframeSeen = false;
  while (!IsStopped()) {
        int r;
        uint8_t *b = m_ringBuffer->Get(r);
        if (b) {
           int Count = m_frameDetector->Analyze(b, r);
           if (Count) {
              if (IsStopped() && m_frameDetector->IndependentFrame()) // finish the recording before the next independent frame
                 break;
              if (m_frameDetector->Synced()) {
                 if (!InfoWritten) {
                   m_recordingInfo = new cRecordingInfo(m_strRecordingName);
                    if (m_recordingInfo->Read()) {
                       if (m_frameDetector->FramesPerSecond() > 0 && DoubleEqual(m_recordingInfo->FramesPerSecond(), DEFAULTFRAMESPERSECOND) && !DoubleEqual(m_recordingInfo->FramesPerSecond(), m_frameDetector->FramesPerSecond())) {
                         m_recordingInfo->SetFramesPerSecond(m_frameDetector->FramesPerSecond());
                         m_recordingInfo->Write();
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
                       while (uint8_t *pmt = m_patPmtGenerator.GetPmt(Index)) {
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

}
