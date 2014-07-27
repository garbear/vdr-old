/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#include "ScanConfig.h"
#include "channels/Channel.h"
#include "devices/Device.h"

#include <assert.h>

namespace VDR
{

cScanConfig::cScanConfig()
 : dvbType(TRANSPONDER_TERRESTRIAL),
   dvbcSymbolRate(eSR_AUTO),
   countryIndex(COUNTRY::DE),        // Default to Germany
   satelliteIndex(SATELLITE::S19E2), // Default to Astra 19.2
   atscModulation(ATSC_MODULATION_BOTH)
{
}

unsigned int cScanConfig::TranslateSymbolRate(eDvbcSymbolRate sr)
{
  switch (sr)
  {
    case eSR_6900000: return 6900000;
    case eSR_6875000: return 6875000;
    case eSR_6111000: return 6111000;
    case eSR_6250000: return 6250000;
    case eSR_6790000: return 6790000;
    case eSR_6811000: return 6811000;
    case eSR_5900000: return 5900000;
    case eSR_5000000: return 5000000;
    case eSR_3450000: return 3450000;
    case eSR_4000000: return 4000000;
    case eSR_6950000: return 6950000;
    case eSR_7000000: return 7000000;
    case eSR_6952000: return 6952000;
    case eSR_5156000: return 5156000;
    case eSR_5483000: return 5483000;
    default:          return 0;
  }
}

eDvbcSymbolRate cScanConfig::TranslateSymbolRate(unsigned int sr)
{
  switch (sr)
  {
    case 6900000: return eSR_6900000;
    case 6875000: return eSR_6875000;
    case 6111000: return eSR_6111000;
    case 6250000: return eSR_6250000;
    case 6790000: return eSR_6790000;
    case 6811000: return eSR_6811000;
    case 5900000: return eSR_5900000;
    case 5000000: return eSR_5000000;
    case 3450000: return eSR_3450000;
    case 4000000: return eSR_4000000;
    case 6950000: return eSR_6950000;
    case 7000000: return eSR_7000000;
    case 6952000: return eSR_6952000;
    case 5156000: return eSR_5156000;
    case 5483000: return eSR_5483000;
    default:      return eSR_AUTO;
  }
}

}
