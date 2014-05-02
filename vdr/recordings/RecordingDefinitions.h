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

#define RECORDING_XML_ROOT                "metadata"

#define RECORDING_XML_ELM_RECORDING       "recording"
#define RECORDING_XML_ATTR_CHANNEL_ID     "channel_id"
#define RECORDING_XML_ATTR_CHANNEL_NAME   "channel_name"
#define RECORDING_XML_ATTR_CHANNEL_UID    "channel_uid"
#define RECORDING_XML_ATTR_FPS            "fps"
#define RECORDING_XML_ATTR_PRIORITY       "priority"
#define RECORDING_XML_ATTR_LIFETIME       "lifetime"

// XXX we might want to add support for multiple events here
#define RECORDING_XML_ELM_EPG_EVENT       "event"
#define RECORDING_XML_ATTR_EPG_EVENT_ID   "id"
#define RECORDING_XML_ATTR_EPG_START_TIME "start"
#define RECORDING_XML_ATTR_EPG_DURATION   "duration"
#define RECORDING_XML_ATTR_EPG_TABLE      "table"
#define RECORDING_XML_ATTR_EPG_VERSION    "version"
