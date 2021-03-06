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

#include "Remux.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceTrackSubsystem.h"
#include "libsi/si.h"
#include "libsi/section.h"
#include "libsi/descriptor.h"
#include "recordings/Recording.h"
#include "recordings/RecordingConfig.h"
#include "utils/CommonMacros.h"
#include "utils/I18N.h"
#include "utils/log/Log.h"
#include "utils/Shutdown.h"

#include <algorithm>
#include <vector>

namespace VDR
{

// Set these to 'true' for debug output:
static bool DebugPatPmt = false;
static bool DebugFrames = false;

#define dbgpatpmt(a...) if (DebugPatPmt) fprintf(stderr, a)
#define dbgframes(a...) if (DebugFrames) fprintf(stderr, a)

#define EMPTY_SCANNER (0xFFFFFFFF)

ePesHeader AnalyzePesHeader(const uint8_t *Data, int Count, int &PesPayloadOffset, bool *ContinuationHeader)
{
  if (Count < 7)
     return phNeedMoreData; // too short

  if ((Data[6] & 0xC0) == 0x80) { // MPEG 2
     if (Count < 9)
        return phNeedMoreData; // too short

     PesPayloadOffset = 6 + 3 + Data[8];
     if (Count < PesPayloadOffset)
        return phNeedMoreData; // too short

     if (ContinuationHeader)
        *ContinuationHeader = ((Data[6] == 0x80) && !Data[7] && !Data[8]);

     return phMPEG2; // MPEG 2
     }

  // check for MPEG 1 ...
  PesPayloadOffset = 6;

  // skip up to 16 stuffing bytes
  for (int i = 0; i < 16; i++) {
      if (Data[PesPayloadOffset] != 0xFF)
         break;

      if (Count <= ++PesPayloadOffset)
         return phNeedMoreData; // too short
      }

  // skip STD_buffer_scale/size
  if ((Data[PesPayloadOffset] & 0xC0) == 0x40) {
     PesPayloadOffset += 2;

     if (Count <= PesPayloadOffset)
        return phNeedMoreData; // too short
     }

  if (ContinuationHeader)
     *ContinuationHeader = false;

  if ((Data[PesPayloadOffset] & 0xF0) == 0x20) {
     // skip PTS only
     PesPayloadOffset += 5;
     }
  else if ((Data[PesPayloadOffset] & 0xF0) == 0x30) {
     // skip PTS and DTS
     PesPayloadOffset += 10;
     }
  else if (Data[PesPayloadOffset] == 0x0F) {
     // continuation header
     PesPayloadOffset++;

     if (ContinuationHeader)
        *ContinuationHeader = true;
     }
  else
     return phInvalid; // unknown

  if (Count < PesPayloadOffset)
     return phNeedMoreData; // too short

  return phMPEG1; // MPEG 1
}

#define VIDEO_STREAM_S   0xE0

// --- cRemux ----------------------------------------------------------------

void cRemux::SetBrokenLink(uint8_t *Data, int Length)
{
  int PesPayloadOffset = 0;
  if (AnalyzePesHeader(Data, Length, PesPayloadOffset) >= phMPEG1 && (Data[3] & 0xF0) == VIDEO_STREAM_S) {
     for (int i = PesPayloadOffset; i < Length - 7; i++) {
         if (Data[i] == 0 && Data[i + 1] == 0 && Data[i + 2] == 1 && Data[i + 3] == 0xB8) {
            if (!(Data[i + 7] & 0x40)) // set flag only if GOP is not closed
               Data[i + 7] |= 0x20;
            return;
            }
         }
     dsyslog("SetBrokenLink: no GOP header found in video packet");
     }
  else
     dsyslog("SetBrokenLink: no video packet in frame");
}

// --- Some TS handling tools ------------------------------------------------

void TsHidePayload(uint8_t *p)
{
  p[1] &= ~TS_PAYLOAD_START;
  p[3] |=  TS_ADAPT_FIELD_EXISTS;
  p[3] &= ~TS_PAYLOAD_EXISTS;
  p[4] = TS_SIZE - 5;
  p[5] = 0x00;
  memset(p + 6, 0xFF, TS_SIZE - 6);
}

void TsSetPcr(uint8_t *p, int64_t Pcr)
{
  if (TsHasAdaptationField(p)) {
     if (p[4] >= 7 && (p[5] & TS_ADAPT_PCR)) {
        int64_t b = Pcr / PCRFACTOR;
        int e = Pcr % PCRFACTOR;
        p[ 6] =  b >> 25;
        p[ 7] =  b >> 17;
        p[ 8] =  b >>  9;
        p[ 9] =  b >>  1;
        p[10] = (b <<  7) | (p[10] & 0x7E) | ((e >> 8) & 0x01);
        p[11] =  e;
        }
     }
}

int64_t TsGetPts(const uint8_t *p, int l)
{
  // Find the first packet with a PTS and use it:
  while (l > 0) {
        const uint8_t *d = p;
        if (TsPayloadStart(d) && TsGetPayload(&d) && PesHasPts(d))
           return PesGetPts(d);
        p += TS_SIZE;
        l -= TS_SIZE;
        }
  return -1;
}

int64_t TsGetDts(const uint8_t *p, int l)
{
  // Find the first packet with a DTS and use it:
  while (l > 0) {
        const uint8_t *d = p;
        if (TsPayloadStart(d) && TsGetPayload(&d) && PesHasDts(d))
           return PesGetDts(d);
        p += TS_SIZE;
        l -= TS_SIZE;
        }
  return -1;
}

void TsSetPts(uint8_t *p, int l, int64_t Pts)
{
  // Find the first packet with a PTS and use it:
  while (l > 0) {
        const uint8_t *d = p;
        if (TsPayloadStart(d) && TsGetPayload(&d) && PesHasPts(d)) {
           PesSetPts(const_cast<uint8_t*>(d), Pts);
           return;
           }
        p += TS_SIZE;
        l -= TS_SIZE;
        }
}

void TsSetDts(uint8_t *p, int l, int64_t Dts)
{
  // Find the first packet with a DTS and use it:
  while (l > 0) {
        const uint8_t *d = p;
        if (TsPayloadStart(d) && TsGetPayload(&d) && PesHasDts(d)) {
           PesSetDts(const_cast<uint8_t*>(d), Dts);
           return;
           }
        p += TS_SIZE;
        l -= TS_SIZE;
        }
}

// --- Some PES handling tools -----------------------------------------------

void PesSetPts(uint8_t *p, int64_t Pts)
{
  p[ 9] = ((Pts >> 29) & 0x0E) | (p[9] & 0xF1);
  p[10] =   Pts >> 22;
  p[11] = ((Pts >> 14) & 0xFE) | 0x01;
  p[12] =   Pts >>  7;
  p[13] = ((Pts <<  1) & 0xFE) | 0x01;
}

void PesSetDts(uint8_t *p, int64_t Dts)
{
  p[14] = ((Dts >> 29) & 0x0E) | (p[14] & 0xF1);
  p[15] =   Dts >> 22;
  p[16] = ((Dts >> 14) & 0xFE) | 0x01;
  p[17] =   Dts >>  7;
  p[18] = ((Dts <<  1) & 0xFE) | 0x01;
}

int64_t PtsDiff(int64_t Pts1, int64_t Pts2)
{
  int64_t d = Pts2 - Pts1;
  if (d > MAX33BIT / 2)
     return d - (MAX33BIT + 1);
  if (d < -MAX33BIT / 2)
     return d + (MAX33BIT + 1);
  return d;
}

// --- cTsPayload ------------------------------------------------------------

cTsPayload::cTsPayload(void)
{
  data = NULL;
  length = 0;
  pid = -1;
  index = 0;
}

cTsPayload::cTsPayload(uint8_t *Data, int Length, int Pid)
{
  Setup(Data, Length, Pid);
}

void cTsPayload::Setup(uint8_t *Data, int Length, int Pid)
{
  data = Data;
  length = Length;
  pid = Pid >= 0 ? Pid : TsPid(Data);
  index = 0;
}

uint8_t cTsPayload::GetByte(void)
{
  if (!Eof()) {
     if (index % TS_SIZE == 0) { // encountered the next TS header
        for (;; index += TS_SIZE) {
            if (data[index] == TS_SYNC_BYTE && index + TS_SIZE <= length) { // to make sure we are at a TS header start and drop incomplete TS packets at the end
               uint8_t *p = data + index;
               if (TsPid(p) == pid) { // only handle TS packets for the initial PID
                  if (TsHasPayload(p)) {
                     if (index > 0 && TsPayloadStart(p)) { // checking index to not skip the very first TS packet
                        length = index; // triggers EOF
                        return 0x00;
                        }
                     index += TsPayloadOffset(p);
                     break;
                     }
                  }
               }
            else {
               length = index; // triggers EOF
               return 0x00;
               }
           }
        }
     return data[index++];
     }
  return 0x00;
}

bool cTsPayload::SkipBytes(int Bytes)
{
  while (Bytes-- > 0)
        GetByte();
  return !Eof();
}

bool cTsPayload::SkipPesHeader(void)
{
  return SkipBytes(PesPayloadOffset(data + TsPayloadOffset(data)));
}

int cTsPayload::GetLastIndex(void)
{
  return index - 1;
}

void cTsPayload::SetByte(uint8_t Byte, int Index)
{
  if (Index >= 0 && Index < length)
     data[Index] = Byte;
}

bool cTsPayload::Find(uint32_t Code)
{
  int OldIndex = index;
  uint32_t Scanner = EMPTY_SCANNER;
  while (!Eof()) {
        Scanner = (Scanner << 8) | GetByte();
        if (Scanner == Code)
           return true;
        }
  index = OldIndex;
  return false;
}

// --- cPatPmtGenerator ------------------------------------------------------

cPatPmtGenerator::cPatPmtGenerator(ChannelPtr Channel /* = cChannel::EmptyChannel */)
{
  numPmtPackets = 0;
  patCounter = pmtCounter = 0;
  patVersion = pmtVersion = 0;
  pmtPid = 0;
  esInfoLength = NULL;
  SetChannel(Channel);
}

void cPatPmtGenerator::IncCounter(int &Counter, uint8_t *TsPacket)
{
  TsPacket[3] = (TsPacket[3] & 0xF0) | Counter;
  if (++Counter > 0x0F)
     Counter = 0x00;
}

void cPatPmtGenerator::IncVersion(int &Version)
{
  if (++Version > 0x1F)
     Version = 0x00;
}

void cPatPmtGenerator::IncEsInfoLength(int Length)
{
  if (esInfoLength) {
     Length += ((*esInfoLength & 0x0F) << 8) | *(esInfoLength + 1);
     *esInfoLength = 0xF0 | (Length >> 8);
     *(esInfoLength + 1) = Length;
     }
}

int cPatPmtGenerator::MakeStream(uint8_t *Target, STREAM_TYPE Type, int Pid)
{
  int i = 0;
  Target[i++] = Type; // stream type
  Target[i++] = 0xE0 | (Pid >> 8); // dummy (3), pid hi (5)
  Target[i++] = Pid; // pid lo
  esInfoLength = &Target[i];
  Target[i++] = 0xF0; // dummy (4), ES info length hi
  Target[i++] = 0x00; // ES info length lo
  return i;
}

int cPatPmtGenerator::MakeAC3Descriptor(uint8_t *Target, STREAM_TYPE Type)
{
  int i = 0;
  Target[i++] = Type;
  Target[i++] = 0x01; // length
  Target[i++] = 0x00;
  IncEsInfoLength(i);
  return i;
}

int cPatPmtGenerator::MakeSubtitlingDescriptor(uint8_t *Target, const char *Language, uint8_t SubtitlingType, uint16_t CompositionPageId, uint16_t AncillaryPageId)
{
  int i = 0;
  Target[i++] = SI::SubtitlingDescriptorTag;
  Target[i++] = 0x08; // length
  Target[i++] = *Language++;
  Target[i++] = *Language++;
  Target[i++] = *Language++;
  Target[i++] = SubtitlingType;
  Target[i++] = CompositionPageId >> 8;
  Target[i++] = CompositionPageId & 0xFF;
  Target[i++] = AncillaryPageId >> 8;
  Target[i++] = AncillaryPageId & 0xFF;
  IncEsInfoLength(i);
  return i;
}

int cPatPmtGenerator::MakeLanguageDescriptor(uint8_t *Target, const char *Language)
{
  int i = 0;
  Target[i++] = SI::ISO639LanguageDescriptorTag;
  int Length = i++;
  Target[Length] = 0x00; // length
  for (const char *End = Language + strlen(Language); Language < End; ) {
      Target[i++] = *Language++;
      Target[i++] = *Language++;
      Target[i++] = *Language++;
      Target[i++] = 0x00;     // audio type
      Target[Length] += 0x04; // length
      if (*Language == '+')
         Language++;
      }
  IncEsInfoLength(i);
  return i;
}

int cPatPmtGenerator::MakeCRC(uint8_t *Target, const uint8_t *Data, int Length)
{
  int crc = SI::CRC32::crc32((const char *)Data, Length, 0xFFFFFFFF);
  int i = 0;
  Target[i++] = crc >> 24;
  Target[i++] = crc >> 16;
  Target[i++] = crc >> 8;
  Target[i++] = crc;
  return i;
}

#define P_TSID    0x8008 // pseudo TS ID
#define P_PMT_PID 0x0084 // pseudo PMT pid
#define MAXPID    0x2000 // the maximum possible number of pids

void cPatPmtGenerator::GeneratePmtPid(ChannelPtr Channel)
{
  bool Used[MAXPID] = { false };

  if (Channel->GetVideoStream().vpid < MAXPID)
    Used[Channel->GetVideoStream().vpid] = true;

  if (Channel->GetVideoStream().ppid < MAXPID)
    Used[Channel->GetVideoStream().ppid] = true;

  if (Channel->GetTeletextStream().tpid < MAXPID)
    Used[Channel->GetTeletextStream().tpid] = true;

  for (std::vector<AudioStream>::const_iterator it = Channel->GetAudioStreams().begin(); it != Channel->GetAudioStreams().end(); ++it)
  {
    if (it->apid < MAXPID)
      Used[it->apid] = true;
  }

  for (std::vector<DataStream>::const_iterator it = Channel->GetDataStreams().begin(); it != Channel->GetDataStreams().end(); ++it)
  {
    if (it->dpid < MAXPID)
      Used[it->dpid] = true;
  }

  for (std::vector<SubtitleStream>::const_iterator it = Channel->GetSubtitleStreams().begin(); it != Channel->GetSubtitleStreams().end(); ++it)
  {
    if (it->spid < MAXPID)
      Used[it->spid] = true;
  }

  for (pmtPid = P_PMT_PID; Used[pmtPid]; pmtPid++)
    ;
}

void cPatPmtGenerator::GeneratePat(void)
{
  memset(pat, 0xFF, sizeof(pat));
  uint8_t *p = pat;
  int i = 0;
  p[i++] = TS_SYNC_BYTE; // TS indicator
  p[i++] = TS_PAYLOAD_START | (PATPID >> 8); // flags (3), pid hi (5)
  p[i++] = PATPID & 0xFF; // pid lo
  p[i++] = 0x10; // flags (4), continuity counter (4)
  p[i++] = 0x00; // pointer field (payload unit start indicator is set)
  int PayloadStart = i;
  p[i++] = 0x00; // table id
  p[i++] = 0xB0; // section syntax indicator (1), dummy (3), section length hi (4)
  int SectionLength = i;
  p[i++] = 0x00; // section length lo (filled in later)
  p[i++] = P_TSID >> 8;   // TS id hi
  p[i++] = P_TSID & 0xFF; // TS id lo
  p[i++] = 0xC1 | (patVersion << 1); // dummy (2), version number (5), current/next indicator (1)
  p[i++] = 0x00; // section number
  p[i++] = 0x00; // last section number
  p[i++] = pmtPid >> 8;   // program number hi
  p[i++] = pmtPid & 0xFF; // program number lo
  p[i++] = 0xE0 | (pmtPid >> 8); // dummy (3), PMT pid hi (5)
  p[i++] = pmtPid & 0xFF; // PMT pid lo
  pat[SectionLength] = i - SectionLength - 1 + 4; // -2 = SectionLength storage, +4 = length of CRC
  MakeCRC(pat + i, pat + PayloadStart, i - PayloadStart);
  IncVersion(patVersion);
}

void cPatPmtGenerator::GeneratePmt(ChannelPtr Channel)
{
  // generate the complete PMT section:
  uint8_t buf[MAX_SECTION_SIZE];
  memset(buf, 0xFF, sizeof(buf));
  numPmtPackets = 0;
  if (Channel) {
     int Vpid = Channel->GetVideoStream().vpid;
     int Ppid = Channel->GetVideoStream().ppid;
     uint8_t *p = buf;
     int i = 0;
     p[i++] = 0x02; // table id
     int SectionLength = i;
     p[i++] = 0xB0; // section syntax indicator (1), dummy (3), section length hi (4)
     p[i++] = 0x00; // section length lo (filled in later)
     p[i++] = pmtPid >> 8;   // program number hi
     p[i++] = pmtPid & 0xFF; // program number lo
     p[i++] = 0xC1 | (pmtVersion << 1); // dummy (2), version number (5), current/next indicator (1)
     p[i++] = 0x00; // section number
     p[i++] = 0x00; // last section number
     p[i++] = 0xE0 | (Ppid >> 8); // dummy (3), PCR pid hi (5)
     p[i++] = Ppid; // PCR pid lo
     p[i++] = 0xF0; // dummy (4), program info length hi (4)
     p[i++] = 0x00; // program info length lo

     if (Vpid)
        i += MakeStream(buf + i, Channel->GetVideoStream().vtype, Vpid);

     for (std::vector<AudioStream>::const_iterator it = Channel->GetAudioStreams().begin(); it != Channel->GetAudioStreams().end(); ++it)
     {
       i += MakeStream(buf + i, it->atype, it->apid);
       i += MakeLanguageDescriptor(buf + i, it->alang.c_str());
     }

     for (std::vector<DataStream>::const_iterator it = Channel->GetDataStreams().begin(); it != Channel->GetDataStreams().end(); ++it)
     {
       i += MakeStream(buf + i, STREAM_TYPE_13818_PES_PRIVATE, it->dpid);
       i += MakeAC3Descriptor(buf + i, it->dtype);
       i += MakeLanguageDescriptor(buf + i, it->dlang.c_str());
     }

     for (std::vector<SubtitleStream>::const_iterator it = Channel->GetSubtitleStreams().begin(); it != Channel->GetSubtitleStreams().end(); ++it)
     {
       i += MakeStream(buf + i, STREAM_TYPE_13818_PES_PRIVATE, it->spid);
       i += MakeSubtitlingDescriptor(buf + i, it->slang.c_str(), it->subtitlingType, it->compositionPageId, it->ancillaryPageId);
     }

     int sl = i - SectionLength - 2 + 4; // -2 = SectionLength storage, +4 = length of CRC
     buf[SectionLength] |= (sl >> 8) & 0x0F;
     buf[SectionLength + 1] = sl;
     MakeCRC(buf + i, buf, i);
     // split the PMT section into several TS packets:
     uint8_t *q = buf;
     bool pusi = true;
     while (i > 0) {
           uint8_t *p = pmt[numPmtPackets++];
           int j = 0;
           p[j++] = TS_SYNC_BYTE; // TS indicator
           p[j++] = (pusi ? TS_PAYLOAD_START : 0x00) | (pmtPid >> 8); // flags (3), pid hi (5)
           p[j++] = pmtPid & 0xFF; // pid lo
           p[j++] = 0x10; // flags (4), continuity counter (4)
           if (pusi) {
              p[j++] = 0x00; // pointer field (payload unit start indicator is set)
              pusi = false;
              }
           int l = TS_SIZE - j;
           memcpy(p + j, q, l);
           q += l;
           i -= l;
           }
     IncVersion(pmtVersion);
     }
}

void cPatPmtGenerator::SetVersions(int PatVersion, int PmtVersion)
{
  patVersion = PatVersion & 0x1F;
  pmtVersion = PmtVersion & 0x1F;
}

void cPatPmtGenerator::SetChannel(ChannelPtr Channel)
{
  if (Channel)
  {
    GeneratePmtPid(Channel);
    GeneratePat();
    GeneratePmt(Channel);
   }
}

uint8_t *cPatPmtGenerator::GetPat(void)
{
  IncCounter(patCounter, pat);
  return pat;
}

uint8_t *cPatPmtGenerator::GetPmt(int &Index)
{
  if (Index < numPmtPackets) {
     IncCounter(pmtCounter, pmt[Index]);
     return pmt[Index++];
     }
  return NULL;
}

// --- cPatPmtParser ---------------------------------------------------------

cPatPmtParser::cPatPmtParser(void)
{
  Reset();
}

void cPatPmtParser::Reset(void)
{
  pmtSize = 0;
  patVersion = pmtVersion = -1;
  pmtPids[0] = 0;
  vpid = vtype = STREAM_TYPE_UNDEFINED;
  ppid = 0;
}

void cPatPmtParser::ParsePat(const uint8_t *Data, int Length)
{
  // Unpack the TS packet:
  int PayloadOffset = TsPayloadOffset(Data);
  Data += PayloadOffset;
  Length -= PayloadOffset;
  // The PAT is always assumed to fit into a single TS packet
  if ((Length -= Data[0] + 1) <= 0)
     return;
  Data += Data[0] + 1; // process pointer_field
  SI::PAT Pat(Data);
  if (Pat.CheckAndParse()) {
     dbgpatpmt("PAT: TSid = %d, c/n = %d, v = %d, s = %d, ls = %d\n", Pat.getTransportStreamId(), Pat.getCurrentNextIndicator(), Pat.getVersionNumber(), Pat.getSectionNumber(), Pat.getLastSectionNumber());
     if (patVersion == Pat.getVersionNumber())
        return;
     int NumPmtPids = 0;
     SI::PAT::Association assoc;
     for (SI::Loop::Iterator it; Pat.associationLoop.getNext(assoc, it); ) {
         dbgpatpmt("     isNITPid = %d\n", assoc.isNITPid());
         if (!assoc.isNITPid()) {
            if (NumPmtPids <= MAX_PMT_PIDS)
               pmtPids[NumPmtPids++] = assoc.getPid();
            dbgpatpmt("     service id = %d, pid = %d\n", assoc.getServiceId(), assoc.getPid());
            }
         }
     pmtPids[NumPmtPids] = 0;
     patVersion = Pat.getVersionNumber();
     }
  else
     esyslog("ERROR: can't parse PAT");
}

void cPatPmtParser::ParsePmt(const uint8_t *Data, int Length)
{
  // Unpack the TS packet:
  bool PayloadStart = TsPayloadStart(Data);
  int PayloadOffset = TsPayloadOffset(Data);
  Data += PayloadOffset;
  Length -= PayloadOffset;
  // The PMT may extend over several TS packets, so we need to assemble them
  if (PayloadStart) {
     pmtSize = 0;
     if ((Length -= Data[0] + 1) <= 0)
        return;
     Data += Data[0] + 1; // this is the first packet
     if (SectionLength(Data, Length) > Length) {
        if (Length <= int(sizeof(pmt))) {
           memcpy(pmt, Data, Length);
           pmtSize = Length;
           }
        else
           esyslog("ERROR: PMT packet length too big (%d byte)!", Length);
        return;
        }
     // the packet contains the entire PMT section, so we run into the actual parsing
     }
  else if (pmtSize > 0) {
     // this is a following packet, so we add it to the pmt storage
     if (Length <= int(sizeof(pmt)) - pmtSize) {
        memcpy(pmt + pmtSize, Data, Length);
        pmtSize += Length;
        }
     else {
        esyslog("ERROR: PMT section length too big (%d byte)!", pmtSize + Length);
        pmtSize = 0;
        }
     if (SectionLength(pmt, pmtSize) > pmtSize)
        return; // more packets to come
     // the PMT section is now complete, so we run into the actual parsing
     Data = pmt;
     }
  else
     return; // fragment of broken packet - ignore
  SI::PMT Pmt(Data);
  if (Pmt.CheckAndParse()) {
     dbgpatpmt("PMT: sid = %d, c/n = %d, v = %d, s = %d, ls = %d\n", Pmt.getServiceId(), Pmt.getCurrentNextIndicator(), Pmt.getVersionNumber(), Pmt.getSectionNumber(), Pmt.getLastSectionNumber());
     dbgpatpmt("     pcr = %d\n", Pmt.getPCRPid());
     if (pmtVersion == Pmt.getVersionNumber())
        return;
     int NumApids = 0;
     int NumDpids = 0;
     int NumSpids = 0;
     vpid = vtype = STREAM_TYPE_UNDEFINED;
     ppid = 0;
     apids[0] = 0;
     dpids[0] = 0;
     spids[0] = 0;
     atypes[0] = STREAM_TYPE_UNDEFINED;
     dtypes[0] = STREAM_TYPE_UNDEFINED;
     SI::PMT::Stream stream;
     for (SI::Loop::Iterator it; Pmt.streamLoop.getNext(stream, it); ) {
         dbgpatpmt("     stream type = %02X, pid = %d", stream.getStreamType(), stream.getPid());
         switch (stream.getStreamType()) {
           case STREAM_TYPE_11172_VIDEO:
           case STREAM_TYPE_13818_VIDEO:
           case STREAM_TYPE_14496_H264_VIDEO: // H.264
                      vpid = stream.getPid();
                      vtype = (STREAM_TYPE)stream.getStreamType();
                      ppid = Pmt.getPCRPid();
                      break;
           case STREAM_TYPE_11172_AUDIO:
           case STREAM_TYPE_13818_AUDIO:
           case STREAM_TYPE_13818_AUDIO_ADTS: // ISO/IEC 13818-7 Audio with ADTS transport syntax
           case STREAM_TYPE_14496_AUDIO_LATM: // ISO/IEC 14496-3 Audio with LATM transport syntax
                      {
                      if (NumApids < MAXAPIDS) {
                         apids[NumApids] = stream.getPid();
                         atypes[NumApids] = (STREAM_TYPE)stream.getStreamType();
                         *alangs[NumApids] = 0;
                         SI::Descriptor *d;
                         for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                             switch (d->getDescriptorTag()) {
                               case SI::ISO639LanguageDescriptorTag: {
                                    SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                    SI::ISO639LanguageDescriptor::Language l;
                                    char *s = alangs[NumApids];
                                    int n = 0;
                                    for (SI::Loop::Iterator it; ld->languageLoop.getNext(l, it); ) {
                                        if (*ld->languageCode != '-') { // some use "---" to indicate "none"
                                           dbgpatpmt(" '%s'", l.languageCode);
                                           if (n > 0)
                                              *s++ = '+';
                                           strn0cpy(s, I18nNormalizeLanguageCode(l.languageCode), MAXLANGCODE1);
                                           s += strlen(s);
                                           if (n++ > 1)
                                              break;
                                           }
                                        }
                                    }
                                    break;
                               default: ;
                               }
                             delete d;
                             }
                         NumApids++;
                         apids[NumApids] = 0;
                         }
                      }
                      break;
           case STREAM_TYPE_13818_PES_PRIVATE:
                      {
                      int dpid = 0;
                      STREAM_TYPE dtype = STREAM_TYPE_UNDEFINED;
                      char lang[MAXLANGCODE1] = "";
                      SI::Descriptor *d;
                      for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                          switch (d->getDescriptorTag()) {
                            case SI::AC3DescriptorTag:
                            case SI::EnhancedAC3DescriptorTag:
                                 dbgpatpmt(" AC3");
                                 dpid = stream.getPid();
                                 dtype = (STREAM_TYPE)d->getDescriptorTag(); // TODO: STREAM_TYPE or DESCRIPTOR_TAG???
                                 break;
                            case SI::SubtitlingDescriptorTag:
                                 dbgpatpmt(" subtitling");
                                 if (NumSpids < MAXSPIDS) {
                                    spids[NumSpids] = stream.getPid();
                                    *slangs[NumSpids] = 0;
                                    subtitlingTypes[NumSpids] = 0;
                                    compositionPageIds[NumSpids] = 0;
                                    ancillaryPageIds[NumSpids] = 0;
                                    SI::SubtitlingDescriptor *sd = (SI::SubtitlingDescriptor *)d;
                                    SI::SubtitlingDescriptor::Subtitling sub;
                                    char *s = slangs[NumSpids];
                                    int n = 0;
                                    for (SI::Loop::Iterator it; sd->subtitlingLoop.getNext(sub, it); ) {
                                        if (sub.languageCode[0]) {
                                           dbgpatpmt(" '%s'", sub.languageCode);
                                           subtitlingTypes[NumSpids] = sub.getSubtitlingType();
                                           compositionPageIds[NumSpids] = sub.getCompositionPageId();
                                           ancillaryPageIds[NumSpids] = sub.getAncillaryPageId();
                                           if (n > 0)
                                              *s++ = '+';
                                           strn0cpy(s, I18nNormalizeLanguageCode(sub.languageCode), MAXLANGCODE1);
                                           s += strlen(s);
                                           if (n++ > 1)
                                              break;
                                           }
                                        }
                                    NumSpids++;
                                    spids[NumSpids] = 0;
                                    }
                                 break;
                            case SI::ISO639LanguageDescriptorTag: {
                                 SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                 dbgpatpmt(" '%s'", ld->languageCode);
                                 strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                                 }
                                 break;
                            default: ;
                            }
                          delete d;
                          }
                      if (dpid) {
                         if (NumDpids < MAXDPIDS) {
                            dpids[NumDpids] = dpid;
                            dtypes[NumDpids] = dtype;
                            strn0cpy(dlangs[NumDpids], lang, sizeof(dlangs[NumDpids]));
                            NumDpids++;
                            dpids[NumDpids] = 0;
                            }
                         }
                      }
                      break;
           case STREAM_TYPE_13818_USR_PRIVATE_81:
                      {
                      dbgpatpmt(" AC3");
                      char lang[MAXLANGCODE1] = { 0 };
                      SI::Descriptor *d;
                      for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                          switch (d->getDescriptorTag()) {
                            case SI::ISO639LanguageDescriptorTag: {
                                 SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                 dbgpatpmt(" '%s'", ld->languageCode);
                                 strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                                 }
                                 break;
                            default: ;
                            }
                         delete d;
                         }
                      if (NumDpids < MAXDPIDS) {
                         dpids[NumDpids] = stream.getPid();
                         dtypes[NumDpids] = (STREAM_TYPE)SI::AC3DescriptorTag; // TODO: STREAM_TYPE or DESCRIPTOR_TAG???
                         strn0cpy(dlangs[NumDpids], lang, sizeof(dlangs[NumDpids]));
                         NumDpids++;
                         dpids[NumDpids] = 0;
                         }
                      }
                      break;
           default:
             break;
           }
         dbgpatpmt("\n");
         }
     pmtVersion = Pmt.getVersionNumber();
     }
  else
     esyslog("ERROR: can't parse PMT");
  pmtSize = 0;
}

