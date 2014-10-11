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

#include "ScanReceiver.h"
#include "channels/ChannelTypes.h"
#include "PMT.h"

#include <stdint.h>

namespace VDR
{
class cPat : public cScanReceiver
{
public:
  cPat(cDevice* device);
  virtual ~cPat(void) { }

  void LockLost(void);
  void ReceivePacket(uint16_t pid, const uint8_t* data);
  bool WaitForScan(uint32_t iTimeout = TRANSPONDER_TIMEOUT);

  bool InATSC(void) const { return true; }
  bool InDVB(void) const { return true; }
  bool InChannelScan(void) const { return true; }

private:
  cPmt m_pmt;
};

}
