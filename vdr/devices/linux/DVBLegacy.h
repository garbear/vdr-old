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

#include <linux/dvb/version.h>

#define DVBAPIVERSION (DVB_API_VERSION << 8 | DVB_API_VERSION_MINOR)

#if DVBAPIVERSION < 0x0500
#error VDR requires Linux DVB driver API version 5.0 or higher!
#endif

// --- Definitions for older DVB API versions --------------------------------

#if DVBAPIVERSION < 0x0501
enum {
  FE_CAN_2G_MODULATION = 0x10000000,
  };
enum {
  TRANSMISSION_MODE_4K = TRANSMISSION_MODE_AUTO + 1,
  };
#endif

#if DVBAPIVERSION < 0x0502
enum {
  FE_CAN_TURBO_FEC = 0x8000000,
  };
#endif

#if DVBAPIVERSION < 0x0503
enum {
  TRANSMISSION_MODE_1K = TRANSMISSION_MODE_4K + 1,
  TRANSMISSION_MODE_16K,
  TRANSMISSION_MODE_32K,
  };
enum {
  GUARD_INTERVAL_1_128 = GUARD_INTERVAL_AUTO + 1,
  GUARD_INTERVAL_19_128,
  GUARD_INTERVAL_19_256,
  };
enum {
  SYS_DVBT2 = SYS_DAB + 1,
  };
#endif

#if DVBAPIVERSION < 0x0505
#define DTV_ENUM_DELSYS  44
#endif

#if DVBAPIVERSION < 0x0508
enum {
  FE_CAN_MULTISTREAM = 0x4000000,
  };
#define DTV_STREAM_ID            42
#define DTV_DVBT2_PLP_ID_LEGACY  43
#endif
