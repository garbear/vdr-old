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

#include "channels/Channel.h"
#include "channels/ChannelTypes.h"

#include <stdint.h>

#define MAXAPIDS 32 // audio
#define MAXDPIDS 16
#define MAXSPIDS 32 // subtitles
#define MAXCAIDS 12 // conditional access

namespace VDR
{

enum ePesHeader {
  phNeedMoreData = -1,
  phInvalid = 0,
  phMPEG1 = 1,
  phMPEG2 = 2
  };

ePesHeader AnalyzePesHeader(const uint8_t *Data, int Count, int &PesPayloadOffset, bool *ContinuationHeader = NULL);

class cRemux {
public:
  static void SetBrokenLink(uint8_t *Data, int Length);
  };

// Some TS handling tools.
// The following functions all take a pointer to one complete TS packet.

#define TS_SYNC_BYTE          0x47
#define TS_SIZE               188
#define TS_ERROR              0x80
#define TS_PAYLOAD_START      0x40
#define TS_TRANSPORT_PRIORITY 0x20
#define TS_PID_MASK_HI        0x1F
#define TS_SCRAMBLING_CONTROL 0xC0
#define TS_ADAPT_FIELD_EXISTS 0x20
#define TS_PAYLOAD_EXISTS     0x10
#define TS_CONT_CNT_MASK      0x0F
#define TS_ADAPT_DISCONT      0x80
#define TS_ADAPT_RANDOM_ACC   0x40 // would be perfect for detecting independent frames, but unfortunately not used by all broadcasters
#define TS_ADAPT_ELEM_PRIO    0x20
#define TS_ADAPT_PCR          0x10
#define TS_ADAPT_OPCR         0x08
#define TS_ADAPT_SPLICING     0x04
#define TS_ADAPT_TP_PRIVATE   0x02
#define TS_ADAPT_EXTENSION    0x01

#define PATPID 0x0000 // PAT PID (constant 0)
#define MAXPID 0x2000 // for arrays that use a PID as the index

#define PTSTICKS  90000 // number of PTS ticks per second
#define PCRFACTOR 300 // conversion from 27MHz PCR extension to 90kHz PCR base
#define MAX33BIT  0x00000001FFFFFFFFLL // max. possible value with 33 bit
#define MAX27MHZ  ((MAX33BIT + 1) * PCRFACTOR - 1) // max. possible PCR value

inline bool TsHasPayload(const uint8_t *p)
{
  return p[3] & TS_PAYLOAD_EXISTS;
}

inline bool TsHasAdaptationField(const uint8_t *p)
{
  return p[3] & TS_ADAPT_FIELD_EXISTS;
}

inline bool TsPayloadStart(const uint8_t *p)
{
  return p[1] & TS_PAYLOAD_START;
}

inline bool TsError(const uint8_t *p)
{
  return p[1] & TS_ERROR;
}

inline uint16_t TsPid(const uint8_t *p)
{
  return (p[1] & TS_PID_MASK_HI) * 256 + p[2];
}

inline uint8_t TsTid(const uint8_t *p)
{
  return p[0] & 0xFF;
}

inline bool TsIsScrambled(const uint8_t *p)
{
  return p[3] & TS_SCRAMBLING_CONTROL;
}

inline uint8_t TsGetContinuityCounter(const uint8_t *p)
{
  return p[3] & TS_CONT_CNT_MASK;
}

inline void TsSetContinuityCounter(uint8_t *p, uint8_t Counter)
{
  p[3] = (p[3] & ~TS_CONT_CNT_MASK) | (Counter & TS_CONT_CNT_MASK);
}

inline int TsPayloadOffset(const uint8_t *p)
{
  int o = TsHasAdaptationField(p) ? p[4] + 5 : 4;
  return o <= TS_SIZE ? o : TS_SIZE;
}

inline int TsGetPayload(const uint8_t **p)
{
  if (TsHasPayload(*p)) {
     int o = TsPayloadOffset(*p);
     *p += o;
     return TS_SIZE - o;
     }
  return 0;
}

inline int TsContinuityCounter(const uint8_t *p)
{
  return p[3] & TS_CONT_CNT_MASK;
}

inline int64_t TsGetPcr(const uint8_t *p)
{
  if (TsHasAdaptationField(p)) {
     if (p[4] >= 7 && (p[5] & TS_ADAPT_PCR)) {
        return ((((int64_t)p[ 6]) << 25) |
                (((int64_t)p[ 7]) << 17) |
                (((int64_t)p[ 8]) <<  9) |
                (((int64_t)p[ 9]) <<  1) |
                (((int64_t)p[10]) >>  7)) * PCRFACTOR +
               (((((int)p[10]) & 0x01) << 8) |
                ( ((int)p[11])));
        }
     }
  return -1;
}

void TsHidePayload(uint8_t *p);
void TsSetPcr(uint8_t *p, int64_t Pcr);

// The following functions all take a pointer to a sequence of complete TS packets.

int64_t TsGetPts(const uint8_t *p, int l);
int64_t TsGetDts(const uint8_t *p, int l);
void TsSetPts(uint8_t *p, int l, int64_t Pts);
void TsSetDts(uint8_t *p, int l, int64_t Dts);

// Some PES handling tools:
// The following functions that take a pointer to PES data all assume that
// there is enough data so that PesLongEnough() returns true.

inline bool PesLongEnough(int Length)
{
  return Length >= 6;
}

inline bool PesHasLength(const uint8_t *p)
{
  return p[4] | p[5];
}

inline int PesLength(const uint8_t *p)
{
  return 6 + p[4] * 256 + p[5];
}

inline int PesPayloadOffset(const uint8_t *p)
{
  return 9 + p[8];
}

inline bool PesHasPts(const uint8_t *p)
{
  return (p[7] & 0x80) && p[8] >= 5;
}

inline bool PesHasDts(const uint8_t *p)
{
  return (p[7] & 0x40) && p[8] >= 10;
}

inline int64_t PesGetPts(const uint8_t *p)
{
  return ((((int64_t)p[ 9]) & 0x0E) << 29) |
         (( (int64_t)p[10])         << 22) |
         ((((int64_t)p[11]) & 0xFE) << 14) |
         (( (int64_t)p[12])         <<  7) |
         ((((int64_t)p[13]) & 0xFE) >>  1);
}

inline int64_t PesGetDts(const uint8_t *p)
{
  return ((((int64_t)p[14]) & 0x0E) << 29) |
         (( (int64_t)p[15])         << 22) |
         ((((int64_t)p[16]) & 0xFE) << 14) |
         (( (int64_t)p[17])         <<  7) |
         ((((int64_t)p[18]) & 0xFE) >>  1);
}

void PesSetPts(uint8_t *p, int64_t Pts);
void PesSetDts(uint8_t *p, int64_t Dts);

// PTS handling:

inline int64_t PtsAdd(int64_t Pts1, int64_t Pts2) { return (Pts1 + Pts2) & MAX33BIT; }
       ///< Adds the given PTS values, taking into account the 33bit wrap around.
int64_t PtsDiff(int64_t Pts1, int64_t Pts2);
       ///< Returns the difference between two PTS values. The result of Pts2 - Pts1
       ///< is the actual number of 90kHz time ticks that pass from Pts1 to Pts2,
       ///< properly taking into account the 33bit wrap around. If Pts2 is "before"
       ///< Pts1, the result is negative.

// A transprent TS payload handler:

class cTsPayload {
private:
  uint8_t *data;
  int length;
  int pid;
  int index; // points to the next byte to process
protected:
  void Reset(void) { index = 0; }
public:
  cTsPayload(void);
  cTsPayload(uint8_t *Data, int Length, int Pid = -1);
       ///< Creates a new TS payload handler and calls Setup() with the given Data.
  void Setup(uint8_t *Data, int Length, int Pid = -1);
       ///< Sets up this TS payload handler with the given Data, which points to a
       ///< sequence of Length bytes of complete TS packets. Any incomplete TS
       ///< packet at the end will be ignored.
       ///< If Pid is given, only TS packets with data for that PID will be processed.
       ///< Otherwise the PID of the first TS packet defines which payload will be
       ///< delivered.
       ///< Any intermediate TS packets with different PIDs will be skipped.
  bool AtTsStart(void) { return index < length && (index % TS_SIZE) == 0; }
       ///< Returns true if this payload handler is currently pointing to first byte
       ///< of a TS packet.
  bool AtPayloadStart(void) { return AtTsStart() && TsPayloadStart(data + index); }
       ///< Returns true if this payload handler is currently pointing to the first byte
       ///< of a TS packet that starts a new payload.
  int Available(void) { return length - index; }
       ///< Returns the number of raw bytes (including any TS headers) still available
       ///< in the TS payload handler.
  int Used(void) { return (index + TS_SIZE - 1) / TS_SIZE * TS_SIZE; }
       ///< Returns the number of raw bytes that have already been used (e.g. by calling
       ///< GetByte()). Any TS packet of which at least a single byte has been delivered
       ///< is counted with its full size.
  bool Eof(void) const { return index >= length; }
       ///< Returns true if all available bytes of the TS payload have been processed.
  uint8_t GetByte(void);
       ///< Gets the next byte of the TS payload, skipping any intermediate TS header data.
  bool SkipBytes(int Bytes);
       ///< Skips the given number of bytes in the payload and returns true if there
       ///< is still data left to read.
  bool SkipPesHeader(void);
       ///< Skips all bytes belonging to the PES header of the payload.
  int GetLastIndex(void);
       ///< Returns the index into the TS data of the payload byte that has most recently
       ///< been read. If no byte has been read yet, -1 will be returned.
  void SetByte(uint8_t Byte, int Index);
       ///< Sets the TS data byte at the given Index to the value Byte.
       ///< Index should be one that has been retrieved by a previous call to GetIndex(),
       ///< otherwise the behaviour is undefined. The current read index will not be
       ///< altered by a call to this function.
  bool Find(uint32_t Code);
       ///< Searches for the four byte sequence given in Code and returns true if it
       ///< was found within the payload data. The next call to GetByte() will return the
       ///< value immediately following the Code. If the code was not found, the read
       ///< index will remain the same as before this call, so that several calls to
       ///< Find() can be performed starting at the same index..
       ///< The special code 0xFFFFFFFF can not be searched, because this value is used
       ///< to initialize the scanner.
  };

// PAT/PMT Generator:

#define MAX_SECTION_SIZE 4096 // maximum size of an SI section
#define MAX_PMT_TS  (MAX_SECTION_SIZE / TS_SIZE + 1)

class cPatPmtGenerator {
private:
  uint8_t pat[TS_SIZE]; // the PAT always fits into a single TS packet
  uint8_t pmt[MAX_PMT_TS][TS_SIZE]; // the PMT may well extend over several TS packets
  int numPmtPackets;
  int patCounter;
  int pmtCounter;
  int patVersion;
  int pmtVersion;
  int pmtPid;
  uint8_t *esInfoLength;
  void IncCounter(int &Counter, uint8_t *TsPacket);
  void IncVersion(int &Version);
  void IncEsInfoLength(int Length);
protected:
  int MakeStream(uint8_t *Target, STREAM_TYPE Type, int Pid);
  int MakeAC3Descriptor(uint8_t *Target, STREAM_TYPE Type);
  int MakeSubtitlingDescriptor(uint8_t *Target, const char *Language, uint8_t SubtitlingType, uint16_t CompositionPageId, uint16_t AncillaryPageId);
  int MakeLanguageDescriptor(uint8_t *Target, const char *Language);
  int MakeCRC(uint8_t *Target, const uint8_t *Data, int Length);
  void GeneratePmtPid(ChannelPtr Channel);
       ///< Generates a PMT pid that doesn't collide with any of the actual
       ///< pids of the Channel.
  void GeneratePat(void);
       ///< Generates a PAT section for later use with GetPat().
  void GeneratePmt(ChannelPtr Channel);
       ///< Generates a PMT section for the given Channel, for later use
       ///< with GetPmt().
public:
  cPatPmtGenerator(ChannelPtr Channel = cChannel::EmptyChannel);
  void SetVersions(int PatVersion, int PmtVersion);
       ///< Sets the version numbers for the generated PAT and PMT, in case
       ///< this generator is used to, e.g.,  continue a previously interrupted
       ///< recording (in which case the numbers given should be derived from
       ///< the PAT/PMT versions last used in the existing recording, incremented
       ///< by 1. If the given numbers exceed the allowed range of 0..31, the
       ///< higher bits will automatically be cleared.
       ///< SetVersions() needs to be called before SetChannel() in order to
       ///< have an effect from the very start.
  void SetChannel(ChannelPtr Channel);
       ///< Sets the Channel for which the PAT/PMT shall be generated.
  uint8_t *GetPat(void);
       ///< Returns a pointer to the PAT section, which consists of exactly
       ///< one TS packet.
  uint8_t *GetPmt(int &Index);
       ///< Returns a pointer to the Index'th TS packet of the PMT section.
       ///< Index must be initialized to 0 and will be incremented by each
       ///< call to GetPmt(). Returns NULL is all packets of the PMT section
       ///< have been fetched..
  };

// PAT/PMT Parser:

#define MAX_PMT_PIDS 32

class cPatPmtParser {
private:
  uint8_t pmt[MAX_SECTION_SIZE];
  int pmtSize;
  int patVersion;
  int pmtVersion;
  int pmtPids[MAX_PMT_PIDS + 1]; // list is zero-terminated
  int vpid;
  int ppid;
  STREAM_TYPE vtype;
  int apids[MAXAPIDS + 1]; // list is zero-terminated
  STREAM_TYPE atypes[MAXAPIDS + 1]; // list is zero-terminated
  char alangs[MAXAPIDS][MAXLANGCODE2];
  int dpids[MAXDPIDS + 1]; // list is zero-terminated
  STREAM_TYPE dtypes[MAXDPIDS + 1]; // list is zero-terminated
  char dlangs[MAXDPIDS][MAXLANGCODE2];
  int spids[MAXSPIDS + 1]; // list is zero-terminated
  char slangs[MAXSPIDS][MAXLANGCODE2];
  uint8_t subtitlingTypes[MAXSPIDS];
  uint16_t compositionPageIds[MAXSPIDS];
  uint16_t ancillaryPageIds[MAXSPIDS];
protected:
  int SectionLength(const uint8_t *Data, int Length) { return (Length >= 3) ? ((int(Data[1]) & 0x0F) << 8)| Data[2] : 0; }
public:
  cPatPmtParser(void);
  void Reset(void);
       ///< Resets the parser. This function must be called whenever a new
       ///< stream is parsed.
  void ParsePat(const uint8_t *Data, int Length);
       ///< Parses the PAT data from the single TS packet in Data.
       ///< Length is always TS_SIZE.
  void ParsePmt(const uint8_t *Data, int Length);
       ///< Parses the PMT data from the single TS packet in Data.
       ///< Length is always TS_SIZE.
       ///< The PMT may consist of several TS packets, which
       ///< are delivered to the parser through several subsequent calls to
       ///< ParsePmt(). The whole PMT data will be processed once the last packet
       ///< has been received.
  bool ParsePatPmt(const uint8_t *Data, int Length);
       ///< Parses the given Data (which may consist of several TS packets, typically
       ///< an entire frame) and extracts the PAT and PMT.
       ///< Returns true if a valid PAT/PMT has been detected.
  bool GetVersions(int &PatVersion, int &PmtVersion) const;
       ///< Returns true if a valid PAT/PMT has been parsed and stores
       ///< the current version numbers in the given variables.
  bool IsPmtPid(int Pid) const { for (int i = 0; pmtPids[i]; i++) if (pmtPids[i] == Pid) return true; return false; }
       ///< Returns true if Pid the one of the PMT pids as defined by the current PAT.
       ///< If no PAT has been received yet, false will be returned.
  int Vpid(void) const { return vpid; }
       ///< Returns the video pid as defined by the current PMT, or 0 if no video
       ///< pid has been detected, yet.
  int Ppid(void) const { return ppid; }
       ///< Returns the PCR pid as defined by the current PMT, or 0 if no PCR
       ///< pid has been detected, yet.
  STREAM_TYPE Vtype(void) const { return vtype; }
       ///< Returns the video stream type as defined by the current PMT, or 0 if no video
       ///< stream type has been detected, yet.
  const int *Apids(void) const { return apids; }
  const int *Dpids(void) const { return dpids; }
  const int *Spids(void) const { return spids; }
  int Apid(int i) const { return (0 <= i && i < MAXAPIDS) ? apids[i] : 0; }
  int Dpid(int i) const { return (0 <= i && i < MAXDPIDS) ? dpids[i] : 0; }
  int Spid(int i) const { return (0 <= i && i < MAXSPIDS) ? spids[i] : 0; }
  STREAM_TYPE Atype(int i) const { return (0 <= i && i < MAXAPIDS) ? atypes[i] : STREAM_TYPE_UNDEFINED; }
  STREAM_TYPE Dtype(int i) const { return (0 <= i && i < MAXDPIDS) ? dtypes[i] : STREAM_TYPE_UNDEFINED; }
  const char *Alang(int i) const { return (0 <= i && i < MAXAPIDS) ? alangs[i] : ""; }
  const char *Dlang(int i) const { return (0 <= i && i < MAXDPIDS) ? dlangs[i] : ""; }
  const char *Slang(int i) const { return (0 <= i && i < MAXSPIDS) ? slangs[i] : ""; }
  uint8_t SubtitlingType(int i) const { return (0 <= i && i < MAXSPIDS) ? subtitlingTypes[i] : uint8_t(0); }
  uint16_t CompositionPageId(int i) const { return (0 <= i && i < MAXSPIDS) ? compositionPageIds[i] : uint16_t(0); }
  uint16_t AncillaryPageId(int i) const { return (0 <= i && i < MAXSPIDS) ? ancillaryPageIds[i] : uint16_t(0); }
  };

// TS to PES converter:
// Puts together the payload of several TS packets that form one PES
// packet.

class cTsToPes {
private:
  uint8_t *data;
  int size;
  int length;
  int offset;
  uint8_t *lastData;
  int lastLength;
  bool repeatLast;
public:
  cTsToPes(void);
  ~cTsToPes();
  void PutTs(const uint8_t *Data, int Length);
       ///< Puts the payload data of the single TS packet at Data into the converter.
       ///< Length is always TS_SIZE.
       ///< If the given TS packet starts a new PES payload packet, the converter
       ///< will be automatically reset. Any packets before the first one that starts
       ///< a new PES payload packet will be ignored.
       ///< Once a TS packet has been put into a cTsToPes converter, all subsequent
       ///< packets until the next call to Reset() must belong to the same PID as
       ///< the first packet. There is no check whether this actually is the case, so
       ///< the caller is responsible for making sure this condition is met.
  const uint8_t *GetPes(int &Length);
       ///< Gets a pointer to the complete PES packet, or NULL if the packet
       ///< is not complete yet. If the packet is complete, Length will contain
       ///< the total packet length. The returned pointer is only valid until
       ///< the next call to PutTs() or Reset(), or until this object is destroyed.
       ///< Once GetPes() has returned a non-NULL value, it must be called
       ///< repeatedly, and the data processed, until it returns NULL. This
       ///< is because video packets may be larger than the data a single
       ///< PES packet with an actual length field can hold, and are therefore
       ///< split into several PES packets with smaller sizes.
       ///< Note that for video data GetPes() may only be called if the next
       ///< TS packet that will be given to PutTs() has the "payload start" flag
       ///< set, because this is the only way to determine the end of a video PES
       ///< packet.
  void SetRepeatLast(void);
       ///< Makes the next call to GetPes() return exactly the same data as the
       ///< last one (provided there was no call to Reset() in the meantime).
  void Reset(void);
       ///< Resets the converter. This needs to be called after a PES packet has
       ///< been fetched by a call to GetPes(), and before the next call to
       ///< PutTs().
  };

// Some helper functions for debugging:

void BlockDump(const char *Name, const u_char *Data, int Length);
void TsDump(const char *Name, const u_char *Data, int Length);
void PesDump(const char *Name, const u_char *Data, int Length);

// Frame detector:

#define MIN_TS_PACKETS_FOR_FRAME_DETECTOR 5

class cFrameParser;

class cFrameDetector
{
private:
  enum { MaxPtsValues = 150 };
  uint16_t m_pid;
  bool synced;
  bool newFrame;
  bool independentFrame;
  uint32_t ptsValues[MaxPtsValues]; // 32 bit is enough - we only need the delta
  int numPtsValues;
  int numFrames;
  int numIFrames;
  bool isVideo;
  double framesPerSecond;
  int framesInPayloadUnit;
  int framesPerPayloadUnit; // Some broadcasters send one frame per payload unit (== 1),
                            // while others put an entire GOP into one payload unit (> 1).
  bool scanning;
  cFrameParser *parser;

public:
  /*!
   * Sets up a frame detector for the given channel. If no channel is given,
   * one needs to be set by a separate call to SetChannel().
   */
  cFrameDetector(const ChannelPtr& channel = cChannel::EmptyChannel);

  /*!
   * Sets the channel to detect frames for.
   */
  void SetChannel(const ChannelPtr& channel);
  void SetPid(uint16_t pid, STREAM_TYPE type);

  int Analyze(const uint8_t *Data, int Length);
      ///< Analyzes the TS packets pointed to by Data. Length is the number of
      ///< bytes Data points to, and must be a multiple of TS_SIZE.
      ///< Returns the number of bytes that have been analyzed.
      ///< If the return value is 0, the data was not sufficient for analyzing and
      ///< Analyze() needs to be called again with more actual data.
  bool Synced(void) { return synced; }
      ///< Returns true if the frame detector has synced on the data stream.
  bool NewFrame(void) { return newFrame; }
      ///< Returns true if the data given to the last call to Analyze() started a
      ///< new frame.
  bool IndependentFrame(void) { return independentFrame; }
      ///< Returns true if a new frame was detected and this is an independent frame
      ///< (i.e. one that can be displayed by itself, without using data from any
      ///< other frames).
  double FramesPerSecond(void) { return framesPerSecond; }
      ///< Returns the number of frames per second, or 0 if this information is not
      ///< available.
  };

}
