/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2007 Chris Tallon
 *      Portions Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Portions Copyright (C) 2010, 2011 Alexander Pipelka
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "Parser.h"
#include "Bitstream.h"

#include <stdint.h>
#include <sys/types.h>

namespace VDR
{

// --- cParserAAC -------------------------------------------------

class cParserAAC : public cParser
{
private:
  int         m_SampleRate;
  int         m_Channels;
  int         m_BitRate;
  int         m_FrameSize;

  int64_t     m_PTS;                /* pts of the current frame */
  int64_t     m_DTS;                /* dts of the current frame */

  bool        m_Configured;
  int         m_AudioMuxVersion_A;
  int         m_FrameLengthType;

  int FindHeaders(uint8_t *buf, int buf_size);
  bool ParseLATMAudioMuxElement(cBitstream *bs);
  void ReadStreamMuxConfig(cBitstream *bs);
  void ReadAudioSpecificConfig(cBitstream *bs);
  uint32_t LATMGetValue(cBitstream *bs) { return bs->readBits(bs->readBits(2) * 8); }

public:
  cParserAAC(int pID, cTSStream *stream, sPtsWrap *ptsWrap, bool observePtsWraps);
  virtual ~cParserAAC();
  virtual void Parse(sStreamPacket *pkt);
  virtual void Reset();
};

}
