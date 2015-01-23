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
#include "Recording.h"
#include "RecordingConfig.h"
#include "filesystem/File.h"
#include "filesystem/FileName.h"
#include "filesystem/IndexFile.h"
#include "filesystem/Directory.h"
#include "lib/platform/util/timeutils.h"
#include "settings/Settings.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"

using namespace PLATFORM;

#define RECORDERBUFSIZE     (MEGABYTE(20) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE

// The maximum time we wait before assuming that a recorded video data stream
// is broken:
#define MAXBROKENTIMEOUT    30000 // milliseconds

#define MINFREEDISKSPACE    MEGABYTE(512)
#define DISKCHECKINTERVAL   100 // seconds

namespace VDR
{

// --- cRecorder ---------------------------------------------------------------

cRecorder::cRecorder(cRecording* recording)
 : m_recording(recording),
   m_strRecordingPath(recording->URL()),
   m_frameDetector(recording->Channel()),
   m_ringBuffer(RECORDERBUFSIZE, MIN_TS_PACKETS_FOR_FRAME_DETECTOR * TS_SIZE, true, "Recorder"),
   m_fileSize(0),
   m_endTimer((recording->EndTime() - CDateTime::GetUTCDateTime()).GetSecondsTotal() * 1000)
{
  m_recording->RegisterObserver(this);

  m_ringBuffer.SetTimeouts(0, 100);
  m_ringBuffer.SetIoThrottle();

  m_patPmtGenerator.SetChannel(m_recording->Channel());
}

cRecorder::~cRecorder(void)
{
  m_recording->UnregisterObserver(this);
}

bool cRecorder::RunningLowOnDiskSpace(void)
{
  if (m_checkDiskSpaceTimeout.TimeLeft() == 0)
  {
    m_checkDiskSpaceTimeout.Init(DISKCHECKINTERVAL * 1000);

    disk_space_t space;
    CDirectory::CalculateDiskSpace(m_strRecordingPath, space);

    if (space.free < MINFREEDISKSPACE)
    {
      dsyslog("low disk space (%d MB, limit is %d MB)", space.free, MINFREEDISKSPACE / MEGABYTE(1));
      return true;
    }
  }
  return false;
}

bool cRecorder::Start(void)
{
  if (m_file.OpenForWrite(m_strRecordingPath))
    return CreateThread(true);

  return false;
}

void cRecorder::Stop(void)
{
  StopThread(-1);
}

void cRecorder::Receive(const uint16_t pid, const uint8_t* data, const size_t len, ts_crc_check_t& crcvalid)
{
  if (!IsStopped())
  {
    int p = m_ringBuffer.Put(data, len);
    if (p != len && !IsStopped())
      m_ringBuffer.ReportOverflow(len - p);
  }
}

void* cRecorder::Process(void)
{
  CTimeout brokenTimeout(MAXBROKENTIMEOUT);
  bool     bFirstIframeSeen = false;

  while (!IsStopped())
  {
    int bufferLength;
    uint8_t* buffer = m_ringBuffer.Get(bufferLength);
    if (buffer)
    {
      int count = m_frameDetector.Analyze(buffer, bufferLength);
      if (count)
      {
        // finish the recording before the next independent frame
        if (m_frameDetector.IndependentFrame())
        {
          if (m_endTimer.TimeLeft() == 0)
            StopThread(-1);
          if (IsStopped())
            break;
        }

        if (m_frameDetector.Synced())
        {
          if (bFirstIframeSeen || m_frameDetector.IndependentFrame())
          {
            bFirstIframeSeen = true; // start recording with the first I-frame

            //if (m_frameDetector.NewFrame())
            //  m_index.Write(m_frameDetector.IndependentFrame(), m_fileName.Number(), m_fileSize);

            if (m_frameDetector.IndependentFrame())
            {
              m_file.Write(m_patPmtGenerator.GetPat(), TS_SIZE);
              m_fileSize += TS_SIZE;

              int index = 0;
              while (uint8_t* pmt = m_patPmtGenerator.GetPmt(index))
              {
                m_file.Write(pmt, TS_SIZE);
                m_fileSize += TS_SIZE;
              }
            }

            if (m_file.Write(buffer, count) < 0)
            {
              LOG_ERROR_STR(m_strRecordingPath.c_str());
              break;
            }

            m_fileSize += count;
            brokenTimeout.Init(MAXBROKENTIMEOUT);
          }
        }

        m_ringBuffer.Del(count);
      }
    }

    if (brokenTimeout.TimeLeft() == 0)
    {
      esyslog("ERROR: video data stream broken");
      brokenTimeout.Init(MAXBROKENTIMEOUT);
    }
  }
  return NULL;
}

void cRecorder::LostPriority(void)
{
  m_recording->Interrupt();
}

void cRecorder::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageRecordingChanged:
    if (m_strRecordingPath != m_recording->URL())
    {
      // TODO: recording was renamed
      //m_strRecordingPath = m_recording->URL();
    }
    break;
  default:
    break;
  }
}

}
