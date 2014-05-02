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
#pragma once

#include "CountryUtils.h"
#include "SatelliteUtils.h"
#include "sources/linux/DVBTransponderParams.h"

#include <linux/dvb/frontend.h>

namespace VDR
{

// TODO: Needs a better home. Currently lives in DVBTransponderParams.h
/*
enum eDvbType
{
  DVB_TERR,
  DVB_CABLE,
  DVB_SAT,
  DVB_ATSC,
};
*/
#include "sources/linux/DVBTransponderParams.h"

enum eScanFlags
{
  SCAN_TV =        ( 1 << 0 ),
  SCAN_RADIO =     ( 1 << 1 ),
  SCAN_FTA =       ( 1 << 2 ),
  SCAN_SCRAMBLED = ( 1 << 3 ),
  SCAN_HD =        ( 1 << 4 ),
};

enum eDvbcSymbolRate
{
  eSR_AUTO,
  eSR_ALL,
  eSR_BEGIN,
  eSR_6900000 = eSR_BEGIN,
  eSR_6875000,
  eSR_6111000,
  eSR_6250000,
  eSR_6790000,
  eSR_6811000,
  eSR_5900000,
  eSR_5000000,
  eSR_3450000,
  eSR_4000000,
  eSR_6950000,
  eSR_7000000,
  eSR_6952000,
  eSR_5156000,
  eSR_5483000,
  eSR_END = eSR_5483000
};

class cScanConfig
{
public:
  cScanConfig();

  /*!
   * \brief Translate one of eDVBCSymbolRate's numeric enums into an integer value
   * \param sr Will assert if sr is eSR_AUTO or eSR_ALL
   * \return The integer value (e.g. 6900000 for eSR_6900000)
   */
  static unsigned int TranslateSymbolRate(eDvbcSymbolRate sr);
  static eDvbcSymbolRate TranslateSymbolRate(unsigned int sr);

  eDvbType              dvbType;
  fe_spectral_inversion dvbtInversion;
  fe_spectral_inversion dvbcInversion;
  eDvbcSymbolRate       dvbcSymbolRate;
  COUNTRY::eCountry     countryIndex;
  SATELLITE::eSatellite satelliteIndex;
  fe_modulation         atscModulation; // Either VSB over-the-air (VSB_8) or QAM Annex B cable TV (QAM_256)
};

}
