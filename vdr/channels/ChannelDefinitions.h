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

#define CHANNEL_XML_ROOT              "channels"

#define CHANNEL_XML_ELM_CHANNEL       "channel"
#define CHANNEL_XML_ELM_SEPARATOR     "separator"
#define CHANNEL_XML_ELM_APIDS         "apids"
#define CHANNEL_XML_ELM_APID          "apid"
#define CHANNEL_XML_ELM_DPIDS         "dpids"
#define CHANNEL_XML_ELM_DPID          "dpid"
#define CHANNEL_XML_ELM_SPIDS         "spids"
#define CHANNEL_XML_ELM_SPID          "spid"
#define CHANNEL_XML_ELM_CAIDS         "caids"
#define CHANNEL_XML_ELM_CAID          "caid"
#define CHANNEL_XML_ELM_PARAMETERS    "parameters"

#define CHANNEL_XML_ATTR_NAME         "name"
#define CHANNEL_XML_ATTR_SHORTNAME    "shortname"
#define CHANNEL_XML_ATTR_PROVIDER     "provider"
#define CHANNEL_XML_ATTR_SEPARATOR    "separator"
#define CHANNEL_XML_ATTR_NUMBER       "number"
#define CHANNEL_XML_ATTR_PARAMETERS   "parameters"
#define CHANNEL_XML_ATTR_VPID         "vpid"
#define CHANNEL_XML_ATTR_PPID         "ppid"
#define CHANNEL_XML_ATTR_VTYPE        "vtype"
#define CHANNEL_XML_ATTR_TPID         "tpid"
#define CHANNEL_XML_ATTR_ALANG        "alang"
#define CHANNEL_XML_ATTR_ATYPE        "atype"
#define CHANNEL_XML_ATTR_DLANG        "dlang"
#define CHANNEL_XML_ATTR_DTYPE        "dtype"
#define CHANNEL_XML_ATTR_SLANG        "slang"

#define CHANNEL_SOURCE_XML_ATTR_SOURCE    "source"
#define CHANNEL_SOURCE_XML_ATTR_POSITION  "satellite_position"

#define CHANNEL_ID_XML_ATTR_NID  "nid"  // Network ID
#define CHANNEL_ID_XML_ATTR_TSID "tsid" // Transport stream ID
#define CHANNEL_ID_XML_ATTR_SID  "sid"  // Service ID

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
