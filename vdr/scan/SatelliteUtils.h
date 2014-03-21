/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "Types.h"
#include "CountryUtils.h"
#include "dvb/extended_frontend.h"

#include <stdint.h>
#include <string>

namespace VDR
{

struct __sat_transponder
{
  fe_delivery_system_t modulation_system;
  uint32_t             intermediate_frequency;
  fe_polarization_t    polarization;
  uint32_t             symbol_rate;
  fe_code_rate_t       fec_inner;
  fe_rolloff_t         rolloff;
  fe_modulation_t      modulation_type;
};

/******************************************************************************
 * Every satellite has its own number here.
 * if adding new one simply append to enum.
 *****************************************************************************/
namespace SATELLITE
{
enum eSatellite
{
    S4E8 = 0,
    S7E0,
    S9E0,
    S10E0,
    S13E0,
    S16E0,
    S19E2,
    S21E6,
    S23E5,
    S25E5,
    S26EX,
    S28E2,
    S28E5,
    S31E5,
    S32E9,
    S33E0,
    S35E9,
    S36E0,
    S38E0,
    S39E0,
    S40EX,
    S42E0,
    S45E0,
    S49E0,
    S53E0,
    S57E0,
    S57EX,
    S60EX,
    S62EX,
    S64E2,
    S68EX,
    S70E5,
    S72EX,
    S75EX,
    S76EX,
    S78E5,
    S80EX,
    S83EX,
    S87E5,
    S88EX,
    S90EX,
    S91E5,
    S93E5,
    S95E0,
    S96EX,
    S100EX,
    S105EX,
    S108EX,
    S140EX,
    S160E0,
    S0W8,
    S4W0,
    S5WX,
    S7W0,
    S8W0,
    S11WX,
    S12W5,
    S14W0,
    S15W0,
    S18WX,
    S22WX,
    S24WX,
    S27WX,
    S30W0,
    S97W0,
};
}

struct cSat
{
  const char*                 short_name;
  const SATELLITE::eSatellite id;
  const char*                 full_name;
  const __sat_transponder*    items;
  const int                   item_count;
  const fe_west_east_flag_t   west_east_flag;
  const uint16_t              orbital_position;
  int                         rotor_position;   // Note: *not* const
  const char*                 source_id;        // VDR sources.conf
};

class SatelliteUtils
{
public:
  /*!
   * \brief Get information about a satellite
   */
  static const cSat& GetSatellite(SATELLITE::eSatellite satelliteId);

  /*!
   * \brief Get information about a satellite transponder
   * \param satelliteId The satellite ID. Will assert if not a valid enum value.
   * \param iChannelNumber The transponder index. Will assert if >= GetTransponderCount()
   */
  static const __sat_transponder& GetTransponder(SATELLITE::eSatellite satelliteId, unsigned int iChannelNumber);
  static unsigned int GetTransponderCount(SATELLITE::eSatellite satelliteId);
};

}
