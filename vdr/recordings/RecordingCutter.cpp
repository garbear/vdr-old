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

#include "RecordingCutter.h"
#include "Recordings.h"
#include "RecordingUserCommand.h"
#include "recordings/marks/Marks.h"
#include "devices/Remux.h"
#include "filesystem/Directory.h"
#include "filesystem/FileName.h"
#include "filesystem/IndexFile.h"
#include "filesystem/Videodir.h"
#include "filesystem/VideoFile.h"
#include "lib/platform/threads/throttle.h"
#include "settings/Settings.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"

#include <stdint.h>

using namespace PLATFORM;

namespace VDR
{

// --- cPacketBuffer ---------------------------------------------------------

class cPacketBuffer
{
public:
  cPacketBuffer(void);
  ~cPacketBuffer(void);

  /*!
   * Appends Length bytes of Data to this packet buffer.
   */
  void Append(uint8_t *Data, int Length);

  /*!
   * Flushes the content of this packet buffer into the given Data, starting at
   * position Length, and clears the buffer afterwards. Length will be
   * incremented accordingly. If Length plus the total length of the stored
   * packets would exceed MaxLength, nothing is copied.
   */
  void Flush(uint8_t *Data, int &Length, int MaxLength);

private:
  uint8_t* data;
  int      size;
  int      length;
};

cPacketBuffer::cPacketBuffer(void)
{
  data = NULL;
  size = length = 0;
}

cPacketBuffer::~cPacketBuffer()
{
  free(data);
}

void cPacketBuffer::Append(uint8_t *Data, int Length)
{
  if (length + Length >= size) {
     int NewSize = (length + Length) * 3 / 2;
     if (uint8_t *p = (uint8_t*)realloc(data, NewSize)) {
        data = p;
        size = NewSize;
        }
     else
        return; // out of memory
     }
  memcpy(data + length, Data, Length);
  length += Length;
}

void cPacketBuffer::Flush(uint8_t *Data, int &Length, int MaxLength)
{
  if (Data && length > 0 && Length + length <= MaxLength) {
     memcpy(Data + Length, data, length);
     Length += length;
     }
  length = 0;
}

// --- cPacketStorage --------------------------------------------------------

class cPacketStorage
{
public:
  void Append(int Pid, uint8_t *Data, int Length);
  void Flush(int Pid, uint8_t *Data, int &Length, int MaxLength);

private:
  std::map<uint16_t, std::vector<uint8_t> > buffers;
};

void cPacketStorage::Append(int Pid, uint8_t *Data, int Length)
{
  std::vector<uint8_t>& data = buffers[Pid];
  std::vector<uint8_t> data(Data, Data + Length);
  buffers[Pid].push_back(data.begin(), data.end());
}

void cPacketStorage::Flush(int Pid, uint8_t *Data, int &Length, int MaxLength)
{
  if ()
  std::vector<uint8_t>& data = buffers[Pid];
  memcpy(Data + Length, data.data(), data.size());
  Length += data.size();
  data.clear();
}

// --- cMpeg2Fixer -----------------------------------------------------------

class cMpeg2Fixer : private cTsPayload {
private:
  bool FindHeader(uint32_t Code, const char *Header);
public:
  cMpeg2Fixer(uint8_t *Data, int Length, int Vpid);
  void SetBrokenLink(void);
  void SetClosedGop(void);
  int GetTref(void);
  void AdjGopTime(int Offset, int FramesPerSecond);
  void AdjTref(int TrefOffset);
  };

cMpeg2Fixer::cMpeg2Fixer(uint8_t *Data, int Length, int Vpid)
{
  // Go to first video packet:
  for (; Length > 0; Data += TS_SIZE, Length -= TS_SIZE) {
      if (TsPid(Data) == Vpid) {
         Setup(Data, Length, Vpid);
         break;
         }
      }
}

bool cMpeg2Fixer::FindHeader(uint32_t Code, const char *Header)
{
  Reset();
  if (Find(Code))
     return true;
  esyslog("ERROR: %s header not found!", Header);
  return false;
}

void cMpeg2Fixer::SetBrokenLink(void)
{
  if (!FindHeader(0x000001B8, "GOP"))
     return;
  SkipBytes(3);
  uint8_t b = GetByte();
  if (!(b & 0x40)) { // GOP not closed
     b |= 0x20;
     SetByte(b, GetLastIndex());
     }
}

void cMpeg2Fixer::SetClosedGop(void)
{
  if (!FindHeader(0x000001B8, "GOP"))
     return;
  SkipBytes(3);
  uint8_t b = GetByte();
  b |= 0x40;
  SetByte(b, GetLastIndex());
}

int cMpeg2Fixer::GetTref(void)
{
  if (!FindHeader(0x00000100, "picture"))
     return 0;
  int Tref = GetByte() << 2;
  Tref |= GetByte() >> 6;
  return Tref;
}

void cMpeg2Fixer::AdjGopTime(int Offset, int FramesPerSecond)
{
  if (!FindHeader(0x000001B8, "GOP"))
     return;
  uint8_t Byte1 = GetByte();
  int Index1 = GetLastIndex();
  uint8_t Byte2 =  GetByte();
  int Index2 = GetLastIndex();
  uint8_t Byte3 = GetByte();
  int Index3 = GetLastIndex();
  uint8_t Byte4 =  GetByte();
  int Index4 = GetLastIndex();
  uint8_t Frame = ((Byte3 & 0x1F) << 1) | (Byte4 >> 7);
  uint8_t Sec   = ((Byte2 & 0x07) << 3) | (Byte3 >> 5);
  uint8_t Min   = ((Byte1 & 0x03) << 4) | (Byte2 >> 4);
  uint8_t Hour  = ((Byte1 & 0x7C) >> 2);
  int GopTime = ((Hour * 60 + Min) * 60 + Sec) * FramesPerSecond + Frame;
  if (GopTime) { // do not fix when zero
     GopTime += Offset;
     if (GopTime < 0)
        GopTime += 24 * 60 * 60 * FramesPerSecond;
     Frame = GopTime % FramesPerSecond;
     GopTime = GopTime / FramesPerSecond;
     Sec = GopTime % 60;
     GopTime = GopTime / 60;
     Min = GopTime % 60;
     GopTime = GopTime / 60;
     Hour = GopTime % 24;
     SetByte((Byte1 & 0x80) | (Hour << 2) | (Min >> 4), Index1);
     SetByte(((Min & 0x0F) << 4) | 0x08 | (Sec >> 3), Index2);
     SetByte(((Sec & 0x07) << 3) | (Frame >> 1), Index3);
     SetByte((Byte4 & 0x7F) | ((Frame & 0x01) << 7), Index4);
     }
}

void cMpeg2Fixer::AdjTref(int TrefOffset)
{
  if (!FindHeader(0x00000100, "picture"))
     return;
  int Tref = GetByte() << 2;
  int Index1 = GetLastIndex();
  uint8_t Byte2 = GetByte();
  int Index2 = GetLastIndex();
  Tref |= Byte2 >> 6;
  Tref -= TrefOffset;
  SetByte(Tref >> 2, Index1);
  SetByte((Tref << 6) | (Byte2 & 0x3F), Index2);
}

// --- cCuttingThread --------------------------------------------------------

class cCuttingThread : public CThread {
private:
  const char *error;
  bool isPesRecording;
  double framesPerSecond;
  CVideoFile *fromFile, *toFile;
  cFileName *fromFileName, *toFileName;
  cIndexFile *fromIndex, *toIndex;
  cMarks fromMarks, toMarks;
  int numSequences;
  off_t maxVideoFileSize;
  off_t fileSize;
  bool suspensionLogged;
  int sequence;          // cutting sequence
  int delta;             // time between two frames (PTS ticks)
  int64_t lastVidPts;    // the video PTS of the last frame (in display order)
  bool multiFramePkt;    // multiple frames within one PES packet
  int64_t firstPts;      // first valid PTS, for dangling packet stripping
  int64_t offset;        // offset to add to all timestamps
  int tRefOffset;        // number of stripped frames in GOP
  uint8_t counter[MAXPID]; // the TS continuity counter for each PID
  bool keepPkt[MAXPID];  // flag for each PID to keep packets, for dangling packet stripping
  int numIFrames;        // number of I-frames without pending packets
  cPatPmtParser patPmtParser;
  bool Throttled(void);
  bool SwitchFile(bool Force = false);
  bool LoadFrame(int Index, uint8_t *Buffer, bool &Independent, int &Length);
  bool FramesAreEqual(int Index1, int Index2);
  void GetPendingPackets(uint8_t *Buffer, int &Length, int Index);
       // Gather all non-video TS packets from Index upward that either belong to
       // payloads that started before Index, or have a PTS that is before lastVidPts,
       // and add them to the end of the given Data.
  bool FixFrame(uint8_t *Data, int &Length, bool Independent, int Index, bool CutIn, bool CutOut);
  bool ProcessSequence(int LastEndIndex, int BeginIndex, int EndIndex, int NextBeginIndex);
protected:
  virtual void* Process(void);
public:
  cCuttingThread(const std::string& strFromFileName, const std::string& strToFileName);
  virtual ~cCuttingThread();
  const char *Error(void) { return error; }
  };

cCuttingThread::cCuttingThread(const std::string& strFromFileName, const std::string& strToFileName)
:CThread()
{
  error = NULL;
  fromFile = toFile = NULL;
  fromFileName = toFileName = NULL;
  fromIndex = toIndex = NULL;
  cRecording Recording(strFromFileName);
  isPesRecording = Recording.IsPesRecording();
  framesPerSecond = Recording.FramesPerSecond();
  suspensionLogged = false;
  fileSize = 0;
  sequence = 0;
  delta = int(round(PTSTICKS / framesPerSecond));
  lastVidPts = -1;
  multiFramePkt = false;
  firstPts = -1;
  offset = 0;
  tRefOffset = 0;
  memset(counter, 0x00, sizeof(counter));
  numIFrames = 0;
  if (fromMarks.Load(strFromFileName.c_str(), framesPerSecond, isPesRecording) && !fromMarks.Empty()) {
     numSequences = fromMarks.GetNumSequences();
     if (numSequences > 0) {
        fromFileName = new cFileName(strFromFileName, false, true, isPesRecording);
        toFileName = new cFileName(strToFileName, true, true, isPesRecording);
        fromIndex = new cIndexFile(strFromFileName, false, isPesRecording);
        toIndex = new cIndexFile(strToFileName, true, isPesRecording);
        toMarks.Load(strToFileName.c_str(), framesPerSecond, isPesRecording); // doesn't actually load marks, just sets the file name
        maxVideoFileSize = MEGABYTE(cSettings::Get().m_iMaxVideoFileSizeMB);
        if (isPesRecording && maxVideoFileSize > MEGABYTE(MAXVIDEOFILESIZEPES))
           maxVideoFileSize = MEGABYTE(MAXVIDEOFILESIZEPES);
        CreateThread();
        }
     else
        esyslog("no editing sequences found for %s", strFromFileName.c_str());
     }
  else
     esyslog("no editing marks found for %s", strFromFileName.c_str());
}

cCuttingThread::~cCuttingThread()
{
  StopThread(3000);
  delete fromFileName;
  delete toFileName;
  delete fromIndex;
  delete toIndex;
}

bool cCuttingThread::Throttled(void)
{
  if (PLATFORM::cIoThrottle::Engaged()) {
     if (!suspensionLogged) {
        dsyslog("suspending cutter thread");
        suspensionLogged = true;
        }
     return true;
     }
  else if (suspensionLogged) {
     dsyslog("resuming cutter thread");
     suspensionLogged = false;
     }
  return false;
}

bool cCuttingThread::LoadFrame(int Index, uint8_t *Buffer, bool &Independent, int &Length)
{
  uint16_t FileNumber;
  off_t FileOffset;
  if (fromIndex->Get(Index, &FileNumber, &FileOffset, &Independent, &Length)) {
     fromFile = fromFileName->SetOffset(FileNumber, FileOffset);
     if (fromFile) {
        fromFile->SetReadAhead(MEGABYTE(20));
        int len = cRecording::ReadFrame(fromFile, Buffer,  Length, MAXFRAMESIZE);
        if (len < 0)
           error = "ReadFrame";
        else if (len != Length)
           Length = len;
        return error == NULL;
        }
     else
        error = "fromFile";
     }
  return false;
}

bool cCuttingThread::SwitchFile(bool Force)
{
  if (fileSize > maxVideoFileSize || Force) {
     toFile = toFileName->NextFile();
     if (!toFile) {
        error = "toFile";
        return false;
        }
     fileSize = 0;
     }
  return true;
}

class cHeapBuffer {
private:
  uint8_t *buffer;
public:
  cHeapBuffer(int Size) { buffer = MALLOC(uint8_t, Size); }
  ~cHeapBuffer() { free(buffer); }
  operator uint8_t*() { return buffer; }
  };

bool cCuttingThread::FramesAreEqual(int Index1, int Index2)
{
  cHeapBuffer Buffer1(MAXFRAMESIZE);
  cHeapBuffer Buffer2(MAXFRAMESIZE);
  if (!Buffer1 || !Buffer2)
     return false;
  bool Independent;
  int Length1;
  int Length2;
  if (LoadFrame(Index1, Buffer1, Independent, Length1) && LoadFrame(Index2, Buffer2, Independent, Length2)) {
     if (Length1 == Length2) {
        int Diffs = 0;
        for (int i = 0; i < Length1; i++) {
            if (Buffer1[i] != Buffer2[i]) {
               if (Diffs++ > 10) // the continuity counters of the PAT/PMT packets may differ
                  return false;
               }
            }
        return true;
        }
     }
  return false;
}

void cCuttingThread::GetPendingPackets(uint8_t *Data, int &Length, int Index)
{
  cHeapBuffer Buffer(MAXFRAMESIZE);
  if (!Buffer)
     return;
  bool Processed[MAXPID] = { false };
  cPacketStorage PacketStorage;
  int64_t LastPts = lastVidPts + delta;// adding one frame length to fully cover the very last frame
  Processed[patPmtParser.Vpid()] = true; // we only want non-video packets
  for (int NumIndependentFrames = 0; NumIndependentFrames < 2; Index++) {
      bool Independent;
      int len;
      if (LoadFrame(Index, Buffer, Independent, len)) {
         if (Independent)
            NumIndependentFrames++;
         for (uint8_t *p = Buffer; len >= TS_SIZE && *p == TS_SYNC_BYTE; len -= TS_SIZE, p += TS_SIZE) {
             int Pid = TsPid(p);
             if (Pid != PATPID && !patPmtParser.IsPmtPid(Pid) && !Processed[Pid]) {
                int64_t Pts = TsGetPts(p, TS_SIZE);
                if (Pts >= 0) {
                   int64_t d = PtsDiff(LastPts, Pts);
                   if (d < 0) // Pts is before LastPts
                      PacketStorage.Flush(Pid, Data, Length, MAXFRAMESIZE);
                   else { // Pts is at or after LastPts
                      NumIndependentFrames = 0; // we search until we find two consecutive I-frames without any more pending packets
                      Processed[Pid] = true;
                      }
                   }
                if (!Processed[Pid])
                   PacketStorage.Append(Pid, p, TS_SIZE);
                }
             }
         }
      else
         break;
      }
}

bool cCuttingThread::FixFrame(uint8_t *Data, int &Length, bool Independent, int Index, bool CutIn, bool CutOut)
{
  if (!patPmtParser.Vpid()) {
     if (!patPmtParser.ParsePatPmt(Data, Length))
        return false;
     }
  if (CutIn) {
     sequence++;
     memset(keepPkt, false, sizeof(keepPkt));
     numIFrames = 0;
     firstPts = TsGetPts(Data, Length);
     // Determine the PTS offset at the beginning of each sequence (except the first one):
     if (sequence != 1) {
        if (firstPts >= 0)
           offset = (lastVidPts + delta - firstPts) & MAX33BIT;
        }
     }
  if (CutOut)
     GetPendingPackets(Data, Length, Index + 1);
  if (Independent) {
     numIFrames++;
     tRefOffset = 0;
     }
  // Fix MPEG-2:
  if (patPmtParser.Vtype() == 2) {
     cMpeg2Fixer Mpeg2fixer(Data, Length, patPmtParser.Vpid());
     if (CutIn) {
        if (sequence == 1 || multiFramePkt)
           Mpeg2fixer.SetBrokenLink();
        else {
           Mpeg2fixer.SetClosedGop();
           tRefOffset = Mpeg2fixer.GetTref();
           }
        }
     if (tRefOffset)
        Mpeg2fixer.AdjTref(tRefOffset);
     if (sequence > 1 && Independent)
        Mpeg2fixer.AdjGopTime((offset - MAX33BIT) / delta, round(framesPerSecond));
     }
  bool DeletedFrame = false;
  bool GotVidPts = false;
  bool StripVideo = sequence > 1 && tRefOffset;
  uint8_t *p;
  int len;
  for (p = Data, len = Length; len >= TS_SIZE && *p == TS_SYNC_BYTE; p += TS_SIZE, len -= TS_SIZE) {
      int Pid = TsPid(p);
      // Strip dangling packets:
      if (numIFrames < 2 && Pid != PATPID && !patPmtParser.IsPmtPid(Pid)) {
         if (Pid != patPmtParser.Vpid() || StripVideo) {
            int64_t Pts = TsGetPts(p, TS_SIZE);
            if (Pts >= 0)
               keepPkt[Pid] = PtsDiff(firstPts, Pts) >= 0; // Pts is at or after FirstPts
            if (!keepPkt[Pid]) {
               TsHidePayload(p);
               numIFrames = 0; // we search until we find two consecutive I-frames without any more dangling packets
               if (Pid == patPmtParser.Vpid())
                  DeletedFrame = true;
               }
            }
         }
      // Adjust the TS continuity counter:
      if (sequence > 1) {
         if (TsHasPayload(p))
            counter[Pid] = (counter[Pid] + 1) & TS_CONT_CNT_MASK;
         TsSetContinuityCounter(p, counter[Pid]);
         }
      else
         counter[Pid] = TsGetContinuityCounter(p); // collect initial counters
      // Adjust PTS:
      int64_t Pts = TsGetPts(p, TS_SIZE);
      if (Pts >= 0) {
         if (sequence > 1) {
            Pts = PtsAdd(Pts, offset);
            TsSetPts(p, TS_SIZE, Pts);
            }
         // Keep track of the highest video PTS - in case of multiple fields per frame, take the first one
         if (!GotVidPts && Pid == patPmtParser.Vpid()) {
            GotVidPts = true;
            if (lastVidPts < 0 || PtsDiff(lastVidPts, Pts) > 0)
               lastVidPts = Pts;
            }
         }
      // Adjust DTS:
      if (sequence > 1) {
         int64_t Dts = TsGetDts(p, TS_SIZE);
         if (Dts >= 0) {
            Dts = PtsAdd(Dts, offset);
            if (CutIn)
               Dts = PtsAdd(Dts, tRefOffset * delta);
            TsSetDts(p, TS_SIZE, Dts);
            }
         int64_t Pcr = TsGetPcr(p);
         if (Pcr >= 0) {
            Pcr = Pcr + offset * PCRFACTOR;
            if (Pcr > MAX27MHZ)
               Pcr -= MAX27MHZ + 1;
            TsSetPcr(p, Pcr);
            }
         }
      }
  if (!DeletedFrame && !GotVidPts) {
     // Adjust PTS for multiple frames within a single PES packet:
     lastVidPts = (lastVidPts + delta) & MAX33BIT;
     multiFramePkt = true;
     }
  return DeletedFrame;
}

bool cCuttingThread::ProcessSequence(int LastEndIndex, int BeginIndex, int EndIndex, int NextBeginIndex)
{
  // Check for seamless connections:
  bool SeamlessBegin = LastEndIndex >= 0 && FramesAreEqual(LastEndIndex, BeginIndex);
  bool SeamlessEnd = NextBeginIndex >= 0 && FramesAreEqual(EndIndex, NextBeginIndex);
  // Process all frames from BeginIndex (included) to EndIndex (excluded):
  cHeapBuffer Buffer(MAXFRAMESIZE);
  if (!Buffer) {
     error = "malloc";
     return false;
     }
  for (int Index = BeginIndex; !IsStopped() && Index < EndIndex; Index++) {
      bool Independent;
      int Length;
      if (LoadFrame(Index, Buffer, Independent, Length)) {
         // Make sure there is enough disk space:
         Recordings.AssertFreeDiskSpace(-1);
         bool CutIn = !SeamlessBegin && Index == BeginIndex;
         bool CutOut = !SeamlessEnd && Index == EndIndex - 1;
         bool DeletedFrame = false;
         if (!isPesRecording) {
            DeletedFrame = FixFrame(Buffer, Length, Independent, Index, CutIn, CutOut);
            }
         else if (CutIn)
            cRemux::SetBrokenLink(Buffer, Length);
         // Every file shall start with an independent frame:
         if (Independent) {
            if (!SwitchFile())
               return false;
            }
         // Write index:
         if (!DeletedFrame && !toIndex->Write(Independent, toFileName->Number(), fileSize)) {
            error = "toIndex";
            return false;
            }
         // Write data:
         if (toFile->Write(Buffer, Length) < 0) {
            error = "safe_write";
            return false;
            }
         fileSize += Length;
         // Generate marks at the editing points in the edited recording:
         if (numSequences > 1 && Index == BeginIndex) {
            if (!toMarks.Empty())
               toMarks.Add(toIndex->Last());
            toMarks.Add(toIndex->Last());
            toMarks.Save();
            }
         }
      else
         return false;
      }
  return true;
}

void* cCuttingThread::Process(void)
{
  if (cMark *BeginMark = fromMarks.GetNextBegin()) {
     fromFile = fromFileName->Open();
     toFile = toFileName->Open();
     if (!fromFile || !toFile)
        return NULL;
     int LastEndIndex = -1;
     while (BeginMark && !IsStopped()) {
           // Suspend cutting if we have severe throughput problems:
           if (Throttled()) {
             CEvent::Sleep(100);
              continue;
              }
           // Determine the actual begin and end marks, skipping any marks at the same position:
           cMark *EndMark = fromMarks.GetNextEnd(BeginMark);
           // Process the current sequence:
           int EndIndex = EndMark ? EndMark->Position() : fromIndex->Last() + 1;
           int NextBeginIndex = -1;
           if (EndMark) {
              if (cMark *NextBeginMark = fromMarks.GetNextBegin(EndMark))
                 NextBeginIndex = NextBeginMark->Position();
              }
           if (!ProcessSequence(LastEndIndex, BeginMark->Position(), EndIndex, NextBeginIndex))
              break;
           if (!EndMark)
              break; // reached EOF
           LastEndIndex = EndIndex;
           // Switch to the next sequence:
           BeginMark = fromMarks.GetNextBegin(EndMark);
           if (BeginMark) {
              // Split edited files:
              if (cSettings::Get().m_bSplitEditedFiles) {
                 if (!SwitchFile(true))
                    break;
                 }
              }
           }
     Recordings.TouchUpdate();
     }
  else
     esyslog("no editing marks found!");

  return NULL;
}

// --- cCutter ---------------------------------------------------------------

CMutex cCutter::mutex;
std::string cCutter::originalVersionName;
std::string cCutter::editedVersionName;
cCuttingThread *cCutter::cuttingThread = NULL;
bool cCutter::error = false;
bool cCutter::ended = false;

bool cCutter::Start(const char *FileName)
{
  CLockObject lock(mutex);
  if (!cuttingThread) {
     error = false;
     ended = false;
     originalVersionName = FileName;
     cRecording Recording(FileName);

     cMarks FromMarks;
     FromMarks.Load(FileName, Recording.FramesPerSecond(), Recording.IsPesRecording());
     if (cMark *First = FromMarks.GetNextBegin())
        Recording.SetStartTime(Recording.Start() + (int(First->Position() / Recording.FramesPerSecond() + 30) / 60) * 60);

     std::string strEvent = Recording.PrefixFileName('%');
     if (!strEvent.empty() && RemoveVideoFile(strEvent) && MakeDirs(strEvent, true)) {
        // XXX this can be removed once RenameVideoFile() follows symlinks (see videodir.c)
        // remove a possible deleted recording with the same name to avoid symlink mixups:
        char *s = strdup(strEvent.c_str());
        char *e = strrchr(s, '.');
        if (e) {
           if (strcmp(e, ".rec") == 0) {
              strcpy(e, ".del");
              RemoveVideoFile(s);
              }
           }
        free(s);
        // XXX
        editedVersionName = strEvent;
        Recording.WriteInfo();
        Recordings.AddByName(editedVersionName, false);
        cuttingThread = new cCuttingThread(FileName, editedVersionName);
        return true;
        }
     }
  return false;
}

void cCutter::Stop(void)
{
  CLockObject lock(mutex);
  bool Interrupted = cuttingThread && cuttingThread->IsRunning();
  const char *Error = cuttingThread ? cuttingThread->Error() : NULL;
  delete cuttingThread;
  cuttingThread = NULL;
  if ((Interrupted || Error) && !editedVersionName.empty()) {
     if (Interrupted)
        isyslog("editing process has been interrupted");
     if (Error)
        esyslog("ERROR: '%s' during editing process", Error);
//     if (cReplayControl::NowReplaying() && strcmp(cReplayControl::NowReplaying(), editedVersionName) == 0)
//        cControl::Shutdown();
     RemoveVideoFile(editedVersionName);
     Recordings.DelByName(editedVersionName);
     }
}

bool cCutter::Active(const char *FileName)
{
  CLockObject lock(mutex);
  if (cuttingThread) {
     if (cuttingThread->IsRunning())
        return !FileName || strcmp(FileName, originalVersionName.c_str()) == 0 || strcmp(FileName, editedVersionName.c_str()) == 0;
     error = cuttingThread->Error();
     Stop();
     if (!error)
        cRecordingUserCommand::Get().InvokeCommand(RUC_EDITEDRECORDING, editedVersionName, originalVersionName);
     originalVersionName.clear();
     editedVersionName.clear();
     ended = true;
     }
  return false;
}

bool cCutter::Error(void)
{
  CLockObject lock(mutex);
  bool result = error;
  error = false;
  return result;
}

bool cCutter::Ended(void)
{
  CLockObject lock(mutex);
  bool result = ended;
  ended = false;
  return result;
}

#define CUTTINGCHECKINTERVAL 500 // ms between checks for the active cutting process

bool CutRecording(const char *FileName)
{
  if (CDirectory::CanWrite(FileName)) {
     cRecording Recording(FileName);
     if (!Recording.Name().empty()) {
        cMarks Marks;
        if (Marks.Load(FileName, Recording.FramesPerSecond(), Recording.IsPesRecording()) && !Marks.Empty()) {
           if (Marks.GetNumSequences()) {
              if (cCutter::Start(FileName)) {
                 while (cCutter::Active())
                   CEvent::Sleep(CUTTINGCHECKINTERVAL);
                 return true;
                 }
              else
                 fprintf(stderr, "can't start editing process\n");
              }
           else
              fprintf(stderr, "'%s' has no editing sequences\n", FileName);
           }
        else
           fprintf(stderr, "'%s' has no editing marks\n", FileName);
        }
     else
        fprintf(stderr, "'%s' is not a recording\n", FileName);
     }
  else
     fprintf(stderr, "'%s' is not a directory\n", FileName);
  return false;
}

}
