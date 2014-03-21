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
#pragma once

#include "Types.h"
#include "dvb/Filter.h"
#include "Types.h"

#include <sys/types.h>

namespace VDR
{

#define INVALID_CHANNEL  0x4000 // TODO: Is this needed?

class iNitScannerCallback
{
public:
  virtual void NitFoundTransponder(ChannelPtr transponder, bool bOtherTable, int originalNid, int originalTid) = 0;
  virtual ~iNitScannerCallback() { }
};

class cNitScanner : public cFilter
{
public:
  cNitScanner(iNitScannerCallback* callback, bool bUseOtherTable = false);
  virtual ~cNitScanner() { }

protected:
  virtual void ProcessData(u_short Pid, u_char Tid, const u_char * Data, int Length);

private:
  int                  m_tableId;
  iNitScannerCallback* m_callback;
  cSectionSyncer       m_syncNit;
};

}