bool cPatPmtParser::ParsePatPmt(const uint8_t *Data, int Length)
{
  while (Length >= TS_SIZE) {
        if (*Data != TS_SYNC_BYTE)
           break; // just for safety
        int Pid = TsPid(Data);
        if (Pid == PATPID)
           ParsePat(Data, TS_SIZE);
        else if (IsPmtPid(Pid)) {
           ParsePmt(Data, TS_SIZE);
           if (patVersion >= 0 && pmtVersion >= 0)
              return true;
           }
        Data += TS_SIZE;
        Length -= TS_SIZE;
        }
  return false;
}

bool cPatPmtParser::GetVersions(int &PatVersion, int &PmtVersion) const
{
  PatVersion = patVersion;
  PmtVersion = pmtVersion;
  return patVersion >= 0 && pmtVersion >= 0;
}

// --- cTsToPes --------------------------------------------------------------

cTsToPes::cTsToPes(void)
{
  data = NULL;
  size = 0;
  Reset();
}

cTsToPes::~cTsToPes()
{
  free(data);
}

void cTsToPes::PutTs(const uint8_t *Data, int Length)
{
  if (TsError(Data)) {
     Reset();
     return; // ignore packets with TEI set, and drop any PES data collected so far
     }
  if (TsPayloadStart(Data))
     Reset();
  else if (!size)
     return; // skip everything before the first payload start
  Length = TsGetPayload(&Data);
  if (length + Length > size) {
     int NewSize = std::max(KILOBYTE(2), length + Length);
     if (uint8_t *NewData = (uint8_t*)realloc(data, NewSize)) {
        data = NewData;
        size = NewSize;
        }
     else {
        esyslog("ERROR: out of memory");
        Reset();
        return;
        }
     }
  memcpy(data + length, Data, Length);
  length += Length;
}

