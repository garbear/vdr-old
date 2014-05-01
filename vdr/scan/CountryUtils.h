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

#include <linux/dvb/frontend.h>
#include <string>

namespace VDR
{

enum eFrontendType
{
  eFT_DVB,
  eFT_ATSC
};

enum eChannelList
{
  ATSC_VSB  = 1, // ATSC terrestrial, US NTSC center freqs
  ATSC_QAM  = 2, // ATSC cable, US EIA/NCTA Std Cable center freqs + IRC list
  DVBT_AU   = 3, // AUSTRALIA, 7MHz step list
  DVBT_DE   = 4, // GERMANY
  DVBT_FR   = 5, // FRANCE, +/- offset
  DVBT_GB   = 6, // UNITED KINGDOM, +/- offset
  DVBC_QAM  = 7, // EUROPE
  DVBC_FI   = 8, // FINLAND, QAM128
  DVBC_FR   = 9, // FRANCE, needs user response
};

enum eOffsetType
{
  NO_OFFSET,
  POS_OFFSET,
  NEG_OFFSET
};

namespace COUNTRY
{
  /*
   * Every country has its own number here
   * If adding new one simply append to enum
   */
  enum eCountry
  {
    AF, AX, AL, DZ, AS, AD, AO, AI, AQ, AG, AR, AM, AW, AU, AT, AZ, BS, BH, BD,
    BB, BY, BE, BZ, BJ, BM, BT, BO, BA, BW, BV, BR, IO, BN, BG, BF, BI, KH, CM,
    CA, CV, KY, CF, TD, CL, CN, CX, CC, CO, KM, CG, CD, CK, CR, CI, HR, CU, CY,
    CZ, DK, DJ, DM, DO, EC, EG, SV, GQ, ER, EE, ET, FK, FO, FJ, FI, FR, GF, PF,
    TF, GA, GM, GE, DE, GH, GI, GR, GL, GD, GP, GU, GT, GG, GN, GW, GY, HT, HM,
    VA, HN, HK, HU, IS, IN, ID, IR, IQ, IE, IM, IL, IT, JM, JP, JE, JO, KZ, KE,
    KI, KP, KR, KW, KG, LA, LV, LB, LS, LR, LY, LI, LT, LU, MO, MK, MG, MW, MY,
    MV, ML, MT, MH, MQ, MR, MU, YT, MX, FM, MD, MC, MN, ME, MS, MA, MZ, MM, NA,
    NR, NP, NL, AN, NC, NZ, NI, NE, NG, NU, NF, MP, NO, OM, PK, PW, PS, PA, PG,
    PY, PE, PH, PN, PL, PT, PR, QA, RE, RO, RU, RW, BL, SH, KN, LC, MF, PM, VC,
    WS, SM, ST, SA, SN, RS, SC, SL, SG, SK, SI, SB, SO, ZA, GS, ES, LK, SD, SR,
    SJ, SZ, SE, CH, SY, TW, TJ, TZ, TH, TL, TG, TK, TO, TT, TN, TR, TM, TC, TV,
    UG, UA, AE, GB, US, UM, UY, UZ, VU, VE, VN, VG, VI, WF, EH, YE, ZM,
  };
}

struct cCountry
{
  const char*       short_name;
  COUNTRY::eCountry id;
  const char*       full_name;
};

class CountryUtils
{
public:

  /*!
   * \brief Get the frontend type (DVB or ATSC)
   * \param country The country ID (see CountryUtils.h)
   * \param frontendType Either DVB or ATSC
   * \return False if not known for that country
   */
  static bool GetFrontendType(COUNTRY::eCountry countryId, eFrontendType& frontendType);

  /*!
   * \brief Get a country-specific channel list
   * \param country The country ID
   * \param dvbType If DVB, the frontend type is used to choose between cable and terrestrial
   * \param atscModulation If ATSC, the modulation is used to choose between cable and terrestrial
   * \param channelList The channel list
   */
  static bool GetChannelList(COUNTRY::eCountry country, fe_type dvbType, fe_modulation atscModulation, eChannelList& channelList);


  /*!
   * \brief Get the base offset for specified channel and channel list
   */
  static bool GetBaseOffset(unsigned int channel, eChannelList channelList, int& offset);


  static bool GetFrequencyStep(unsigned int channel, eChannelList channelList, unsigned int& frequencyStep);

  /*!
   * \brief Some countries use constant offsets around center frequency
   */
  static bool GetFrequencyOffset(unsigned int channel, eChannelList channelList, eOffsetType offsetType, int& offset);

  static bool GetBandwidth(unsigned int channel, eChannelList channelList, fe_bandwidth& bandwidth);

  /*!
   * \brief Convert between ISO 3166-1 two-letter constant, index number and full name
   */
  static bool GetIdFromShortName(std::string shortName, COUNTRY::eCountry& countryId);
  static bool GetShortNameFromId(COUNTRY::eCountry countryId, std::string& shortName);
  static bool GetFullNameFromId(COUNTRY::eCountry countryId, std::string& fullName);
};

}
