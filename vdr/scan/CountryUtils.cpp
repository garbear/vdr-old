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

#include "CountryUtils.h"
#include "utils/StringUtils.h"

using namespace COUNTRY;
using namespace std;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))
#endif

// Two letters constants from ISO 3166-1 sorted alphabetically by long country name
cCountry countryList[] =
{
/*- ISO 3166-1 - unique id - long country name */
       {"AF",     AF ,   "AFGHANISTAN"},
       {"AX",     AX ,   "ÅLAND ISLANDS"},
       {"AL",     AL ,   "ALBANIA"},
       {"DZ",     DZ ,   "ALGERIA"},
       {"AS",     AS ,   "AMERICAN SAMOA"},
       {"AD",     AD ,   "ANDORRA"},
       {"AO",     AO ,   "ANGOLA"},
       {"AI",     AI ,   "ANGUILLA"},
       {"AQ",     AQ ,   "ANTARCTICA"},
       {"AG",     AG ,   "ANTIGUA AND BARBUDA"},
       {"AR",     AR ,   "ARGENTINA"},
       {"AM",     AM ,   "ARMENIA"},
       {"AW",     AW ,   "ARUBA"},
       {"AU",     AU ,   "AUSTRALIA"},
       {"AT",     AT ,   "AUSTRIA"},
       {"AZ",     AZ ,   "AZERBAIJAN"},
       {"BS",     BS ,   "BAHAMAS"},
       {"BH",     BH ,   "BAHRAIN"},
       {"BD",     BD ,   "BANGLADESH"},
       {"BB",     BB ,   "BARBADOS"},
       {"BY",     BY ,   "BELARUS"},
       {"BE",     BE ,   "BELGIUM"},
       {"BZ",     BZ ,   "BELIZE"},
       {"BJ",     BJ ,   "BENIN"},
       {"BM",     BM ,   "BERMUDA"},
       {"BT",     BT ,   "BHUTAN"},
       {"BO",     BO ,   "BOLIVIA"},
       {"BA",     BA ,   "BOSNIA AND HERZEGOVINA"},
       {"BW",     BW ,   "BOTSWANA"},
       {"BV",     BV ,   "BOUVET ISLAND"},
       {"BR",     BR ,   "BRAZIL"},
       {"IO",     IO ,   "BRITISH INDIAN OCEAN TERRITORY"},
       {"BN",     BN ,   "BRUNEI DARUSSALAM"},
       {"BG",     BG ,   "BULGARIA"},
       {"BF",     BF ,   "BURKINA FASO"},
       {"BI",     BI ,   "BURUNDI"},
       {"KH",     KH ,   "CAMBODIA"},
       {"CM",     CM ,   "CAMEROON"},
       {"CA",     CA ,   "CANADA"},
       {"CV",     CV ,   "CAPE VERDE"},
       {"KY",     KY ,   "CAYMAN ISLANDS"},
       {"CF",     CF ,   "CENTRAL AFRICAN REPUBLIC"},
       {"TD",     TD ,   "CHAD"},
       {"CL",     CL ,   "CHILE"},
       {"CN",     CN ,   "CHINA"},
       {"CX",     CX ,   "CHRISTMAS ISLAND"},
       {"CC",     CC ,   "COCOS (KEELING) ISLANDS"},
       {"CO",     CO ,   "COLOMBIA"},
       {"KM",     KM ,   "COMOROS"},
       {"CG",     CG ,   "CONGO"},
       {"CD",     CD ,   "CONGO, THE DEMOCRATIC REPUBLIC OF THE"},
       {"CK",     CK ,   "COOK ISLANDS"},
       {"CR",     CR ,   "COSTA RICA"},
       {"CI",     CI ,   "CÔTE D'IVOIRE"},
       {"HR",     HR ,   "CROATIA"},
       {"CU",     CU ,   "CUBA"},
       {"CY",     CY ,   "CYPRUS"},
       {"CZ",     CZ ,   "CZECH REPUBLIC"},
       {"DK",     DK ,   "DENMARK"},
       {"DJ",     DJ ,   "DJIBOUTI"},
       {"DM",     DM ,   "DOMINICA"},
       {"DO",     DO ,   "DOMINICAN REPUBLIC"},
       {"EC",     EC ,   "ECUADOR"},
       {"EG",     EG ,   "EGYPT"},
       {"SV",     SV ,   "EL SALVADOR"},
       {"GQ",     GQ ,   "EQUATORIAL GUINEA"},
       {"ER",     ER ,   "ERITREA"},
       {"EE",     EE ,   "ESTONIA"},
       {"ET",     ET ,   "ETHIOPIA"},
       {"FK",     FK ,   "FALKLAND ISLANDS (MALVINAS)"},
       {"FO",     FO ,   "FAROE ISLANDS"},
       {"FJ",     FJ ,   "FIJI"},
       {"FI",     FI ,   "FINLAND"},
       {"FR",     FR ,   "FRANCE"},
       {"GF",     GF ,   "FRENCH GUIANA"},
       {"PF",     PF ,   "FRENCH POLYNESIA"},
       {"TF",     TF ,   "FRENCH SOUTHERN TERRITORIES"},
       {"GA",     GA ,   "GABON"},
       {"GM",     GM ,   "GAMBIA"},
       {"GE",     GE ,   "GEORGIA"},
       {"DE",     DE ,   "GERMANY"},
       {"GH",     GH ,   "GHANA"},
       {"GI",     GI ,   "GIBRALTAR"},
       {"GR",     GR ,   "GREECE"},
       {"GL",     GL ,   "GREENLAND"},
       {"GD",     GD ,   "GRENADA"},
       {"GP",     GP ,   "GUADELOUPE"},
       {"GU",     GU ,   "GUAM"},
       {"GT",     GT ,   "GUATEMALA"},
       {"GG",     GG ,   "GUERNSEY"},
       {"GN",     GN ,   "GUINEA"},
       {"GW",     GW ,   "GUINEA-BISSAU"},
       {"GY",     GY ,   "GUYANA"},
       {"HT",     HT ,   "HAITI"},
       {"HM",     HM ,   "HEARD ISLAND AND MCDONALD ISLANDS"},
       {"VA",     VA ,   "HOLY SEE (VATICAN CITY STATE)"},
       {"HN",     HN ,   "HONDURAS"},
       {"HK",     HK ,   "HONG KONG"},
       {"HU",     HU ,   "HUNGARY"},
       {"IS",     IS ,   "ICELAND"},
       {"IN",     IN ,   "INDIA"},
       {"ID",     ID ,   "INDONESIA"},
       {"IR",     IR ,   "IRAN, ISLAMIC REPUBLIC OF"},
       {"IQ",     IQ ,   "IRAQ"},
       {"IE",     IE ,   "IRELAND"},
       {"IM",     IM ,   "ISLE OF MAN"},
       {"IL",     IL ,   "ISRAEL"},
       {"IT",     IT ,   "ITALY"},
       {"JM",     JM ,   "JAMAICA"},
       {"JP",     JP ,   "JAPAN"},
       {"JE",     JE ,   "JERSEY"},
       {"JO",     JO ,   "JORDAN"},
       {"KZ",     KZ ,   "KAZAKHSTAN"},
       {"KE",     KE ,   "KENYA"},
       {"KI",     KI ,   "KIRIBATI"},
       {"KP",     KP ,   "KOREA, DEMOCRATIC PEOPLE'S REPUBLIC OF"},
       {"KR",     KR ,   "KOREA, REPUBLIC OF"},
       {"KW",     KW ,   "KUWAIT"},
       {"KG",     KG ,   "KYRGYZSTAN"},
       {"LA",     LA ,   "LAO PEOPLE'S DEMOCRATIC REPUBLIC"},
       {"LV",     LV ,   "LATVIA"},
       {"LB",     LB ,   "LEBANON"},
       {"LS",     LS ,   "LESOTHO"},
       {"LR",     LR ,   "LIBERIA"},
       {"LY",     LY ,   "LIBYAN ARAB JAMAHIRIYA"},
       {"LI",     LI ,   "LIECHTENSTEIN"},
       {"LT",     LT ,   "LITHUANIA"},
       {"LU",     LU ,   "LUXEMBOURG"},
       {"MO",     MO ,   "MACAO"},
       {"MK",     MK ,   "MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF"},
       {"MG",     MG ,   "MADAGASCAR"},
       {"MW",     MW ,   "MALAWI"},
       {"MY",     MY ,   "MALAYSIA"},
       {"MV",     MV ,   "MALDIVES"},
       {"ML",     ML ,   "MALI"},
       {"MT",     MT ,   "MALTA"},
       {"MH",     MH ,   "MARSHALL ISLANDS"},
       {"MQ",     MQ ,   "MARTINIQUE"},
       {"MR",     MR ,   "MAURITANIA"},
       {"MU",     MU ,   "MAURITIUS"},
       {"YT",     YT ,   "MAYOTTE"},
       {"MX",     MX ,   "MEXICO"},
       {"FM",     FM ,   "MICRONESIA, FEDERATED STATES OF"},
       {"MD",     MD ,   "MOLDOVA"},
       {"MC",     MC ,   "MONACO"},
       {"MN",     MN ,   "MONGOLIA"},
       {"ME",     ME ,   "MONTENEGRO"},
       {"MS",     MS ,   "MONTSERRAT"},
       {"MA",     MA ,   "MOROCCO"},
       {"MZ",     MZ ,   "MOZAMBIQUE"},
       {"MM",     MM ,   "MYANMAR"},
       {"NA",     NA ,   "NAMIBIA"},
       {"NR",     NR ,   "NAURU"},
       {"NP",     NP ,   "NEPAL"},
       {"NL",     NL ,   "NETHERLANDS"},
       {"AN",     AN ,   "NETHERLANDS ANTILLES"},
       {"NC",     NC ,   "NEW CALEDONIA"},
       {"NZ",     NZ ,   "NEW ZEALAND"},
       {"NI",     NI ,   "NICARAGUA"},
       {"NE",     NE ,   "NIGER"},
       {"NG",     NG ,   "NIGERIA"},
       {"NU",     NU ,   "NIUE"},
       {"NF",     NF ,   "NORFOLK ISLAND"},
       {"MP",     MP ,   "NORTHERN MARIANA ISLANDS"},
       {"NO",     NO ,   "NORWAY"},
       {"OM",     OM ,   "OMAN"},
       {"PK",     PK ,   "PAKISTAN"},
       {"PW",     PW ,   "PALAU"},
       {"PS",     PS ,   "PALESTINIAN TERRITORY, OCCUPIED"},
       {"PA",     PA ,   "PANAMA"},
       {"PG",     PG ,   "PAPUA NEW GUINEA"},
       {"PY",     PY ,   "PARAGUAY"},
       {"PE",     PE ,   "PERU"},
       {"PH",     PH ,   "PHILIPPINES"},
       {"PN",     PN ,   "PITCAIRN"},
       {"PL",     PL ,   "POLAND"},
       {"PT",     PT ,   "PORTUGAL"},
       {"PR",     PR ,   "PUERTO RICO"},
       {"QA",     QA ,   "QATA"},
       {"RE",     RE ,   "RÉUNION"},
       {"RO",     RO ,   "ROMANIA"},
       {"RU",     RU ,   "RUSSIAN FEDERATION"},
       {"RW",     RW ,   "RWANDA"},
       {"BL",     BL ,   "SAINT BARTHÉLEMY"},
       {"SH",     SH ,   "SAINT HELENA"},
       {"KN",     KN ,   "SAINT KITTS AND NEVIS"},
       {"LC",     LC ,   "SAINT LUCIA"},
       {"MF",     MF ,   "SAINT MARTIN"},
       {"PM",     PM ,   "SAINT PIERRE AND MIQUELON"},
       {"VC",     VC ,   "SAINT VINCENT AND THE GRENADINES"},
       {"WS",     WS ,   "SAMOA"},
       {"SM",     SM ,   "SAN MARINO"},
       {"ST",     ST ,   "SAO TOME AND PRINCIPE"},
       {"SA",     SA ,   "SAUDI ARABIA"},
       {"SN",     SN ,   "SENEGAL"},
       {"RS",     RS ,   "SERBIA"},
       {"SC",     SC ,   "SEYCHELLES"},
       {"SL",     SL ,   "SIERRA LEONE"},
       {"SG",     SG ,   "SINGAPORE"},
       {"SK",     SK ,   "SLOVAKIA"},
       {"SI",     SI ,   "SLOVENIA"},
       {"SB",     SB ,   "SOLOMON ISLANDS"},
       {"SO",     SO ,   "SOMALIA"},
       {"ZA",     ZA ,   "SOUTH AFRICA"},
       {"GS",     GS ,   "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS"},
       {"ES",     ES ,   "SPAIN"},
       {"LK",     LK ,   "SRI LANKA"},
       {"SD",     SD ,   "SUDAN"},
       {"SR",     SR ,   "SURINAME"},
       {"SJ",     SJ ,   "SVALBARD AND JAN MAYEN"},
       {"SZ",     SZ ,   "SWAZILAND"},
       {"SE",     SE ,   "SWEDEN"},
       {"CH",     CH ,   "SWITZERLAND"},
       {"SY",     SY ,   "SYRIAN ARAB REPUBLIC"},
       {"TW",     TW ,   "TAIWAN"},
       {"TJ",     TJ ,   "TAJIKISTAN"},
       {"TZ",     TZ ,   "TANZANIA, UNITED REPUBLIC OF"},
       {"TH",     TH ,   "THAILAND"},
       {"TL",     TL ,   "TIMOR-LESTE"},
       {"TG",     TG ,   "TOGO"},
       {"TK",     TK ,   "TOKELAU"},
       {"TO",     TO ,   "TONGA"},
       {"TT",     TT ,   "TRINIDAD AND TOBAGO"},
       {"TN",     TN ,   "TUNISIA"},
       {"TR",     TR ,   "TURKEY"},
       {"TM",     TM ,   "TURKMENISTAN"},
       {"TC",     TC ,   "TURKS AND CAICOS ISLANDS"},
       {"TV",     TV ,   "TUVALU"},
       {"UG",     UG ,   "UGANDA"},
       {"UA",     UA ,   "UKRAINE"},
       {"AE",     AE ,   "UNITED ARAB EMIRATES"},
       {"GB",     GB ,   "UNITED KINGDOM"},
       {"US",     US ,   "UNITED STATES"},
       {"UM",     UM ,   "UNITED STATES MINOR OUTLYING ISLANDS"},
       {"UY",     UY ,   "URUGUAY"},
       {"UZ",     UZ ,   "UZBEKISTAN"},
       {"VU",     VU ,   "VANUATU"},
       {"VE",     VE ,   "VENEZUELA"},
       {"VN",     VN ,   "VIET NAM"},
       {"VG",     VG ,   "VIRGIN ISLANDS, BRITISH"},
       {"VI",     VI ,   "VIRGIN ISLANDS, U.S."},
       {"WF",     WF ,   "WALLIS AND FUTUNA"},
       {"EH",     EH ,   "WESTERN SAHARA"},
       {"YE",     YE ,   "YEMEN"},
       {"ZM",     ZM ,   "ZAMBIA"},
};

