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

#include "ChannelID.h"
#include "ChannelTypes.h"
#include "dvb/DVBTypes.h"
#include "epg/EPGTypes.h"
#include "transponders/Transponder.h"
#include "utils/List.h"
#include "utils/Observer.h"

#include <stdint.h>
#include <string>
#include <vector>

#define MAXLANGCODE1 4 // a 3 letter language code, zero terminated
#define MAXLANGCODE2 8 // up to two 3 letter language codes, separated by '+' and zero terminated

class TiXmlNode;

namespace VDR
{

class VideoStream
{
public:
  VideoStream(void) :
    vpid(0),
    vtype(STREAM_TYPE_UNDEFINED),
    ppid(0) {}

  uint16_t    vpid;  // Video stream packet ID
  STREAM_TYPE vtype; // Video stream type (see enum _stream_type in si_ext.h)
  uint16_t    ppid;  // Program clock reference packet ID
};

class AudioStream
{
public:
  AudioStream(void) :
    apid(0),
    atype(STREAM_TYPE_UNDEFINED) {}

  uint16_t    apid;  // Audio stream packet ID
  STREAM_TYPE atype; // Audio stream type (see enum _stream_type in si_ext.h)
  std::string alang; // Audio stream language
};

class DataStream // Alternatively, DolbyStream; stream types (AC3, DTS) are dolby
{
public:
  DataStream(void) :
    dpid(0),
    dtype(STREAM_TYPE_UNDEFINED) {}

  uint16_t    dpid;  // Data stream packet ID
  STREAM_TYPE dtype; // Data stream type (see enum _stream_type in si_ext.h)
  std::string dlang; // Data stream language
};

class SubtitleStream
{
public:
  SubtitleStream(void) :
    spid(0),
    subtitlingType(0),
    compositionPageId(0),
    ancillaryPageId(0) {}

  uint16_t    spid;              // Subtitle stream packet ID
  std::string slang;             // Subtitle stream language
  uint8_t     subtitlingType;    // Subtitling type (optional)
  uint16_t    compositionPageId; // Composition page ID (optional)
  uint16_t    ancillaryPageId;   // Ancillary page ID (optional)
};

class TeletextStream
{
public:
  TeletextStream(void) :
    tpid(0) {}

  uint16_t    tpid; // Teletext stream packet ID
};

// TODO: Move to cTransponder and figure out wtf this does
// I suspect that the polarization masking is involved somehow
bool ISTRANSPONDER(int frequencyMHz1, int frequencyMHz2);

/*!
 * Channel Abstraction
 *
 * Channels have the following properties:
 *    - ID:            Identifier object
 *    - UID:           Unique identifier derived from identifier object
 *    - Name:          Description of the service
 *    - Short name:    Brief description of the service
 *    - Provider name: Provider name
 *    - Portal name:   Portal name, not used in ATSC
 *    - Number:        The channel number, or major channel number in ATSC
 *    - Sub number:    The minor channel number in ATSC
 *    - Streams:       Properties describing the TS streams associated with this channel
 *    - Transponder:   The properties required to tune to this channel
 */
class cChannel : public Observable
{
public:
  static const ChannelPtr EmptyChannel;

  // Construct/deconstruct channel object
  cChannel(void);
  virtual ~cChannel(void) { }
  ChannelPtr Clone(void) const;
  cChannel& operator=(const cChannel& rhs);

  // Get channel identifiers
  const cChannelID& ID(void) const           { return m_channelId; }
  uint16_t          Nid()  const             { return m_channelId.Nid(); }
  uint16_t          Tsid() const             { return m_channelId.Tsid();; }
  uint16_t          Sid()  const             { return m_channelId.Sid();; }
  int32_t           ATSCSourceID(void) const { return m_channelId.ATSCSourceId(); }
  uint32_t          UID(void) const          { return m_channelId.Hash(); }

