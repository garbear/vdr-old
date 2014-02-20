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

#include "CountryUtils.h"

#include <stdint.h>

// Country and Network Identification codes
class CniCodes
{
public:
  struct cCniCode
  {
    COUNTRY::eCountry id;
    const char*       network;
    uint16_t          ni_8_30_1;
    uint8_t           c_8_30_2;
    uint8_t           ni_8_30_2;
    uint8_t           a_X_26;
    uint8_t           b_X_26;
    uint16_t          vps__cni;
    uint16_t          cr_idx;
  };

  static const cCniCode& GetCniCode(unsigned int index);
  static unsigned int    GetCniCodeCount();
};
