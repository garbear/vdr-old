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

#define TRANSPONDER_XML_ELM_TRANSPONDER            "transponder"

#define TRANSPONDER_XML_ATTR_TYPE                  "type"
#define TRANSPONDER_XML_ATTR_BANDWIDTH             "bandwidth"
#define TRANSPONDER_XML_ATTR_CODERATEH             "coderateh"
#define TRANSPONDER_XML_ATTR_CODERATEL             "coderatel"
#define TRANSPONDER_XML_ATTR_DELIVERY_SYSTEM       "delivery"
#define TRANSPONDER_XML_ATTR_FREQUENCY             "frequency"
#define TRANSPONDER_XML_ATTR_GUARD                 "guard"
#define TRANSPONDER_XML_ATTR_HIERARCHY             "hierarchy"
#define TRANSPONDER_XML_ATTR_INVERSION             "inversion"
#define TRANSPONDER_XML_ATTR_MODULATION            "modulation"
#define TRANSPONDER_XML_ATTR_POLARIZATION          "polarization"
#define TRANSPONDER_XML_ATTR_ROLLOFF               "rolloff"
#define TRANSPONDER_XML_ATTR_STREAMID              "streamid"
#define TRANSPONDER_XML_ATTR_SYMBOL_RATE           "symbolrate"
#define TRANSPONDER_XML_ATTR_TRANSMISSION          "transmission"

#define TRANSPONDER_STRING_TYPE_ATSC               "ATSC"
#define TRANSPONDER_STRING_TYPE_CABLE              "cable"
#define TRANSPONDER_STRING_TYPE_SATELLITE          "satellite"
#define TRANSPONDER_STRING_TYPE_TERRESTRIAL        "terrestrial"

#define TRANSPONDER_STRING_BANDWIDTH_8_MHZ         "8 MHz"
#define TRANSPONDER_STRING_BANDWIDTH_7_MHZ         "7 MHz"
#define TRANSPONDER_STRING_BANDWIDTH_6_MHZ         "6 MHz"
#define TRANSPONDER_STRING_BANDWIDTH_AUTO          "auto"
#define TRANSPONDER_STRING_BANDWIDTH_5_MHZ         "5 MHz"
#define TRANSPONDER_STRING_BANDWIDTH_10_MHZ        "10 MHz"
#define TRANSPONDER_STRING_BANDWIDTH_1_712_MHZ     "1.712 MHz"

#define TRANSPONDER_STRING_FEC_NONE                "none"
#define TRANSPONDER_STRING_FEC_1_2                 "1/2"
#define TRANSPONDER_STRING_FEC_2_3                 "2/3"
#define TRANSPONDER_STRING_FEC_3_4                 "3/4"
#define TRANSPONDER_STRING_FEC_4_5                 "4/5"
#define TRANSPONDER_STRING_FEC_5_6                 "5/6"
#define TRANSPONDER_STRING_FEC_6_7                 "6/7"
#define TRANSPONDER_STRING_FEC_7_8                 "7/8"
#define TRANSPONDER_STRING_FEC_8_9                 "8/9"
#define TRANSPONDER_STRING_FEC_AUTO                "auto"
#define TRANSPONDER_STRING_FEC_3_5                 "3/5"
#define TRANSPONDER_STRING_FEC_9_10                "9/10"
#define TRANSPONDER_STRING_FEC_2_5                 "2/5"

#define TRANSPONDER_STRING_SYSTEM_UNDEFINED        "undefined"
#define TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_A     "DVBC_ANNEX_A"
#define TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_B     "DVBC_ANNEX_B"
#define TRANSPONDER_STRING_SYSTEM_DVBT             "DVB-T"
#define TRANSPONDER_STRING_SYSTEM_DSS              "DSS"
#define TRANSPONDER_STRING_SYSTEM_DVBS             "DVB-S"
#define TRANSPONDER_STRING_SYSTEM_DVBS2            "DVB-S2"
#define TRANSPONDER_STRING_SYSTEM_DVBH             "DVBH"
#define TRANSPONDER_STRING_SYSTEM_ISDBT            "ISDBT"
#define TRANSPONDER_STRING_SYSTEM_ISDBS            "ISDBS"
#define TRANSPONDER_STRING_SYSTEM_ISDBC            "ISDBC"
#define TRANSPONDER_STRING_SYSTEM_ATSC             "ATSC"
#define TRANSPONDER_STRING_SYSTEM_ATSCMH           "ATSC-MH"
#define TRANSPONDER_STRING_SYSTEM_DTMB             "DTMB"
#define TRANSPONDER_STRING_SYSTEM_CMMB             "CMMB"
#define TRANSPONDER_STRING_SYSTEM_DAB              "DAB"
#define TRANSPONDER_STRING_SYSTEM_DVBT2            "DVB-T2"
#define TRANSPONDER_STRING_SYSTEM_TURBO            "TURBO"
#define TRANSPONDER_STRING_SYSTEM_DVBC_ANNEX_C     "DVBC_ANNEX_C"