#define MAXPESLENGTH 0xFFF0

const uint8_t *cTsToPes::GetPes(int &Length)
{
  if (repeatLast) {
     repeatLast = false;
     Length = lastLength;
     return lastData;
     }
  if (offset < length && PesLongEnough(length)) {
     if (!PesHasLength(data)) // this is a video PES packet with undefined length
        offset = 6; // trigger setting PES length for initial slice
     if (offset) {
        uint8_t *p = data + offset - 6;
        if (p != data) {
           p -= 3;
           if (p < data) {
              Reset();
              return NULL;
              }
           memmove(p, data, 4);
           }
        int l = std::min(length - offset, MAXPESLENGTH);
        offset += l;
        if (p != data) {
           l += 3;
           p[6]  = 0x80;
           p[7]  = 0x00;
           p[8]  = 0x00;
           }
        p[4] = l / 256;
        p[5] = l & 0xFF;
        Length = l + 6;
        lastLength = Length;
        lastData = p;
        return p;
        }
     else {
        Length = PesLength(data);
        if (Length <= length) {
           offset = Length; // to make sure we break out in case of garbage data
           lastLength = Length;
           lastData = data;
           return data;
           }
        }
     }
  return NULL;
}

void cTsToPes::SetRepeatLast(void)
{
  repeatLast = true;
}