/**************************************************************************************************
 * FREQUENCY CALCULATION SCHEME:
 * each frequency used by w_scan is calculated as follows:
 *
 * frequency(channellist, channel, frequency_offset_index) =
 *      base_offset(channel, channellist) +
 *      channel * freq_step(channel, channellist) +
 *      freq_offset(channel, channellist, frequency_offset_index);
 *
 *
 * - channellist is choosen by user, if not choosen defaults are used.
 * - channel = 0..133 (might be extended if needed)
 * - frequency_offset_index = 0 (no offset) -> 1 (pos offset) -> 2 (neg offset)
 *
 * if base_offset(channel, channellist) returns -1 this channel will be skipped.
 * if freq_offset(channel, channellist, frequency_offset_index) returns -1 this offset will be skipped.
 *
 * example:
 * channellist = 4; channel = 12
 *
 * base_offset(12, 4) = 142500000
 * freq_step(12, 4) = 7000000
 * freq_offset(12, 4, 0) = 0
 * freq_offset(12, 4, 1) = -1
 * freq_offset(12, 4, 2) = -1
 *
 * frequency = 142500000 + 12 * 7000000 = 226500000
 * since freq_offset returns -1 for frequency_offset_index = (1,2) no frequency offset is applied.
 *
 * 20090101 -wk
 **************************************************************************************************/

