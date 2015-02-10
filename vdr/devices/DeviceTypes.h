/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include <memory>
#include <vector>

namespace VDR
{

class cDevice;
typedef std::shared_ptr<cDevice> DevicePtr;
typedef std::vector<DevicePtr>   DeviceVector;

typedef uint8_t* TsPacket;


typedef enum {
  SIG_HAS_SIGNAL   = 0x01,
  SIG_HAS_CARRIER  = 0x02,
  SIG_HAS_VITERBI  = 0x04,
  SIG_HAS_SYNC     = 0x08,
  SIG_HAS_LOCK     = 0x10,
  SIG_TIMEDOUT     = 0x20,
  SIG_REINIT       = 0x40,
} signal_quality_status_t;

typedef struct {
  signal_quality_status_t status;
  std::string             status_string;
  int                     quality;
  int                     strength;
  std::string             name;

  uint16_t                snr;
  uint16_t                signal;
  uint32_t                ber;
  uint32_t                unc;
} signal_quality_info_t;

}
