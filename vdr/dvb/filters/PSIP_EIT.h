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
#pragma once

#include "dvb/filters/Filter.h"
#include "epg/Event.h"

#include <map>
#include <stdint.h>

namespace VDR
{

class cPsipEit : public cFilter
{
public:
  cPsipEit(cDevice* device, const std::vector<uint16_t>& pids, unsigned int gpsUtcOffset);
  virtual ~cPsipEit(void) { }

  EventVector GetEvents();

private:
  std::vector<uint16_t> m_pidsLeftToScan; // Remove PIDs from this list as they are scanned
  unsigned int          m_gpsUtcOffset;   // Number of leap-seconds between GPS and UTC time systems
                                          // This is 15 seconds as of 1 January 2009
};

}