bool CountryUtils::GetFrontendType(eCountry countryId, eFrontendType& frontendType)
{
  switch (countryId)
  {
    case    AT:     //      AUSTRIA
    case    BE:     //      BELGIUM
    case    CH:     //      SWITZERLAND
    case    CZ:     //      CZECH REPUBLIC
    case    DE:     //      GERMANY
    case    DK:     //      DENMARK
    case    ES:     //      SPAIN
    case    FR:     //      FRANCE
    case    FI:     //      FINLAND
    case    GB:     //      UNITED KINGDOM
    case    GR:     //      GREECE
    case    HR:     //      CROATIA
    case    HK:     //      HONG KONG
    case    IS:     //      ICELAND
    case    IT:     //      ITALY
    case    LU:     //      LUXEMBOURG
    case    LV:     //      LATVIA
    case    NL:     //      NETHERLANDS
    case    NO:     //      NORWAY
    case    NZ:     //      NEW ZEALAND
    case    PL:     //      POLAND
    case    SE:     //      SWEDEN
    case    SK:     //      SLOVAKIA
    case    TW:     //      TAIWAN, DVB-T w. ATSC freq list (thanks for freqlist to mkrufky)
    case    AU:     //      AUSTRALIA, DVB-T w. 7MHz step
      frontendType = eFT_DVB;
      break;

    case    US:     //      UNITED STATES
    case    CA:     //      CANADA
      frontendType = eFT_ATSC;
      break;

    default:
      return false;
  }

  return true;
}

