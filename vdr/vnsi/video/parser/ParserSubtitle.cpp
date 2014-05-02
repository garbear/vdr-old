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

#include <stdlib.h>
#include <assert.h>

#include "ParserSubtitle.h"

namespace VDR
{

cParserSubtitle::cParserSubtitle(int pID, cTSStream *stream, sPtsWrap *ptsWrap, bool observePtsWraps)
 : cParser(pID, stream, ptsWrap, observePtsWraps)
{
  m_PesBufferInitialSize = 4000;
}

cParserSubtitle::~cParserSubtitle()
{

}

void cParserSubtitle::Parse(sStreamPacket *pkt)
{
  int l = m_PesBufferPtr;

  if (l >= m_PesPacketLength)
  {
    if (l < 2 || m_PesBuffer[0] != 0x20 || m_PesBuffer[1] != 0x00)
    {
      Reset();
      return;
    }

    if(m_PesBuffer[m_PesPacketLength-1] == 0xff)
    {
      pkt->id       = m_pID;
      pkt->data     = m_PesBuffer+2;
      pkt->size     = m_PesPacketLength-3;
      pkt->duration = 0;
      pkt->dts      = m_curDTS;
      pkt->pts      = m_curPTS;
    }

    m_PesBufferPtr = 0;
  }
}

}
