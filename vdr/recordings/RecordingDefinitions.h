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