bool CountryUtils::GetChannelList(eCountry country, fe_type dvbType, fe_modulation atscModulation, eChannelList& channelList)
{
  switch (country)
  {
    //**********DVB freq lists*******************************************//
    case    AT:     //      AUSTRIA
    case    BE:     //      BELGIUM
    case    CH:     //      SWITZERLAND
    case    DE:     //      GERMANY
    case    DK:     //      DENMARK
    case    ES:     //      SPAIN
    case    GR:     //      GREECE
    case    HR:     //      CROATIA
    case    HK:     //      HONG KONG
    case    IS:     //      ICELAND
    case    IT:     //      ITALY
    case    LU:     //      LUXEMBOURG
    case    LV:     //      LATVIA
    case    NL:     //      NETHERLANDS
    case    NO:     //      NORWAY
    case    NZ:     //      NEW ZEALAND
    case    PL:     //      POLAND
    case    SE:     //      SWEDEN
    case    SK:     //      SLOVAKIA
      switch (dvbType)
      {
        case FE_QAM:
          channelList = DVBC_QAM;
          break;
        default:
          channelList = DVBT_DE;
          break;
      }
      break;
    case    CZ:     //      CZECH REPUBLIC
    case    FI:     //      FINLAND
      switch (dvbType)
      {
        case FE_QAM:
          channelList = DVBC_FI;
          break;
        default:
          channelList = DVBT_DE;
          break;
      }
      break;
    case    FR:     //      FRANCE
      switch (dvbType)
      {
        case FE_QAM:
          channelList = DVBC_FR;
          break;
        default:
          channelList = DVBT_FR;
          break;
      }
      break;
    case    GB:     //      UNITED KINGDOM
      switch (dvbType)
      {
      case FE_QAM:
        channelList = DVBC_QAM;
        break;
      default:
        channelList = DVBT_GB;
        break;
      }
      break;
    case    AU:     //      AUSTRALIA
      switch (dvbType)
      {
      case FE_QAM:
        break;
      default:
        channelList = DVBT_AU;
        break;
      }
      break;

    //**********ATSC freq lists******************************************//
    case    US:     //      UNITED STATES
    case    CA:     //      CANADA
    case    TW:     //      TAIWAN, DVB-T w. ATSC freq list
      switch (atscModulation)
      {
        case VSB_8:
          channelList = ATSC_VSB;
          break;
        case QAM_256:
          channelList = ATSC_QAM;
          break;
        default:
          return false;
      }
      break;

    //*******************************************************************//
    default:
      return false;
  }

  return true;
}

