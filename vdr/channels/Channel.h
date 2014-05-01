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

#include "ChannelID.h"
#include "ChannelTypes.h"
#include "dvb/DVBTypes.h"
#include "epg/EPGTypes.h"
#include "sources/linux/DVBTransponderParams.h"
#include "sources/Source.h"
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
  uint8_t  vtype; // Video stream type
  uint16_t ppid;  // Program clock reference packet ID
};

struct AudioStream
{
  uint16_t    apid;  // Audio stream packet ID
  uint8_t     atype; // Audio stream type
  std::string alang; // Audio stream language
};

struct DataStream // dolby (AC3 + DTS)
{
  uint16_t    dpid;  // Data stream packet ID
  uint8_t     dtype; // Data stream type
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
  const cChannel *Channel() const { return NULL; }
};

//#include "config.h"
//#include "thread.h"
//#include "tools.h"

bool ISTRANSPONDER(int frequencyMHz1, int frequencyMHz2); // XXX: Units?

enum eChannelMod
{
  CHANNELMOD_NONE   = 0x00,
  CHANNELMOD_ALL    = 0xFF,
  CHANNELMOD_NAME   = 0x01,
  CHANNELMOD_PIDS   = 0x02,
  CHANNELMOD_ID     = 0x04,
  CHANNELMOD_CA     = 0x10,
  CHANNELMOD_TRANSP = 0x20,
  CHANNELMOD_LANGS  = 0x40,
  CHANNELMOD_RETUNE = (CHANNELMOD_PIDS | CHANNELMOD_CA | CHANNELMOD_TRANSP)
};

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
  ChannelPtr Channel() { return m_channel; }

private:
  ChannelPtr m_channel;
};

class cLinkChannels : public cList<cLinkChannel>
{
};

//typedef std::vector<cLinkChannel*> cLinkChannels;

struct tChannelData
{
  int          iFrequencyHz; // MHz
  int          source;
  int          srate;
  unsigned int number;    // Sequence number assigned on load // TODO: Doesn't belong in tChannelData
};

class cSchedule;

class cChannel : public cListObject, public Observable
{
  //friend class cSchedules;
  //friend class cMenuEditChannel;
  //friend class cDvbSourceParam;

public:
  cChannel();
  cChannel(const cChannel &channel);
  ~cChannel();

  static ChannelPtr EmptyChannel;

  cChannel& operator=(const cChannel &channel);

  ChannelPtr Clone() const;

  const std::string& Name()       const { return m_name; }
  const std::string& ShortName()  const { return m_shortName; }
  const std::string& Provider()   const { return m_provider; }
  const std::string& PortalName() const { return m_portalName; }

  uint16_t GetNid() const { return m_nid; }
  uint16_t GetTid() const { return m_tid; }
  uint16_t GetSid() const { return m_sid; }
  uint16_t GetRid() const { return m_rid; }

  const VideoStream&                 GetVideoStream()                      const { return m_videoStream; }
  const AudioStream&                 GetAudioStream(unsigned int index)    const;
  const DataStream&                  GetDataStream(unsigned int index)     const;
  const SubtitleStream&              GetSubtitleStream(unsigned int index) const;
  const TeletextStream&              GetTeletextStream()                   const { return m_teletextStream; }
  uint16_t                           GetCaId(unsigned int i)               const;

  const std::vector<AudioStream>&    GetAudioStreams()    const { return m_audioStreams; }
  const std::vector<DataStream>&     GetDataStreams()     const { return m_dataStreams; }
  const std::vector<SubtitleStream>& GetSubtitleStreams() const { return m_subtitleStreams; }
  const CaDescriptorVector&          GetCaDescriptors()   const { return m_caDescriptors; }
  std::vector<uint16_t>              GetCaIds()           const;

  void SetName(const std::string &strName,
               const std::string &strShortName,
               const std::string &strProvider);

  void SetPortalName(const std::string &strPortalName);

  void SetId(uint16_t nid, uint16_t tid, uint16_t sid, uint16_t rid = 0);

  void SetStreams(const VideoStream& videoStream,
                  const std::vector<AudioStream>& audioStreams,
                  const std::vector<DataStream>& dataStreams,
                  const std::vector<SubtitleStream>& subtitleStreams,
                  const TeletextStream& teletextStream);