void cTsToPes::Reset(void)
{
  length = offset = 0;
  lastData = NULL;
  lastLength = 0;
  repeatLast = false;
}

// --- Some helper functions for debugging -----------------------------------

void BlockDump(const char *Name, const u_char *Data, int Length)
{
  printf("--- %s\n", Name);
  for (int i = 0; i < Length; i++) {
      if (i && (i % 16) == 0)
         printf("\n");
      printf(" %02X", Data[i]);
      }
  printf("\n");
}

void TsDump(const char *Name, const u_char *Data, int Length)
{
  printf("%s: %04X", Name, Length);
  int n = std::min(Length, 20);
  for (int i = 0; i < n; i++)
      printf(" %02X", Data[i]);
  if (n < Length) {
     printf(" ...");
     n = std::max(n, Length - 10);
     for (n = std::max(n, Length - 10); n < Length; n++)
         printf(" %02X", Data[n]);
     }
  printf("\n");
}

void PesDump(const char *Name, const u_char *Data, int Length)
{
  TsDump(Name, Data, Length);
}

// --- cFrameParser ----------------------------------------------------------

class cFrameParser {
protected:
  bool debug;
  bool newFrame;
  bool independentFrame;
public:
  cFrameParser(void);
  virtual ~cFrameParser() {};
  virtual int Parse(const uint8_t *Data, int Length, int Pid) = 0;
       ///< Parses the given Data, which is a sequence of Length bytes of TS packets.
       ///< The payload in the TS packets with the given Pid is searched for just
       ///< enough information to determine the beginning and type of the next video
       ///< frame.
       ///< Returns the number of bytes parsed. Upon return, the functions NewFrame()
       ///< and IndependentFrame() can be called to retrieve the required information.
  void SetDebug(bool Debug) { debug = Debug; }
  bool NewFrame(void) { return newFrame; }
  bool IndependentFrame(void) { return independentFrame; }
  };

