/*
 * Simple MPEG/DVB parser to achieve network/service information without initial tuning data
 *
 * Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * The author can be reached at: handygewinnspiel AT gmx DOT de
 *
 * The project's page is http://wirbel.htpc-forum.de/w_scan/index2.html
 * country.c/h, added 20090101
 * last update 20091230
 */

#include "countries.h"
#include "scan.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/dvb/frontend.h>

#ifdef VDRVERSNUM
namespace COUNTRY {
#endif

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



/*
 * User selects a country specific channellist.
 * therefore we know
 *      - frontend type DVB or ATSC
 *      - used frequency list (base_offsets, freq_step)
 *      - other country-specific things (transmission mode, frequency offsets from center..)
 * use two letter uppercase for 'country', as defined by ISO 3166-1
 */
int choose_country (const char * country,
                    int * atsc,
                    int * dvb,
                    uint16_t * frontend_type,
                    int * channellist) {
        if (*channellist == USERLIST) return 0;
        verbose("%s atsc%d dvb%d frontend%d\n",
                country, *atsc, *dvb, *frontend_type);
        if (strcasecmp(country_to_short_name(txt_to_country(country)), country)) {
                warning("\n\nCOUNTRY CODE IS NOT DEFINED. FALLING BACK TO \"DE\"\n\n");
                sleep(10);
                }
        info("using settings for %s\n", country_to_full_name(txt_to_country(country)));

        /*
         * choose DVB or ATSC frontend type
         */
        switch(txt_to_country(country)) {

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
                        switch(*dvb) {    
                                case FE_QAM:
                                        *frontend_type = FE_QAM;
                                        info("DVB cable\n");
                                        break;
                                default:
                                        *frontend_type = FE_OFDM;
                                        info("DVB aerial\n");
                                        break;               
                                }
                        break;
                
                case    US:     //      UNITED STATES
                case    CA:     //      CANADA
                        *frontend_type = FE_ATSC;
                        info("ATSC\n");
                        break;
                        
                default:
                        info("Country identifier %s not defined. Using defaults.\n", country);
                        return -1;
                        break;
                }


        /*
         * choose channellist name
         */
        switch(txt_to_country(country)) {
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
                        switch(*dvb) {    
                                case FE_QAM:
                                        *channellist = DVBC_QAM;
                                        info("DVB-C\n");
                                        break;
                                default:
                                        *channellist = DVBT_DE;
                                        info("DVB-T Europe\n");
                                        break;               
                                }
                        break;
                case    CZ:     //      CZECH REPUBLIC
                case    FI:     //      FINLAND
                        switch(*dvb) {    
                                case FE_QAM:
                                        *channellist = DVBC_FI;
                                        info("DVB-C FI\n");
                                        break;
                                default:
                                        *channellist = DVBT_DE;
                                        info("DVB-T Europe\n");
                                        break;               
                                }
                        break;
                case    FR:     //      FRANCE
                        switch(*dvb) {    
                                case FE_QAM:
                                        *channellist = DVBC_FR;
                                        info("DVB-C FR\n");
                                        break;
                                default:
                                        *channellist = DVBT_FR;
                                        info("DVB-T FR\n");
                                        break;               
                                }
                        break;
                case    GB:     //      UNITED KINGDOM
                        switch(*dvb) {    
                                case FE_QAM:
                                        *channellist = DVBC_QAM;
                                        info("DVB-C\n");
                                        break;
                                default:
                                        *channellist = DVBT_GB;
                                        info("DVB-T GB\n");
                                        break;               
                                }
                        break;
                case    AU:     //      AUSTRALIA
                        switch(*dvb) {    
                                case FE_QAM:
                                        info("cable australia not yet defined.\n");
                                        break;
                                default:
                                        *channellist = DVBT_AU;
                                        info("DVB-T AU\n");
                                        break;               
                                }
                        break;
                //**********ATSC freq lists******************************************//    
                case    US:     //      UNITED STATES
                case    CA:     //      CANADA
                case    TW:     //      TAIWAN, DVB-T w. ATSC freq list    
                        if (atsc_is_vsb(*atsc)) {
                                *channellist = ATSC_VSB;
                                info("VSB US/CA, DVB-T TW\n");
                                }
                        if (atsc_is_qam(*atsc)) {
                                *channellist = ATSC_QAM;
                                info("QAM US/CA\n");    
                                }
                        break;
                //******************************************************************//    
                default:
                        info("Country identifier %s not defined. Using default freq lists.\n", country);
                        return -1;
                        break;
                }
        return 0; // success        
}

/*
 * return the base offsets for specified channellist and channel.
 */
int base_offset(int channel, int channellist) {
switch (channellist) {
        case ATSC_QAM: //ATSC cable, US EIA/NCTA Std Cable center freqs + IRC list
                switch (channel) {
                        case  2 ...  4:   return   45000000;
                        case  5 ...  6:   return   49000000;
                        case  7 ... 13:   return  135000000;
                        case 14 ... 22:   return   39000000;
                        case 23 ... 94:   return   81000000;
                        case 95 ... 99:   return -477000000;
                        case 100 ... 133: return   51000000;
                        default:          return SKIP_CHANNEL; 
                        }
        case ATSC_VSB: //ATSC terrestrial, US NTSC center freqs
                switch (channel) {
                        case  2 ...  4: return   45000000;
                        case  5 ...  6: return   49000000;
                        case  7 ... 13: return  135000000;
                        case 14 ... 69: return  389000000;
                        default:        return  SKIP_CHANNEL;
                        }
        case DVBT_AU: //AUSTRALIA, 7MHz step list
                switch (channel) {
                        case  5 ... 12: return  142500000;
                        case 21 ... 69: return  333500000;
                        default:        return  SKIP_CHANNEL;
                        }
        case DVBT_DE: //GERMANY
        case DVBT_FR: //FRANCE, +/- offset
        case DVBT_GB: //UNITED KINGDOM, +/- offset
                switch (channel) {
                        case  5 ... 12: return  142500000;
                        case 21 ... 69: return  306000000;
                        default:        return  SKIP_CHANNEL;
                        }
        case DVBC_QAM: //EUROPE
                switch (channel) {
                        case  0 ... 1:  
                        case  5 ... 12: return   73000000;
                        case 22 ... 90: return  138000000;
                        default:        return  SKIP_CHANNEL;
                        }
        case DVBC_FI: //FINLAND, QAM128
                switch (channel) {
                        case  1 ... 90: return  138000000;
                        default:        return  SKIP_CHANNEL;
                        }
        case DVBC_FR: //FRANCE, needs user response.
                switch (channel) {
                        case  1 ... 39: return  107000000;
                        case 40 ... 89: return  138000000;
                        default:        return  SKIP_CHANNEL;
                        }
        default:
                fatal("%s: undefined channellist %d\n", __FUNCTION__, channellist);
                return SKIP_CHANNEL;
        }
}

/*
 * return the freq step size for specified channellist
 */
int freq_step(int channel, int channellist) {
switch (channellist) {
        case ATSC_QAM:
        case ATSC_VSB: return  6000000; // atsc, 6MHz step
        case DVBT_AU:  return  7000000; // dvb-t australia, 7MHz step
        case DVBT_DE:
        case DVBT_FR:
        case DVBT_GB:  switch (channel) { // dvb-t europe, 7MHz VHF ch5..12, all other 8MHz
                              case  5 ... 12:    return 7000000;
                              case 21 ... 69:    return 8000000;
                              default:           return 8000000; // should be never reached.
                              }
        case DVBC_QAM:
        case DVBC_FI:
        case DVBC_FR:  return  8000000; // dvb-c, 8MHz step
        default:
             fatal("%s: undefined channellist %d\n", __FUNCTION__, channellist);
             return SKIP_CHANNEL;
        }
}


int bandwidth(int channel, int channellist) {
switch(freq_step(channel, channellist)) {
       case 8000000:    return BANDWIDTH_8_MHZ;
       case 7000000:    return BANDWIDTH_7_MHZ;
       case 6000000:    return BANDWIDTH_6_MHZ;
       #ifdef BANDWIDTH_5_MHZ
       //missing in Linux DVB API
       case 5000000:    return BANDWIDTH_5_MHZ;
       #endif
       default:
           fatal("%s: undefined result\n", __FUNCTION__);
           return SKIP_CHANNEL;
       }
}

/*
 * some countries use constant offsets around center frequency.
 * define them here.
 */
int freq_offset(int channel, int channellist, int index) {
switch (channellist) {
        case USERLIST:
                return 0;
        case ATSC_QAM:
                switch (channel) {
                        case 14 ... 16:
                        case 25 ... 53:
                        case 98 ... 99:
                            switch (index) {
                                case NO_OFFSET:     return 0;       //Incrementally Related Carriers (IRC)
                                case POS_OFFSET:    return 12500;   //US EIA/NCTA Standard Cable center frequencies
                                default:            return STOP_OFFSET_LOOP;
                                }
                        default: // IRC = standard cable center
                            switch (index) {
                                case NO_OFFSET:     return 0;       //center freq
                                default:            return STOP_OFFSET_LOOP;
                                }
                        }
        case DVBT_FR:
        case DVBT_GB:
                switch (channel) {
                        case  5 ... 12: //VHF channels
                            switch (index) {
                                case NO_OFFSET:     return 0;       //no offset
                                default:            return STOP_OFFSET_LOOP;
                                }
                        default: //UHF channels
                            switch (index) {
                                case NO_OFFSET:     return 0;       //center freq
                                case POS_OFFSET:    return +167000; //center+offset
                                case NEG_OFFSET:    return -167000; //center-offset
                                default:            return STOP_OFFSET_LOOP;
                                }
                        }
        case DVBT_AU    :
                switch (index) {
                        case NO_OFFSET:             return 0;       //center freq
                        case POS_OFFSET:            return +125000; //center+offset
                        default:                    return STOP_OFFSET_LOOP;
                        }
        case DVBC_FR:
                switch (channel) {
                        case 1 ... 39:
                            switch (index) {
                                case NO_OFFSET:     return 0;       //center freq
                                case POS_OFFSET:    return +125000; //center+offset
                                default:            return STOP_OFFSET_LOOP;
                                }
                        case 40 ... 89:
                        default:
                            switch (index) {
                                case NO_OFFSET:     return 0;
                                default:            return STOP_OFFSET_LOOP;
                                }
                        }
                
        default:
                switch (index) {
                        case NO_OFFSET: return 0;
                        default:        return STOP_OFFSET_LOOP;
                        }
        }
}


/*
 * DVB-T: default value if transmission mode AUTO
 * not supported
 */
int dvbt_transmission_mode(int channel, int channellist) {
switch (channellist) {
        // GB seems to use 8k since 12/2009
        default:        return TRANSMISSION_MODE_8K;  
        }
}


/*
 * start/stop values for dvbc qam loop
 * 0 == QAM_64, 1 == QAM_256, 2 == QAM_128
 */
int dvbc_qam_max(int channel, int channellist) {
switch(channellist) {
       case DVBC_FI:    return 2; //QAM128
       case DVBC_FR:
       case DVBC_QAM:   return 1; //QAM256
       default:         return 0; //no qam loop
       }
}

    
int dvbc_qam_min(int channel, int channellist) {
switch(channellist) {
       case DVBC_FI:
       case DVBC_FR:
       case DVBC_QAM:   return 0; //QAM64
       default:         return 0; //no qam loop
       }
}

int atsc_is_vsb(int atsc) {
    return (atsc & ATSC_VSB);
}

int atsc_is_qam(int atsc) {
    return (atsc & ATSC_QAM);
}

/* 
 * two letters constants from ISO 3166-1.
 * sorted alphabetically by long country name
 */

struct cCountry country_list[] = {
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


/* convert ISO 3166-1 two-letter constant
 * to index number
 */
int txt_to_country(const char * id) {
unsigned int i;
for (i = 0; i < COUNTRY_COUNT(country_list); i++)
   if (! strcasecmp(id,country_list[i].short_name))
      return country_list[i].id;
return DE; // w_scan defaults to DVB-t de_DE
}


/* convert index number
 * to ISO 3166-1 two-letter constant
 */
const char * country_to_short_name(int idx) {
unsigned int i;
for (i = 0; i < COUNTRY_COUNT(country_list); i++)
   if (idx == country_list[i].id)
      return country_list[i].short_name;
return "DE"; // w_scan defaults to DVB-t de_DE
}

/* convert index number
 * to country name
 */
const char * country_to_full_name(int idx) {
unsigned int i;
for (i = 0; i < COUNTRY_COUNT(country_list); i++)
   if (idx == country_list[i].id)
      return country_list[i].full_name;
warning("COUNTRY CODE NOT DEFINED. PLEASE RE-CHECK WETHER YOU TYPED CORRECTLY.\n");
usleep(5000000);
return "GERMANY"; // w_scan defaults to DVB-t de_DE
}

void print_countries(void) {
unsigned int i;
for (i = 0; i < COUNTRY_COUNT(country_list); i++)
    info("\t%s\t\t%s\n", country_list[i].short_name, country_list[i].full_name);
}

int country_count() {
  return COUNTRY_COUNT(country_list);
}

#ifdef VDRVERSNUM
} // end namespace COUNTRY
#endif