#define TRANSPONDER_STRING_GUARD_1_32              "1/32"
#define TRANSPONDER_STRING_GUARD_1_16              "1/16"
#define TRANSPONDER_STRING_GUARD_1_8               "1/8"
#define TRANSPONDER_STRING_GUARD_1_4               "1/4"
#define TRANSPONDER_STRING_GUARD_AUTO              "auto"
#define TRANSPONDER_STRING_GUARD_1_128             "1/128"
#define TRANSPONDER_STRING_GUARD_19_128            "19/128"
#define TRANSPONDER_STRING_GUARD_19_256            "19/256"
#define TRANSPONDER_STRING_GUARD_PN420             "PN420"
#define TRANSPONDER_STRING_GUARD_PN595             "PN595"
#define TRANSPONDER_STRING_GUARD_PN945             "PN945"

#define TRANSPONDER_STRING_HIERARCHY_NONE          "none"
#define TRANSPONDER_STRING_HIERARCHY_1             "1"
#define TRANSPONDER_STRING_HIERARCHY_2             "2"
#define TRANSPONDER_STRING_HIERARCHY_4             "4"
#define TRANSPONDER_STRING_HIERARCHY_AUTO          "auto"

#define TRANSPONDER_STRING_INVERSION_OFF           "off"
#define TRANSPONDER_STRING_INVERSION_ON            "on"
#define TRANSPONDER_STRING_INVERSION_AUTO          "auto"

#define TRANSPONDER_STRING_MODULATION_QPSK         "QPSK"
#define TRANSPONDER_STRING_MODULATION_QAM_16       "QAM16"
#define TRANSPONDER_STRING_MODULATION_QAM_32       "QAM32"
#define TRANSPONDER_STRING_MODULATION_QAM_64       "QAM64"
#define TRANSPONDER_STRING_MODULATION_QAM_128      "QAM128"
#define TRANSPONDER_STRING_MODULATION_QAM_256      "QAM256"
#define TRANSPONDER_STRING_MODULATION_QAM_AUTO     "auto"
#define TRANSPONDER_STRING_MODULATION_VSB_8        "VSB8"
#define TRANSPONDER_STRING_MODULATION_VSB_16       "VSB16"
#define TRANSPONDER_STRING_MODULATION_PSK_8        "PSK8"
#define TRANSPONDER_STRING_MODULATION_APSK_16      "APSK16"
#define TRANSPONDER_STRING_MODULATION_APSK_32      "APSK32"
#define TRANSPONDER_STRING_MODULATION_DQPSK        "DQPSK"
#define TRANSPONDER_STRING_MODULATION_QAM_4_NR     "QAM4NR"

#define TRANSPONDER_STRING_POLARIZATION_HORIZONTAL "horizontal"
#define TRANSPONDER_STRING_POLARIZATION_VERTICAL   "vertical"
#define TRANSPONDER_STRING_POLARIZATION_LEFT       "circular-left"
#define TRANSPONDER_STRING_POLARIZATION_RIGHT      "circular-right"

#define TRANSPONDER_STRING_ROLLOFF_35              "0.35"
#define TRANSPONDER_STRING_ROLLOFF_20              "0.20"
#define TRANSPONDER_STRING_ROLLOFF_25              "0.25"
#define TRANSPONDER_STRING_ROLLOFF_AUTO            "auto"

#define TRANSPONDER_STRING_TRANSMISSION_2K         "2K"
#define TRANSPONDER_STRING_TRANSMISSION_8K         "8K"
#define TRANSPONDER_STRING_TRANSMISSION_AUTO       "auto"
#define TRANSPONDER_STRING_TRANSMISSION_4K         "4K"
#define TRANSPONDER_STRING_TRANSMISSION_1K         "1K"
#define TRANSPONDER_STRING_TRANSMISSION_16K        "16K"
#define TRANSPONDER_STRING_TRANSMISSION_32K        "32K"
#define TRANSPONDER_STRING_TRANSMISSION_C1         "C1"
#define TRANSPONDER_STRING_TRANSMISSION_C3780      "C3780"