cFrameParser::cFrameParser(void)
{
  debug = true;
  newFrame = false;
  independentFrame = false;
}

// --- cAudioParser ----------------------------------------------------------

class cAudioParser : public cFrameParser {
public:
  cAudioParser(void);
  virtual int Parse(const uint8_t *Data, int Length, int Pid);
  };

cAudioParser::cAudioParser(void)
{
}

int cAudioParser::Parse(const uint8_t *Data, int Length, int Pid)
{
  if (TsPayloadStart(Data)) {
     newFrame = independentFrame = true;
     if (debug)
        dbgframes("/");
     }
  else
     newFrame = independentFrame = false;
  return TS_SIZE;
}

// --- cMpeg2Parser ----------------------------------------------------------

class cMpeg2Parser : public cFrameParser {
private:
  uint32_t scanner;
  bool seenIndependentFrame;
public:
  cMpeg2Parser(void);
  virtual int Parse(const uint8_t *Data, int Length, int Pid);
  };

cMpeg2Parser::cMpeg2Parser(void)
{
  scanner = EMPTY_SCANNER;
  seenIndependentFrame = false;
}

int cMpeg2Parser::Parse(const uint8_t *Data, int Length, int Pid)
{
  newFrame = independentFrame = false;
  bool SeenPayloadStart = false;
  cTsPayload tsPayload(const_cast<uint8_t*>(Data), Length, Pid);
  if (TsPayloadStart(Data)) {
     SeenPayloadStart = true;
     tsPayload.SkipPesHeader();
     scanner = EMPTY_SCANNER;
     if (debug && seenIndependentFrame)
        dbgframes("/");
     }
  uint32_t OldScanner = scanner; // need to remember it in case of multiple frames per payload
  for (;;) {
      if (!SeenPayloadStart && tsPayload.AtTsStart())
         OldScanner = scanner;
      scanner = (scanner << 8) | tsPayload.GetByte();
      if (scanner == 0x00000100) { // Picture Start Code
         if (!SeenPayloadStart && tsPayload.GetLastIndex() > TS_SIZE) {
            scanner = OldScanner;
            return tsPayload.Used() - TS_SIZE;
            }
         newFrame = true;
         tsPayload.GetByte();
         uint8_t FrameType = (tsPayload.GetByte() >> 3) & 0x07;
         independentFrame = FrameType == 1; // I-Frame
         if (debug) {
            seenIndependentFrame |= independentFrame;
            if (seenIndependentFrame) {
               static const char FrameTypes[] = "?IPBD???";
               dbgframes("%c", FrameTypes[FrameType]);
               }
            }
         break;
         }
      if (tsPayload.AtPayloadStart() // stop at any new payload start to have the buffer refilled if necessary
         || (tsPayload.Available() < MIN_TS_PACKETS_FOR_FRAME_DETECTOR * TS_SIZE // stop if the available data is below the limit...
            && (tsPayload.Available() <= 0 || tsPayload.AtTsStart()))) // ...but only if there is no more data at all, or if we are at a TS boundary
         break;
      }
  return tsPayload.Used();
}