bool CountryUtils::GetBaseOffset(unsigned int channel, eChannelList channelList, int& offset)
{
  switch (channelList)
  {
    case ATSC_QAM: // ATSC cable, US EIA/NCTA Std Cable center freqs + IRC list
      switch (channel)
      {
        case   2 ...   4: offset =    45000000; return true;
        case   5 ...   6: offset =    49000000; return true;
        case   7 ...  13: offset =   135000000; return true;
        case  14 ...  22: offset =    39000000; return true;
        case  23 ...  94: offset =    81000000; return true;
        case  95 ...  99: offset =  -477000000; return true;
        case 100 ... 133: offset =    51000000; return true;
        default:                                return false;
      }
      break;
    case ATSC_VSB: // ATSC terrestrial, US NTSC center freqs
      switch (channel)
      {
        case  2 ...  4:   offset =   45000000; return true;
        case  5 ...  6:   offset =   49000000; return true;
        case  7 ... 13:   offset =  135000000; return true;
        case 14 ... 69:   offset =  389000000; return true;
        default:                               return false;
      }
      break;
    case DVBT_AU: // AUSTRALIA, 7MHz step list
      switch (channel)
      {
        case  5 ... 12:   offset =  142500000; return true;
        case 21 ... 69:   offset =  333500000; return true;
        default:                               return false;
      }
      break;
    case DVBT_DE: // GERMANY
    case DVBT_FR: // FRANCE, +/- offset
    case DVBT_GB: // UNITED KINGDOM, +/- offset
      switch (channel)
      {
        case  5 ... 12:   offset =  142500000; return true;
        case 21 ... 69:   offset =  306000000; return true;
        default:                               return false;
      }
      break;
    case DVBC_QAM: // EUROPE
      switch (channel)
      {
        case  0 ... 1:    offset =   73000000; return true;
        case  5 ... 12:   offset =   73000000; return true;
        case 22 ... 90:   offset =  138000000; return true;
        default:                               return false;
      }
      break;
    case DVBC_FI: // FINLAND, QAM128
      switch (channel)
      {
        case  1 ... 90:   offset =  138000000; return true;
        default:                               return false;
      }
      break;
    case DVBC_FR: // FRANCE, needs user response.
      switch (channel)
      {
        case  1 ... 39:   offset =  107000000; return true;
        case 40 ... 89:   offset =  138000000; return true;
        default:                               return false;
      }
      break;
    default:
      return false;
  }
}

