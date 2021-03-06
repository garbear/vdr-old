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

#include "ParserTeletext.h"

namespace VDR
{

cParserTeletext::cParserTeletext(int pID, cTSStream *stream, sPtsWrap *ptsWrap, bool observePtsWraps)
 : cParser(pID, stream, ptsWrap, observePtsWraps)
{
  m_PesBufferInitialSize      = 4000;
}

cParserTeletext::~cParserTeletext()
{
}

void cParserTeletext::Parse(sStreamPacket *pkt)
{
  int l = m_PesBufferPtr;
  if (l < 1)
    return;

  if (m_PesBuffer[0] < 0x10 || m_PesBuffer[0] > 0x1F)
  {
    Reset();
    return;
  }

  if (l >= m_PesPacketLength)
  {
    pkt->id       = m_pID;
    pkt->data     = m_PesBuffer;
    pkt->size     = m_PesPacketLength;
    pkt->duration = 0;
    pkt->dts      = m_curDTS;
    pkt->pts      = m_curPTS;

    m_PesBufferPtr = 0;
  }
}

}