// --- cH264Parser -----------------------------------------------------------

class cH264Parser : public cFrameParser {
private:
  enum eNalUnitType {
    nutCodedSliceNonIdr     = 1,
    nutCodedSliceIdr        = 5,
    nutSequenceParameterSet = 7,
    nutAccessUnitDelimiter  = 9,
    };
  cTsPayload tsPayload;
  uint8_t byte; // holds the current byte value in case of bitwise access
  int bit; // the bit index into the current byte (-1 if we're not in bit reading mode)
  int zeroBytes; // the number of consecutive zero bytes (to detect 0x000003)
  uint32_t scanner;
  // Identifiers written in '_' notation as in "ITU-T H.264":
  bool separate_colour_plane_flag;
  int log2_max_frame_num;
  bool frame_mbs_only_flag;
  //
  bool gotAccessUnitDelimiter;
  bool gotSequenceParameterSet;
  uint8_t GetByte(bool Raw = false);
       ///< Gets the next data byte. If Raw is true, no filtering will be done.
       ///< With Raw set to false, if the byte sequence 0x000003 is encountered,
       ///< the byte with 0x03 will be skipped.
  uint8_t GetBit(void);
  uint32_t GetBits(int Bits);
  uint32_t GetGolombUe(void);
  int32_t GetGolombSe(void);
  void ParseAccessUnitDelimiter(void);
  void ParseSequenceParameterSet(void);
  void ParseSliceHeader(void);
public:
  cH264Parser(void);
       ///< Sets up a new H.264 parser.
       ///< This class parses only the data absolutely necessary to determine the
       ///< frame borders and field count of the given H264 material.
  virtual int Parse(const uint8_t *Data, int Length, int Pid);
  };

cH264Parser::cH264Parser(void)
{
  byte = 0;
  bit = -1;
  zeroBytes = 0;
  scanner = EMPTY_SCANNER;
  separate_colour_plane_flag = false;
  log2_max_frame_num = 0;
  frame_mbs_only_flag = false;
  gotAccessUnitDelimiter = false;
  gotSequenceParameterSet = false;
}

