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

namespace VDR
{

class cRequestPacket
{
public:
  cRequestPacket(uint32_t requestID, uint32_t opcode, uint8_t* data, uint32_t dataLength);
  ~cRequestPacket();

  int  serverError();

  uint32_t  getDataLength()     { return userDataLength; }
  uint32_t  getChannelID()      { return channelID; }
  uint32_t  getRequestID()      { return requestID; }
  uint32_t  getStreamID()       { return streamID; }
  uint32_t  getFlag()           { return flag; }
  uint32_t  getOpCode()         { return opCode; }

  char*     extract_String();
  uint8_t   extract_U8();
  uint32_t  extract_U32();
  uint64_t  extract_U64();
  int64_t   extract_S64();
  int32_t   extract_S32();
  double    extract_Double();

  bool      end();

  // If you call this, the memory becomes yours. Free with free()
  uint8_t* getData();

private:
  uint8_t* userData;
  uint32_t userDataLength;
  uint32_t packetPos;
  uint32_t opCode;

  uint32_t channelID;

  uint32_t requestID;
  uint32_t streamID;

  uint32_t flag; // stream only

  bool ownBlock;
};

}