bool CountryUtils::GetFrequencyStep(unsigned int channel, eChannelList channelList, unsigned int& frequencyStep)
{
  switch (channelList)
  {
    case ATSC_QAM:
    case ATSC_VSB:         frequencyStep = 6000000; return true; // ATSC, 6MHz step
    case DVBT_AU:          frequencyStep = 7000000; return true; // DVB-T Australia, 7MHz step
    case DVBT_DE:
    case DVBT_FR:
    case DVBT_GB:
      switch (channel) // DVB-T Europe, 7MHz VHF ch5..12, all other 8MHz
      {
        case  5 ... 12:    frequencyStep = 7000000; return true;
        case 21 ... 69:    frequencyStep = 8000000; return true;
        default:           frequencyStep = 8000000; return true; // Should be never reached.
      }
      break;
    case DVBC_QAM:
    case DVBC_FI:
    case DVBC_FR:  frequencyStep = 8000000; return true; // DVB-C, 8MHz step
    default:
       return false;
  }
}

bool CountryUtils::GetFrequencyOffset(unsigned int channel, eChannelList channelList, eOffsetType offsetType, int& offset)
{
  switch (channelList)
  {
    case ATSC_QAM:
      switch (channel)
      {
        case 14 ... 16:
        case 25 ... 53:
        case 98 ... 99:
          switch (offsetType)
          {
            case NO_OFFSET:   offset =       0; return true; // Incrementally Related Carriers (IRC)
            case POS_OFFSET:  offset =   12500; return true; // US EIA/NCTA Standard Cable center frequencies
            default:                            return false;
          }
        break;
        default: // IRC = standard cable center
          switch (offsetType)
          {
            case NO_OFFSET:   offset =       0; return true; // center freq
            default:                            return false;
          }
          break;
      }
      break;
    case DVBT_FR:
    case DVBT_GB:
      switch (channel)
      {
        case  5 ... 12: // VHF channels
        switch (offsetType)
        {
          case NO_OFFSET:     offset =       0; return true; // no offset
          default:                              return false;
        }
        break;
      default: // UHF channels
        switch (offsetType)
        {
          case NO_OFFSET:     offset =       0; return true; // center freq
          case POS_OFFSET:    offset = +167000; return true; // center+offset
          case NEG_OFFSET:    offset = -167000; return true; // center-offset
          default:                              return false;
        }
        break;
      }
      break;
    case DVBT_AU:
      switch (offsetType)
      {
        case NO_OFFSET:       offset =       0; return true; // center freq
        case POS_OFFSET:      offset = +125000; return true; // center+offset
        default:                                return false;
      }
      break;
    case DVBC_FR:
      switch (channel)
      {
        case 1 ... 39:
          switch (offsetType)
          {
            case NO_OFFSET:   offset =       0; return true; // center freq
            case POS_OFFSET:  offset = +125000; return true; // center+offset
            default:                            return false;
          }
          break;
        case 40 ... 89:
        default:
          switch (offsetType)
          {
            case NO_OFFSET:   offset =       0; return true;
            default:                            return false;
          }
          break;
      }
      break;

    default:
      switch (offsetType)
      {
        case NO_OFFSET:       offset =       0; return true;
        default:                                return false;
      }
      break;
  }
}

