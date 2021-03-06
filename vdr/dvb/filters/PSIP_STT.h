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

namespace VDR
{

class cPsipStt : public cScanReceiver
{
public:
  cPsipStt(cDevice* device);
  virtual ~cPsipStt() { }

  void ReceivePacket(uint16_t pid, const uint8_t* data);

  /*!
   * Get the current number of leap-seconds between GPS and UTC time standards.
   * This is 15 seconds as of 1 January 2009.
   *
   * To convert GPS time to UTC, subtract the offset from GPS time. According to
   * A/65:2009, whenever the International Bureau of Weights and Measures
   * decides that the current offset is too far in error, an additional leap
   * second may be added (or subtracted). If an error or timeout occurred, this
   * returns 0.
   */
  unsigned int GetGpsUtcOffset(void) const { return m_iLastOffset; }

  bool InATSC(void) const { return true; }
  bool InDVB(void) const { return false; }

private:
  unsigned int m_iLastOffset;
};

}