uint8_t cH264Parser::GetByte(bool Raw)
{
  uint8_t b = tsPayload.GetByte();
  if (!Raw) {
     // If we encounter the byte sequence 0x000003, we need to skip the 0x03:
     if (b == 0x00)
        zeroBytes++;
     else {
        if (b == 0x03 && zeroBytes >= 2)
           b = tsPayload.GetByte();
        zeroBytes = 0;
        }
     }
  else
     zeroBytes = 0;
  bit = -1;
  return b;
}

uint8_t cH264Parser::GetBit(void)
{
  if (bit < 0) {
     byte = GetByte();
     bit = 7;
     }
  return (byte & (1 << bit--)) ? 1 : 0;
}

uint32_t cH264Parser::GetBits(int Bits)
{
  uint32_t b = 0;
  while (Bits--)
        b |= GetBit() << Bits;
  return b;
}

uint32_t cH264Parser::GetGolombUe(void)
{
  int z = -1;
  for (int b = 0; !b; z++)
      b = GetBit();
  return (1 << z) - 1 + GetBits(z);
}

int32_t cH264Parser::GetGolombSe(void)
{
  uint32_t v = GetGolombUe();
  if (v) {
     if ((v & 0x01) != 0)
        return (v + 1) / 2; // fails for v == 0xFFFFFFFF, but that will probably never happen
     else
        return -int32_t(v / 2);
     }
  return v;
}

int cH264Parser::Parse(const uint8_t *Data, int Length, int Pid)
{
  newFrame = independentFrame = false;
  tsPayload.Setup(const_cast<uint8_t*>(Data), Length, Pid);
  if (TsPayloadStart(Data)) {
     tsPayload.SkipPesHeader();
     scanner = EMPTY_SCANNER;
     if (debug && gotSequenceParameterSet) {
        dbgframes("/");
        }
     }
  for (;;) {
      scanner = (scanner << 8) | GetByte(true);
      if ((scanner & 0xFFFFFF00) == 0x00000100) { // NAL unit start
         uint8_t NalUnitType = scanner & 0x1F;
         switch (NalUnitType) {
           case nutAccessUnitDelimiter:  ParseAccessUnitDelimiter();
                                         gotAccessUnitDelimiter = true;
                                         break;
           case nutSequenceParameterSet: ParseSequenceParameterSet();
                                         gotSequenceParameterSet = true;
                                         break;
           case nutCodedSliceNonIdr:
           case nutCodedSliceIdr:        if (gotAccessUnitDelimiter && gotSequenceParameterSet) {
                                            ParseSliceHeader();
                                            gotAccessUnitDelimiter = false;
                                            return tsPayload.Used();
                                            }
                                         break;
           default:
             break;
           }
         }
      if (tsPayload.AtPayloadStart() // stop at any new payload start to have the buffer refilled if necessary
         || (tsPayload.Available() < MIN_TS_PACKETS_FOR_FRAME_DETECTOR * TS_SIZE // stop if the available data is below the limit...
            && (tsPayload.Available() <= 0 || tsPayload.AtTsStart()))) // ...but only if there is no more data at all, or if we are at a TS boundary
         break;
      }
  return tsPayload.Used();
}

void cH264Parser::ParseAccessUnitDelimiter(void)
{
  if (debug && gotSequenceParameterSet)
     dbgframes("A");
  GetByte(); // primary_pic_type
}

void cH264Parser::ParseSequenceParameterSet(void)
{
  uint8_t profile_idc = GetByte(); // profile_idc
  GetByte(); // constraint_set[0-5]_flags, reserved_zero_2bits
  GetByte(); // level_idc
  GetGolombUe(); // seq_parameter_set_id
  if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc ==118 || profile_idc == 128) {
     int chroma_format_idc = GetGolombUe(); // chroma_format_idc
     if (chroma_format_idc == 3)
        separate_colour_plane_flag = GetBit();
     GetGolombUe(); // bit_depth_luma_minus8
     GetGolombUe(); // bit_depth_chroma_minus8
     GetBit(); // qpprime_y_zero_transform_bypass_flag
     if (GetBit()) { // seq_scaling_matrix_present_flag
        for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
            if (GetBit()) { // seq_scaling_list_present_flag
               int SizeOfScalingList = (i < 6) ? 16 : 64;
               int LastScale = 8;
               int NextScale = 8;
               for (int j = 0; j < SizeOfScalingList; j++) {
                   if (NextScale)
                      NextScale = (LastScale + GetGolombSe() + 256) % 256; // delta_scale
                   if (NextScale)
                      LastScale = NextScale;
                   }
               }
            }
        }
     }
  log2_max_frame_num = GetGolombUe() + 4; // log2_max_frame_num_minus4
  int pic_order_cnt_type = GetGolombUe(); // pic_order_cnt_type
  if (pic_order_cnt_type == 0)
     GetGolombUe(); // log2_max_pic_order_cnt_lsb_minus4
  else if (pic_order_cnt_type == 1) {
     GetBit(); // delta_pic_order_always_zero_flag
     GetGolombSe(); // offset_for_non_ref_pic
     GetGolombSe(); // offset_for_top_to_bottom_field
     for (int i = GetGolombUe(); i--; ) // num_ref_frames_in_pic_order_cnt_cycle
         GetGolombSe(); // offset_for_ref_frame
     }
  GetGolombUe(); // max_num_ref_frames
  GetBit(); // gaps_in_frame_num_value_allowed_flag
  GetGolombUe(); // pic_width_in_mbs_minus1
  GetGolombUe(); // pic_height_in_map_units_minus1
  frame_mbs_only_flag = GetBit(); // frame_mbs_only_flag
  if (debug) {
     if (gotAccessUnitDelimiter && !gotSequenceParameterSet)
        dbgframes("A"); // just for completeness
     dbgframes(frame_mbs_only_flag ? "S" : "s");
     }
}

void cH264Parser::ParseSliceHeader(void)
{
  newFrame = true;
  GetGolombUe(); // first_mb_in_slice
  int slice_type = GetGolombUe(); // slice_type, 0 = P, 1 = B, 2 = I, 3 = SP, 4 = SI
  independentFrame = (slice_type % 5) == 2;
  if (debug) {
     static const char SliceTypes[] = "PBIpi";
     dbgframes("%c", SliceTypes[slice_type % 5]);
     }
  if (frame_mbs_only_flag)
     return; // don't need the rest - a frame is complete
  GetGolombUe(); // pic_parameter_set_id
  if (separate_colour_plane_flag)
     GetBits(2); // colour_plane_id
  GetBits(log2_max_frame_num); // frame_num
  if (!frame_mbs_only_flag) {
     if (GetBit()) // field_pic_flag
        newFrame = !GetBit(); // bottom_field_flag
     if (debug)
        dbgframes(newFrame ? "t" : "b");
     }
}

// --- cFrameDetector --------------------------------------------------------