  void SetSubtitlingDescriptors(const std::vector<SubtitleStream>& subtitleStreams);

  void SetCaDescriptors(const CaDescriptorVector& caDescriptors);

  bool SerialiseChannel(TiXmlNode *node) const;
  bool Deserialise(const TiXmlNode *node);
  bool DeserialiseConf(const std::string &str);

  /*!
   * \brief Returns the actual frequency in KHz, as given in 'channels.conf'
   */
  int FrequencyKHz() const { return m_channelData.iFrequencyHz / 1000; }

  /*!
   * \brief Returns the actual frequency in Hz XXX
   */
  int FrequencyHz() const { return m_channelData.iFrequencyHz; }

  void SetFrequencyHz(int frequency) { m_channelData.iFrequencyHz = frequency; }

  /*!
   * \brief Returns the transponder frequency in MHz, plus the polarization in case of sat
   */
  int TransponderFrequency() const;

  /*!
   * \brief Builds the transponder from the given frequency and polarization
   * \param frequency The transponder frequency before polarization is taken into account
   * \param polarization The transponder polarization
   * \return The adjusted frequency
   *
   * Some satellites have transponders at the same frequency, just with different
   * polarization.
   */
  static unsigned int Transponder(unsigned int frequency, fe_polarization polarization);

  int Source() const                                 { return m_channelData.source; }
  int Srate() const                                  { return m_channelData.srate; }

  unsigned int Number()             const { return m_channelData.number; }

  bool IsAtsc()                     const { return cSource::IsAtsc(m_channelData.source); }
  bool IsCable()                    const { return cSource::IsCable(m_channelData.source); }
  bool IsSat()                      const { return cSource::IsSat(m_channelData.source); }
  bool IsTerr()                     const { return cSource::IsTerr(m_channelData.source); }
  bool IsSourceType(char source)    const { return cSource::IsType(m_channelData.source, source); }

  void SetNumber(unsigned int number) { m_channelData.number = number; }

  const cDvbTransponderParams& Parameters() const { return m_parameters; }
  //const cLinkChannels& LinkChannels()       const { return m_linkChannels; }
  const cChannel* RefChannel()              const { return m_refChannel; }

  //void SetLinkChannels(cLinkChannels& channels) { m_linkChannels = channels; }

  tChannelID GetChannelID() const;

  bool HasTimer(void) const { return false; } //TODO
  bool HasTimer(const std::vector<cTimer2> &timers) const; // TODO: cTimer2

  eChannelMod Modification(eChannelMod mask = CHANNELMOD_ALL);

  void CopyTransponderData(const cChannel &channel);

  bool SetTransponderData(int source, int frequency, int srate, const cDvbTransponderParams& parameters, bool bQuiet = false);
  void SetLinkChannels(cLinkChannels *linkChannels);
  void SetRefChannel(cChannel *refChannel) { m_refChannel = refChannel; }

  void SetSchedule(SchedulePtr schedule);
  bool HasSchedule(void) const;
  SchedulePtr Schedule(void) const;

  uint32_t Hash(void) const;

private:
  std::string m_name;
  std::string m_shortName;
  std::string m_provider;
  std::string m_portalName;

  uint16_t m_nid; // Network ID
  uint16_t m_tid; // Transport stream ID
  uint16_t m_sid; // Service ID
  uint16_t m_rid; // ???

  VideoStream                 m_videoStream;
  std::vector<AudioStream>    m_audioStreams;    // Max 32
  std::vector<DataStream>     m_dataStreams;     // Max 16
  std::vector<SubtitleStream> m_subtitleStreams; // Max 32
  TeletextStream              m_teletextStream;
  CaDescriptorVector          m_caDescriptors;   // Max 12

  tChannelData          m_channelData;
  cDvbTransponderParams m_parameters;
  int                   m_modification;
  SchedulePtr           m_schedule;
  cLinkChannels*        m_linkChannels;
  //cLinkChannels         m_linkChannels;
  cChannel*             m_refChannel;
  uint32_t              m_channelHash;
};

}