bool CountryUtils::GetBandwidth(unsigned int channel, eChannelList channelList, fe_bandwidth& bandwidth)
{
  unsigned int frequencyStep;
  if (!GetFrequencyStep(channel, channelList, frequencyStep))
    return false;

  switch (frequencyStep)
  {
    case 8000000:    bandwidth = BANDWIDTH_8_MHZ; return true;
    case 7000000:    bandwidth = BANDWIDTH_7_MHZ; return true;
    case 6000000:    bandwidth = BANDWIDTH_6_MHZ; return true;
    // Missing in Linux DVB API
    case 5000000:    bandwidth = BANDWIDTH_5_MHZ; return true;
    default:                                      return false;
  }
}

bool CountryUtils::GetIdFromShortName(string shortName, eCountry& countryId)
{
  if (shortName.length() != 2)
    return false;

  StringUtils::ToUpper(shortName);

  for (unsigned int i = 0; i < ARRAY_SIZE(countryList); i++)
  {
    if (shortName[0] == countryList[i].short_name[0] &&
        shortName[1] == countryList[i].short_name[1])
    {
      countryId = countryList[i].id;
      return true;
    }
  }
  return false;
}

bool CountryUtils::GetShortNameFromId(eCountry countryId, string& shortName)
{
  for (unsigned int i = 0; i < ARRAY_SIZE(countryList); i++)
  {
    if (countryId == countryList[i].id)
    {
      shortName = countryList[i].short_name;
      return true;
    }
  }
  return false;
}

bool CountryUtils::GetFullNameFromId(eCountry countryId, string& fullName)
{
  for (unsigned int i = 0; i < ARRAY_SIZE(countryList); i++)
  {
    if (countryId == countryList[i].id)
    {
      fullName = countryList[i].full_name;
      return true;
    }
  }
  return false;
}