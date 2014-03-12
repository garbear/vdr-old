/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#define SETTINGS_XML_ROOT                              "vdr"

#define SETTINGS_XML_ATTR_VALUE                        "value"

#define SETTINGS_XML_ELM_LISTEN_PORT                   "listen_port"
#define SETTINGS_XML_ELM_SET_SYSTEM_TIME               "set_system_time"
#define SETTINGS_XML_ELM_TIME_SOURCE                   "time_source"
#define SETTINGS_XML_ELM_TIME_TRANSPONDER              "time_transponder"
#define SETTINGS_XML_ELM_UPDATE_CHANNELS_LEVEL         "update_channels"
#define SETTINGS_XML_ELM_STREAM_TIMEOUT                "stream_timeout"
#define SETTINGS_XML_ELM_PMT_TIMEOUT                   "pmt_timeout"
#define SETTINGS_XML_ELM_STANDARD_COMPLIANCE           "standard_compliance"
#define SETTINGS_XML_ELM_RESUME_ID                     "resume_id"
#define SETTINGS_XML_ELM_DEVICE_BONDINGS               "device_bondings"
#define SETTINGS_XML_ELM_NEXT_WAKEUP                   "next_wakeup"

#define SETTINGS_XML_ELM_INSTANT_RECORD_TIME           "instant_record_time"
#define SETTINGS_XML_ELM_DEFAULT_RECORDING_PRIORITY    "default_record_prio"
#define SETTINGS_XML_ELM_DEFAULT_RECORDING_LIFETIME    "default_record_lifetime"
#define SETTINGS_XML_ELM_RECORD_SUBTITLE_NAME          "record_subtitle_name"
#define SETTINGS_XML_ELM_USE_VPS                       "use_vps"
#define SETTINGS_XML_ELM_VPS_MARGIN                    "vps_margin"
#define SETTINGS_XML_ELM_MAX_RECORDING_FILESIZE_MB     "max_recording_size_mb"
#define SETTINGS_XML_ELM_MARGIN_START                  "margin_start"
#define SETTINGS_XML_ELM_MARGIN_END                    "margin_end"

#define SETTINGS_XML_ELM_LNB_SLOF                      "lnb_lsof"
#define SETTINGS_XML_ELM_LNB_FREQ_LOW                  "lnb_freq_low"
#define SETTINGS_XML_ELM_LNB_FREQ_HIGH                 "lnb_freq_high"
#define SETTINGS_XML_ELM_DISEQC                        "diseqc"

#define SETTINGS_XML_ELM_EPG_SCAN_TIMEOUT              "epg_scan_timeout"
#define SETTINGS_XML_ELM_EPG_BUGFIX_LEVEL              "epg_bugfix_level"
#define SETTINGS_XML_ELM_EPG_LINGER_TIME               "epg_linger_time"
#define SETTINGS_XML_ELM_EPG_LANGUAGES                 "epg_languages"

#define SETTINGS_XML_ELM_TIMESHIFT_MODE                "timeshift_mode"
#define SETTINGS_XML_ELM_TIMESHIFT_BUFFER_SIZE         "timeshift_buffer_size"
#define SETTINGS_XML_ELM_TIMESHIFT_BUFFER_FILE_SIZE    "timeshift_buffer_file_size"
#define SETTINGS_XML_ELM_TIMESHIFT_BUFFER_FILE         "timeshift_buffer_dir"

#define HOSTS_XML_ROOT                                 "hosts"
#define HOSTS_XML_ELM_HOST                             "host"
