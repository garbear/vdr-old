/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#include "PSIP_STT.h"

#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

namespace VDR
{

cPsipStt::cPsipStt(cDevice* device)
 : cFilter(device)
{
  OpenResource(PID_STT, TableIdSTT);
}

unsigned int cPsipStt::GetGpsUtcOffset(void)
{
  // Static scope to cache return value
  static unsigned int offset = 0;

  // Skip reading the STT if offset has already been determined
  if (offset != 0)
    return offset;

  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  if (GetSection(pid, data))
  {
    SI::PSIP_STT stt(data.data(), false);
    if (stt.CheckCRCAndParse())
      offset = stt.getGpsUtcOffset();
  }

  return offset;
}

}