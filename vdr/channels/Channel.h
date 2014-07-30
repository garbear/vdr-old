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

class TiXmlNode;

namespace VDR
{

struct VideoStream
{
  uint16_t vpid;  // Video stream packet ID
  uint8_t  vtype; // Video stream type (see enum _stream_type in si_ext.h)
  uint16_t ppid;  // Program clock reference packet ID
};

struct AudioStream
{
  uint16_t    apid;  // Audio stream packet ID
  uint8_t     atype; // Audio stream type (see enum _stream_type in si_ext.h)
  std::string alang; // Audio stream language
};

struct DataStream // dolby (AC3 + DTS)
{
  uint16_t    dpid;  // Data stream packet ID
  uint8_t     dtype; // Data stream type (see enum _stream_type in si_ext.h)
  std::string dlang; // Data stream language
};

struct SubtitleStream
{
  uint16_t    spid;              // Subtitle stream packet ID
  std::string slang;             // Subtitle stream language
  uint8_t     subtitlingType;    // Subtitling type (optional)
  uint16_t    compositionPageId; // Composition page ID (optional)
  uint16_t    ancillaryPageId;   // Ancillary page ID (optional)
};

struct TeletextStream
{
  uint16_t tpid; // Teletext stream packet ID
};


#define CHANNEL_NAME_UNKNOWN  "???"

// TODO
class cTimer2
{
public:
  const cChannel *Channel(void) const { return NULL; }
};

//#include "config.h"
//#include "thread.h"
//#include "tools.h"

// TODO: Move to cTransponder and figure out wtf this does
// I suspect that the polarization masking is involved somehow
bool ISTRANSPONDER(int frequencyMHz1, int frequencyMHz2);

#define MAXLANGCODE1 4 // a 3 letter language code, zero terminated
#define MAXLANGCODE2 8 // up to two 3 letter language codes, separated by '+' and zero terminated

#define CA_FTA           0x0000
#define CA_DVB_MIN       0x0001
#define CA_DVB_MAX       0x000F
#define CA_USER_MIN      0x0010
#define CA_USER_MAX      0x00FF
#define CA_ENCRYPTED_MIN 0x0100
#define CA_ENCRYPTED_MAX 0xFFFF

class cLinkChannel : public cListObject
{
public:
  cLinkChannel(ChannelPtr channel) : m_channel(channel) { }
  ChannelPtr Channel(void) { return m_channel; }

private:
  ChannelPtr m_channel;
};

class cLinkChannels : public cList<cLinkChannel>
{
};

//typedef std::vector<cLinkChannel*> cLinkChannels;

class cSchedule;

class cChannel : public cListObject, public Observable
{
public:
  cChannel(void);
  cChannel(const cChannel& channel);
  ~cChannel(void);

  void Reset(void);

  cChannel& operator=(const cChannel& rhs);

  ChannelPtr Clone(void) const;

  static const ChannelPtr EmptyChannel;

  const cChannelID& ID(void) const { return m_channelId; }
  void SetId(uint16_t nid, uint16_t tsid, uint16_t sid);
  uint32_t UID(void) const { return m_channelId.Hash(); }

  const std::string& Name(void)       const { return m_name; }
  const std::string& ShortName(void)  const { return m_shortName; }
  const std::string& Provider(void)   const { return m_provider; }
  const std::string& PortalName(void) const { return m_portalName; }

  void SetName(const std::string& strName,
               const std::string& strShortName,
               const std::string& strProvider);

  void SetPortalName(const std::string& strPortalName);

  /*!
   *
   */
  const VideoStream&                 GetVideoStream(void)                  const { return m_videoStream; }
  const AudioStream&                 GetAudioStream(unsigned int index)    const;
  const DataStream&                  GetDataStream(unsigned int index)     const;
  const SubtitleStream&              GetSubtitleStream(unsigned int index) const;
  const TeletextStream&              GetTeletextStream(void)               const { return m_teletextStream; }
  uint16_t                           GetCaId(unsigned int index)           const;

  const std::vector<AudioStream>&    GetAudioStreams(void)    const { return m_audioStreams; }
  const std::vector<DataStream>&     GetDataStreams(void)     const { return m_dataStreams; }
  const std::vector<SubtitleStream>& GetSubtitleStreams(void) const { return m_subtitleStreams; }
  const CaDescriptorVector&          GetCaDescriptors(void)   const { return m_caDescriptors; }
  std::vector<uint16_t>              GetCaIds(void)           const;

  void SetStreams(const VideoStream& videoStream,
                  const std::vector<AudioStream>& audioStreams,
                  const std::vector<DataStream>& dataStreams,
                  const std::vector<SubtitleStream>& subtitleStreams,
                  const TeletextStream& teletextStream);

  void SetSubtitlingDescriptors(const std::vector<SubtitleStream>& subtitleStreams);

  void SetCaDescriptors(const CaDescriptorVector& caDescriptors);

  const cTransponder& GetTransponder(void) const       { return m_transponder; }
        cTransponder& GetTransponder(void)             { return m_transponder; }
  void SetTransponder(const cTransponder& transponder) { m_transponder = transponder; }

  unsigned int Number(void) const { return m_transponder.ChannelNumber() * 10 + m_channelId.Sid(); } // TODO

  /*!
   * \brief Returns the transponder frequency in MHz, plus the polarization in
   * the case of a satellite. The polarization takes the form of a mask in the
   * 100 GHz range (see cTransponder::WTF()).
   */
  unsigned int FrequencyMHzWithPolarization() const;

  SchedulePtr Schedule(void) const;
  bool HasSchedule(void) const;
  void SetSchedule(const SchedulePtr& schedule);

  //const cLinkChannels& LinkChannels(void)       const { return m_linkChannels; }
  //void SetLinkChannels(cLinkChannels& channels) { m_linkChannels = channels; }
  void SetLinkChannels(cLinkChannels *linkChannels);

  bool HasTimer(void) const { return false; } //TODO
  bool HasTimer(const std::vector<cTimer2> &timers) const; // TODO: cTimer2

  bool Serialise(TiXmlNode* node) const;
  bool Deserialise(const TiXmlNode* node);

private:
  cChannelID                  m_channelId;
  std::string                 m_name;
  std::string                 m_shortName;
  std::string                 m_provider;
  std::string                 m_portalName;

  VideoStream                 m_videoStream;     // Max 1
  std::vector<AudioStream>    m_audioStreams;    // Max 32
  std::vector<DataStream>     m_dataStreams;     // Max 16
  std::vector<SubtitleStream> m_subtitleStreams; // Max 32
  TeletextStream              m_teletextStream;  // Max 1
  CaDescriptorVector          m_caDescriptors;   // Max 12

  cTransponder                m_transponder;

  SchedulePtr                 m_schedule;
  cLinkChannels*              m_linkChannels;
  //cLinkChannels               m_linkChannels;
};

}
