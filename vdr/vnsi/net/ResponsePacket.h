/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2007 Chris Tallon
 *      Portions Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Portions Copyright (C) 2010, 2011 Alexander Pipelka
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

#include <string>

/*
 * This code is taken from VOMP for VDR plugin.
 */

namespace VDR
{

class cResponsePacket
{
public:
  cResponsePacket();
  ~cResponsePacket();

  bool init(uint32_t requestID);
  bool initScan(uint32_t opCode);
  bool initStatus(uint32_t opCode);
  bool initStream(uint32_t opCode, uint32_t streamID, uint32_t duration, int64_t pts, int64_t dts, uint32_t serial);
  void finalise();
  void finaliseStream();
  void finaliseOSD();
  bool copyin(const uint8_t* src, uint32_t len);
  uint8_t* reserve(uint32_t len);
  bool unreserve(uint32_t len);

  bool add_String(const std::string& string);
  bool add_U32(uint32_t ul);
  bool add_S32(int32_t l);
  bool add_U8(uint8_t c);
  bool add_U64(uint64_t ull);
  bool add_double(double d);

  uint8_t* getPtr() { return buffer; }
  uint32_t getLen() { return bufUsed; }
  uint32_t getStreamHeaderLength() { return headerLengthStream; } ;
  uint32_t getOSDHeaderLength() { return headerLengthOSD; } ;
  void     setLen(uint32_t len) { bufUsed = len; }

private:
  uint8_t* buffer;
  uint32_t bufSize;
  uint32_t bufUsed;

  void initBuffers();
  bool checkExtend(uint32_t by);

  const static uint32_t headerLength          = 12;
  const static uint32_t userDataLenPos        = 8;
  const static uint32_t headerLengthStream    = 40;
  const static uint32_t userDataLenPosStream  = 36;
  const static uint32_t headerLengthOSD       = 36;
  const static uint32_t userDataLenPosOSD     = 32;
};

}