  // Set channel identifiers
  void SetId(const cChannelID& newId);
  void SetId(uint16_t nid          = 0,
             uint16_t tsid         = 0,
             uint16_t sid          = 0,
             int32_t  atscSourceId = ATSC_SOURCE_ID_NONE);
  void SetATSCSourceId(int32_t atscSourceId);

  // Get channel properties
  const std::string& Name(void) const       { return m_name; }
  const std::string& ShortName(void) const  { return m_shortName; }
  const std::string& Provider(void) const   { return m_provider; }
  const std::string& PortalName(void) const { return m_portalName; }
  unsigned int       Number(void) const     { return m_number; }
  unsigned int       SubNumber(void) const  { return m_subNumber; }

  // Set channel properties
  void SetName(const std::string& strName,
               const std::string& strShortName,
               const std::string& strProvider);
  void SetPortalName(const std::string& strPortalName);
  void SetNumber(unsigned int number, unsigned int subNumber = 0);

  // Get streams
  const VideoStream&                 GetVideoStream(void) const      { return m_videoStream; }
  const AudioStream&                 GetAudioStream(unsigned int index) const;
  const std::vector<AudioStream>&    GetAudioStreams(void) const     { return m_audioStreams; }
  const DataStream&                  GetDataStream(unsigned int index) const;
  const std::vector<DataStream>&     GetDataStreams(void) const      { return m_dataStreams; }
  const SubtitleStream&              GetSubtitleStream(unsigned int index) const;
  const std::vector<SubtitleStream>& GetSubtitleStreams(void) const  { return m_subtitleStreams; }
  const TeletextStream&              GetTeletextStream(void) const   { return m_teletextStream; }
  const CaDescriptorVector&          GetCaDescriptors(void) const    { return m_caDescriptors; }
  uint16_t                           GetCaId(unsigned int index) const;
  std::vector<uint16_t>              GetCaIds(void) const;
  std::set<uint16_t>                 GetPids(void) const; // Get the PIDs associated with all streams
  bool                               IsDataChannel(void) const { return GetVideoStream().vpid == 0 && GetAudioStreams().empty(); }

  // Set streams
  bool SetStreams(const VideoStream& videoStream,
                  const std::vector<AudioStream>& audioStreams,
                  const std::vector<DataStream>& dataStreams,
                  const std::vector<SubtitleStream>& subtitleStreams,
                  const TeletextStream& teletextStream);
  void SetSubtitlingDescriptors(const std::vector<SubtitleStream>& subtitleStreams);
  bool SetCaDescriptors(const CaDescriptorVector& caDescriptors);

  // Get transponder
  const cTransponder& GetTransponder(void) const { return m_transponder; }
        cTransponder& GetTransponder(void)       { return m_transponder; }
  unsigned int        FrequencyHz(void) const    { return m_transponder.FrequencyHz(); }
  unsigned int        FrequencyKHz(void) const   { return m_transponder.FrequencyKHz(); }
  unsigned int        FrequencyMHz(void) const   { return m_transponder.FrequencyMHz(); }

  // Set transponder
  void SetTransponder(const cTransponder& transponder);

  // A channel is valid if its ID is valid
  bool IsValid(void) const { return m_channelId.IsValid(); }

  bool Serialise(TiXmlNode* node) const;
  bool Deserialise(const TiXmlNode* node);

private:
  cChannelID                  m_channelId;
  std::string                 m_name;
  std::string                 m_shortName;
  std::string                 m_provider;
  std::string                 m_portalName;
  unsigned int                m_number;
  unsigned int                m_subNumber;
  VideoStream                 m_videoStream;     // Max 1
  std::vector<AudioStream>    m_audioStreams;    // Max 32
  std::vector<DataStream>     m_dataStreams;     // Max 16
  std::vector<SubtitleStream> m_subtitleStreams; // Max 32
  TeletextStream              m_teletextStream;  // Max 1
  CaDescriptorVector          m_caDescriptors;   // Max 12
  cTransponder                m_transponder;
};

}
