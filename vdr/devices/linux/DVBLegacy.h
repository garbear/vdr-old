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

#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>

#define DVBAPIVERSION  (DVB_API_VERSION << 8 | DVB_API_VERSION_MINOR)

#if DVBAPIVERSION < 0x0500
#error Version 5 of the DVB driver API is required!
#endif

// --- Definitions for older DVB API versions --------------------------------

#if DVBAPIVERSION < 0x0501
#define FE_CAN_2G_MODULATION     ((fe_caps_t)0x10000000)
#define TRANSMISSION_MODE_4K     ((fe_transmit_mode_t)(TRANSMISSION_MODE_AUTO + 1))
#endif

#if DVBAPIVERSION < 0x0502
#define FE_CAN_TURBO_FEC         ((fe_caps_t)0x8000000)
#endif

#if DVBAPIVERSION < 0x0503
#define TRANSMISSION_MODE_1K     ((fe_transmit_mode_t)(TRANSMISSION_MODE_4K + 1))
#define TRANSMISSION_MODE_16K    ((fe_transmit_mode_t)(TRANSMISSION_MODE_4K + 2))
#define TRANSMISSION_MODE_32K    ((fe_transmit_mode_t)(TRANSMISSION_MODE_4K + 3))
#define GUARD_INTERVAL_1_128     ((fe_guard_interval_t)(GUARD_INTERVAL_AUTO + 1))
#define GUARD_INTERVAL_19_128    ((fe_guard_interval_t)(GUARD_INTERVAL_AUTO + 2))
#define GUARD_INTERVAL_19_256    ((fe_guard_interval_t)(GUARD_INTERVAL_AUTO + 3))
#define SYS_DVBT2                ((fe_delivery_system_t)(SYS_DAB + 1))
#endif

#if DVBAPIVERSION < 0x0504
#define SYS_TURBO                ((fe_delivery_system_t)(SYS_DVBT2 + 1))
#endif

#if DVBAPIVERSION < 0x0505
#define DTV_ENUM_DELSYS          44
#endif

#if DVBAPIVERSION < 0x0506
#define SYS_DVBC_ANNEX_C         ((fe_delivery_system_t)(SYS_TURBO + 1))
#define SYS_DVBC_ANNEX_A         SYS_DVBC_ANNEX_AC
#endif

#if DVBAPIVERSION < 0x0507
#define FEC_2_5                  ((fe_code_rate_t)(FEC_9_10 + 1))
#define QAM_4_NR                 ((fe_modulation_t)(DQPSK + 1))
#define TRANSMISSION_MODE_C1     ((fe_transmit_mode_t)(TRANSMISSION_MODE_32K + 1))
#define TRANSMISSION_MODE_C3780  ((fe_transmit_mode_t)(TRANSMISSION_MODE_32K + 2))
#define GUARD_INTERVAL_PN420     ((fe_guard_interval_t)(GUARD_INTERVAL_19_256 + 1))
#define GUARD_INTERVAL_PN595     ((fe_guard_interval_t)(GUARD_INTERVAL_19_256 + 2))
#define GUARD_INTERVAL_PN945     ((fe_guard_interval_t)(GUARD_INTERVAL_19_256 + 3))
#define SYS_DTMB                 SYS_DMBTH
#endif

#if DVBAPIVERSION < 0x0508
#define FE_CAN_MULTISTREAM       ((fe_caps_t)0x4000000)
#define DTV_STREAM_ID            42
#define DTV_DVBT2_PLP_ID_LEGACY  43
#endif
