/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "PATScanner.h"
#include "channels/Channel.h"

#include <assert.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI_EXT;

cPatScanner::cPatScanner(iPatScannerCallback* callback)
 : m_callback(callback)
{
  assert(m_callback);

  Set(PID_PAT, TABLE_ID_PAT);
}

cPatScanner::~cPatScanner()
{
  Del(PID_PAT, TABLE_ID_PAT);
}

void cPatScanner::ProcessData(u_short Pid, u_char Tid, const u_char * Data, int Length)
{
  SI::PAT tsPAT(Data, false);
  if (!tsPAT.CheckCRCAndParse())
    return;

  //HEXDUMP(Data, Length);

  SI::PAT::Association assoc;
  for (SI::Loop::Iterator it; tsPAT.associationLoop.getNext(assoc, it);)
  {
    if (!assoc.getServiceId())
      continue;

    assert((bool)Channel()); // TODO

    ChannelPtr ch = ChannelPtr(new cChannel);
    ch->CopyTransponderData(*Channel());
    ch->SetId(0, tsPAT.getTransportStreamId(), assoc.getServiceId());
    ch->SetName(CHANNEL_NAME_UNKNOWN, "", "");

    m_callback->PatFoundChannel(ch, assoc.getPid());
  }
}
