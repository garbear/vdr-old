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

#define CHANNEL_XML_ROOT                  "channels"

#define CHANNEL_XML_ELM_CHANNEL           "channel"
#define CHANNEL_XML_ELM_SEPARATOR         "separator"
#define CHANNEL_XML_ELM_VPID              "videostream"     // Video stream packet ID
#define CHANNEL_XML_ELM_APID              "pid"             // Audio stream packet ID
#define CHANNEL_XML_ELM_APIDS             "audiostreams"
#define CHANNEL_XML_ELM_DPID              "pid"             // Data stream packet ID
#define CHANNEL_XML_ELM_DPIDS             "datastreams"
#define CHANNEL_XML_ELM_SPID              "pid"             // Subtitle stream packet ID
#define CHANNEL_XML_ELM_SPIDS             "subtitlestreams"
#define CHANNEL_XML_ELM_TPID              "teletextstream"  // Teletext stream packet ID
#define CHANNEL_XML_ELM_CAIDS             "caids"
#define CHANNEL_XML_ELM_CAID              "caid"
#define CHANNEL_XML_ELM_TRANSPONDER       "transponder"

#define CHANNEL_XML_ATTR_NAME             "name"
#define CHANNEL_XML_ATTR_SHORTNAME        "shortname"
#define CHANNEL_XML_ATTR_PROVIDER         "provider"
#define CHANNEL_XML_ATTR_NUMBER           "number"
#define CHANNEL_XML_ATTR_PPID             "pcr-pid"  // Program clock reference packet ID
#define CHANNEL_XML_ATTR_VTYPE            "type"     // Video stream type
#define CHANNEL_XML_ATTR_ALANG            "lang"     // Audio stream language
#define CHANNEL_XML_ATTR_ATYPE            "type"     // Audio stream type
#define CHANNEL_XML_ATTR_DLANG            "lang"     // Data stream language
#define CHANNEL_XML_ATTR_DTYPE            "type"     // Data stream type
#define CHANNEL_XML_ATTR_SLANG            "lang"     // Subtitle stream language

#define CHANNEL_SOURCE_XML_ATTR_SOURCE    "source"
#define CHANNEL_SOURCE_XML_ATTR_POSITION  "satelliteposition" // TODO: Add this property to cTransponder

#define CHANNEL_ID_XML_ATTR_NID           "networkid"
#define CHANNEL_ID_XML_ATTR_TSID          "transportstreamid"
#define CHANNEL_ID_XML_ATTR_SID           "serviceid"

#define TRANSPONDER_XML_ATTR_FREQUENCY    "frequency"
#define TRANSPONDER_XML_ATTR_SYMBOL_RATE  "symbolrate"
#define TRANSPONDER_XML_ATTR_POLARIZATION "polarization"
#define TRANSPONDER_XML_ATTR_INVERSION    "inversion"
#define TRANSPONDER_XML_ATTR_BANDWIDTH    "bandwidth"
#define TRANSPONDER_XML_ATTR_CODERATEH    "coderateh"
#define TRANSPONDER_XML_ATTR_CODERATEL    "coderatel"
#define TRANSPONDER_XML_ATTR_MODULATION   "modulation"
#define TRANSPONDER_XML_ATTR_SYSTEM       "system"
#define TRANSPONDER_XML_ATTR_TRANSMISSION "transmission"
#define TRANSPONDER_XML_ATTR_GUARD        "guard"
#define TRANSPONDER_XML_ATTR_HIERARCHY    "hierarchy"
#define TRANSPONDER_XML_ATTR_ROLLOFF      "rolloff"
#define TRANSPONDER_XML_ATTR_STREAMID     "streamid"
