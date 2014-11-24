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

#include "IndexFileGenerator.h"
#include "devices/Remux.h"
#include "filesystem/File.h"
#include "recordings/filesystem/IndexFile.h"
#include "utils/CommonMacros.h" // for KILOBYTE
#include "utils/log/Log.h"
#include "utils/Ringbuffer.h"

#define IFG_BUFFER_SIZE  KILOBYTE(100)

namespace VDR
{

cIndexFileGenerator::cIndexFileGenerator(const std::string& strRecordingName)
 : m_strRecordingName(strRecordingName)
{
  CreateThread(false);
}

cIndexFileGenerator::~cIndexFileGenerator(void)
{
  StopThread(3000);
}

void* cIndexFileGenerator::Process(void)
{
  bool IndexFileComplete = false;
  bool IndexFileWritten = false;
  bool Rewind = false;

  CFile ReplayFile;
  ReplayFile.Open(m_strRecordingName);

  cRingBufferLinear Buffer(IFG_BUFFER_SIZE, MIN_TS_PACKETS_FOR_FRAME_DETECTOR * TS_SIZE);

  cPatPmtParser PatPmtParser;
  cFrameDetector FrameDetector;

  cIndexFile IndexFile(m_strRecordingName, true);

  int BufferChunks = KILOBYTE(1); // no need to read a lot at the beginning when parsing PAT/PMT
  off_t FileSize = 0;
  off_t FrameOffset = -1;

  isyslog("Regenerating index file");

  while (!IsStopped())
  {
    // Rewind input file:
    if (Rewind)
    {
      ReplayFile.Seek(0, SEEK_SET);
      Buffer.Clear();
      Rewind = false;
    }

    // Process data:
    int Length;
    uint8_t* Data = Buffer.Get(Length);
    if (Data)
    {
      if (FrameDetector.Synced())
      {
        // Step 3 - generate the index:
        if (TsPid(Data) == PATPID)
          FrameOffset = FileSize; // the PAT/PMT is at the beginning of an I-frame

        int Processed = FrameDetector.Analyze(Data, Length);
        if (Processed > 0)
        {
          if (FrameDetector.NewFrame())
          {
            const unsigned int fileNumber = 0; // Previously VDR recording file number
            IndexFile.Write(FrameDetector.IndependentFrame(), fileNumber, FrameOffset >= 0 ? FrameOffset : FileSize);
            FrameOffset = -1;
            IndexFileWritten = true;
          }

          FileSize += Processed;
          Buffer.Del(Processed);
        }
      }
      else if (PatPmtParser.Vpid())
      {
        // Step 2 - sync FrameDetector:
        int Processed = FrameDetector.Analyze(Data, Length);
        if (Processed > 0)
        {
          if (FrameDetector.Synced())
          {
            // Synced FrameDetector, so rewind for actual processing:
            Rewind = true;
          }
          Buffer.Del(Processed);
        }
      }
      else
      {
        // Step 1 - parse PAT/PMT:
        uint8_t* p = Data;

        while (Length >= TS_SIZE)
        {
          int Pid = TsPid(p);

          if (Pid == PATPID)
            PatPmtParser.ParsePat(p, TS_SIZE);
          else if (PatPmtParser.IsPmtPid(Pid))
            PatPmtParser.ParsePmt(p, TS_SIZE);

          Length -= TS_SIZE;
          p += TS_SIZE;

          if (PatPmtParser.Vpid())
          {
            // Found Vpid, so rewind to sync FrameDetector:
            FrameDetector.SetPid(PatPmtParser.Vpid(), PatPmtParser.Vtype());
            BufferChunks = IFG_BUFFER_SIZE;
            Rewind = true;
            break;
          }
        }

        Buffer.Del(p - Data);
      }
    }
    // Read data:
    else if (ReplayFile.IsOpen())
    {
      int Result = Buffer.Read(ReplayFile, BufferChunks);

      // Check for EOF or error
      if (Result <= 0)
      {
        ReplayFile.Close();
        FileSize = 0;
        FrameOffset = -1;
        Buffer.Clear();
      }
    }
    // Recording has been processed:
    else
    {
      IndexFileComplete = true;
      break;
    }
  }

  if (IndexFileComplete)
  {
    if (IndexFileWritten)
    {
      /* TODO
      cRecordingInfo RecordingInfo(m_strRecordingName);
      if (RecordingInfo.Read())
      {
        if (FrameDetector.FramesPerSecond() > 0 && !DoubleEqual(RecordingInfo.FramesPerSecond(), FrameDetector.FramesPerSecond()))
        {
          RecordingInfo.SetFramesPerSecond(FrameDetector.FramesPerSecond());
          RecordingInfo.Write();
          Recordings.UpdateByName(m_strRecordingName);
        }
      }
      */

      isyslog("Index file regeneration complete");
      return NULL;
    }
    else
      esyslog("Index file regeneration failed!");
  }

  // Delete the index file if the recording has not been processed entirely:
  IndexFile.Delete();

  return NULL;
}

}