cFrameDetector::cFrameDetector(const ChannelPtr& channel)
 : synced(false),
   newFrame(false),
   independentFrame(false),
   ptsValues(),
   numPtsValues(0),
   numFrames(0),
   numIFrames(0),
   isVideo(false),
   framesPerSecond(0.0),
   framesInPayloadUnit(0),
   framesPerPayloadUnit(0),
   scanning(false),
   parser(NULL)
{
  SetChannel(channel);
}

static int CmpUint32(const void *p1, const void *p2)
{
  if (*(uint32_t *)p1 < *(uint32_t *)p2) return -1;
  if (*(uint32_t *)p1 > *(uint32_t *)p2) return  1;
  return 0;
}

void cFrameDetector::SetChannel(const ChannelPtr& channel)
{
  uint16_t    pid  = channel->GetVideoStream().vpid;
  STREAM_TYPE type = channel->GetVideoStream().vtype;

  if (pid == 0 && channel->GetAudioStream(0).apid != 0)
  {
    pid = channel->GetAudioStream(0).apid;
    type = STREAM_TYPE_13818_AUDIO;
  }

  if (pid == 0 && channel->GetDataStream(0).dpid != 0)
  {
    pid = channel->GetDataStream(0).dpid;
    type = STREAM_TYPE_13818_PES_PRIVATE;
  }

  SetPid(pid, type);
}

void cFrameDetector::SetPid(uint16_t pid, STREAM_TYPE type)
{
  m_pid = pid;
  isVideo = (type == STREAM_TYPE_11172_VIDEO     || // MPEG 1
             type == STREAM_TYPE_13818_VIDEO     || // MPEG 2
             type == STREAM_TYPE_14496_H264_VIDEO); // H.264

  delete parser;
  parser = NULL;

  if (type == STREAM_TYPE_11172_VIDEO ||
      type == STREAM_TYPE_13818_VIDEO)
  {
    parser = new cMpeg2Parser;
  }
  else if (type == STREAM_TYPE_14496_H264_VIDEO)
  {
    parser = new cH264Parser;
  }
  else if (type == STREAM_TYPE_13818_AUDIO     || // MPEG audio
           type == STREAM_TYPE_13818_PES_PRIVATE) // AC3 audio
  {
    parser = new cAudioParser;
  }
  else if (type != STREAM_TYPE_UNDEFINED)
  {
    esyslog("ERROR: unknown stream type %d (PID %d) in frame detector", type, pid);
  }
}

int cFrameDetector::Analyze(const uint8_t *Data, int Length)
{
  if (!parser)
     return 0;
  int Processed = 0;
  newFrame = independentFrame = false;
  while (Length >= MIN_TS_PACKETS_FOR_FRAME_DETECTOR * TS_SIZE) { // makes sure we are looking at enough data, in case the frame type is not stored in the first TS packet
        // Sync on TS packet borders:
        if (Data[0] != TS_SYNC_BYTE) {
           int Skipped = 1;
           while (Skipped < Length && (Data[Skipped] != TS_SYNC_BYTE || (Length - Skipped > TS_SIZE) && (Data[Skipped + TS_SIZE] != TS_SYNC_BYTE)))
                 Skipped++;
           esyslog("ERROR: skipped %d bytes to sync on start of TS packet", Skipped);
           return Processed + Skipped;
           }
        // Handle one TS packet:
        int Handled = TS_SIZE;
        if (TsHasPayload(Data) && !TsIsScrambled(Data)) {
           int Pid = TsPid(Data);
           if (Pid == m_pid) {
              if (Processed)
                 return Processed;
              if (TsPayloadStart(Data))
                 scanning = true;
              if (scanning) {
                 // Detect the beginning of a new frame:
                 if (TsPayloadStart(Data)) {
                    if (!framesPerPayloadUnit)
                       framesPerPayloadUnit = framesInPayloadUnit;
                    }
                 int n = parser->Parse(Data, Length, m_pid);
                 if (n > 0) {
                    if (parser->NewFrame()) {
                       newFrame = true;
                       independentFrame = parser->IndependentFrame();
                       if (synced) {
                          if (framesPerPayloadUnit <= 1)
                             scanning = false;
                          }
                       else {
                          framesInPayloadUnit++;
                          if (independentFrame)
                             numIFrames++;
                          }
                       }
                    Handled = n;
                    }
                 }
              if (TsPayloadStart(Data)) {
                 // Determine the frame rate from the PTS values in the PES headers:
                 if (framesPerSecond <= 0.0) {
                    // frame rate unknown, so collect a sequence of PTS values:
                    if (numPtsValues < 2 || (numPtsValues < MaxPtsValues) && (numIFrames < 2)) { // collect a sequence containing at least two I-frames
                       if (newFrame) { // only take PTS values at the beginning of a frame (in case if fields!)
                          const uint8_t *Pes = Data + TsPayloadOffset(Data);
                          if (numIFrames && PesHasPts(Pes)) {
                             ptsValues[numPtsValues] = PesGetPts(Pes);
                             // check for rollover:
                             if (numPtsValues && ptsValues[numPtsValues - 1] > 0xF0000000 && ptsValues[numPtsValues] < 0x10000000) {
                                dbgframes("#");
                                numPtsValues = 0;
                                numIFrames = 0;
                                }
                             else
                                numPtsValues++;
                             }
                          }
                       }
                    if (numPtsValues >= 2 && numIFrames >= 2) {
                       // find the smallest PTS delta:
                       qsort(ptsValues, numPtsValues, sizeof(uint32_t), CmpUint32);
                       numPtsValues--;
                       for (int i = 0; i < numPtsValues; i++)
                           ptsValues[i] = ptsValues[i + 1] - ptsValues[i];
                       qsort(ptsValues, numPtsValues, sizeof(uint32_t), CmpUint32);
                       uint32_t Delta = ptsValues[0] / framesPerPayloadUnit;
                       // determine frame info:
                       if (isVideo) {
                          if (std::abs((int)Delta - 3600) <= 1)
                             framesPerSecond = 25.0;
                          else if (Delta % 3003 == 0)
                             framesPerSecond = 30.0 / 1.001;
                          else if (std::abs((int)Delta - 1800) <= 1)
                             framesPerSecond = 50.0;
                          else if (Delta == 1501)
                             framesPerSecond = 60.0 / 1.001;
                          else {
                             framesPerSecond = DEFAULTFRAMESPERSECOND;
                             dsyslog("unknown frame delta (%d), assuming %5.2f fps", Delta, DEFAULTFRAMESPERSECOND);
                             }
                          }
                       else // audio
                          framesPerSecond = double(PTSTICKS) / Delta; // PTS of audio frames is always increasing
                       dbgframes("\nDelta = %d  FPS = %5.2f  FPPU = %d NF = %d\n", Delta, framesPerSecond, framesPerPayloadUnit, numPtsValues + 1);
                       synced = true;
                       parser->SetDebug(false);
                       }
                    }
                 }
              }
           else if (Pid == PATPID && synced && Processed)
              return Processed; // allow the caller to see any PAT packets
           }
        Data += Handled;
        Length -= Handled;
        Processed += Handled;
        if (newFrame)
           break;
        }
  return Processed;
}
}
