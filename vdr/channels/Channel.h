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
#include "sources/Source.h"
//#include "timers.h"

class cChannel;

// TODO
class cTimer2
{
public:
  const cChannel *Channel() const { return NULL; }
};

//#include "config.h"
//#include "thread.h"
//#include "tools.h"

#include <string>
#include <vector>

#define ISTRANSPONDER(f1, f2)  (abs((f1) - (f2)) < 4) //XXX

#define CHANNELMOD_NONE     0x00
#define CHANNELMOD_ALL      0xFF
#define CHANNELMOD_NAME     0x01
#define CHANNELMOD_PIDS     0x02
#define CHANNELMOD_ID       0x04
#define CHANNELMOD_CA       0x10
#define CHANNELMOD_TRANSP   0x20
#define CHANNELMOD_LANGS    0x40
#define CHANNELMOD_RETUNE   (CHANNELMOD_PIDS | CHANNELMOD_CA | CHANNELMOD_TRANSP)

#define CHANNELSMOD_NONE    0
#define CHANNELSMOD_AUTO    1
#define CHANNELSMOD_USER    2

#define MAXAPIDS 32 // audio
#define MAXDPIDS 16 // dolby (AC3 + DTS)
#define MAXSPIDS 32 // subtitles
#define MAXCAIDS 12 // conditional access

#define MAXLANGCODE1 4 // a 3 letter language code, zero terminated
#define MAXLANGCODE2 8 // up to two 3 letter language codes, separated by '+' and zero terminated

#define CA_FTA           0x0000
#define CA_DVB_MIN       0x0001
#define CA_DVB_MAX       0x000F
#define CA_USER_MIN      0x0010
#define CA_USER_MAX      0x00FF
#define CA_ENCRYPTED_MIN 0x0100
#define CA_ENCRYPTED_MAX 0xFFFF

class cChannel;

class cLinkChannel : public cListObject
{
public:
  cLinkChannel(cChannel *channel) : m_channel(channel) { }
  cChannel *Channel() { return m_channel; }

private:
  cChannel *m_channel;
};

/*
class cLinkChannels : public cList<cLinkChannel>
{
};
*/
typedef std::vector<cLinkChannel> cLinkChannels;

struct tChannelData
{
  int      frequency; // MHz
  int      source;
  int      srate;
  int      vpid;
  int      ppid;
  int      vtype;

  int      apids[MAXAPIDS + 1]; // list is zero-terminated
  int      atypes[MAXAPIDS + 1]; // list is zero-terminated
  char     alangs[MAXAPIDS][MAXLANGCODE2];

  int      dpids[MAXDPIDS + 1]; // list is zero-terminated
  int      dtypes[MAXAPIDS + 1]; // list is zero-terminated
  char     dlangs[MAXDPIDS][MAXLANGCODE2];

  int      spids[MAXSPIDS + 1]; // list is zero-terminated
  char     slangs[MAXSPIDS][MAXLANGCODE2];

  uchar    subtitlingTypes[MAXSPIDS];
  uint16_t compositionPageIds[MAXSPIDS];
  uint16_t ancillaryPageIds[MAXSPIDS];

  int      tpid;
  int      caids[MAXCAIDS + 1]; // list is zero-terminated
  int      nid;
  int      tid;
  int      sid;
  int      rid;
  int      number;    // Sequence number assigned on load
  bool     groupSep;
};

class cSchedule;

class cChannel// : public cListObject
{
  //friend class cSchedules;
  //friend class cMenuEditChannel;
  //friend class cDvbSourceParam;

public:
  cChannel();
  cChannel(const cChannel &channel);
  ~cChannel();

  cChannel& operator=(const cChannel &channel);

  std::string Name() const;
  std::string ShortName(bool bOrName = false) const;
  const std::string &Provider() const { return m_provider; }
  const std::string &PortalName() const { return m_portalName; }

  std::string ToText() const { return ToText(*this); }
  bool Parse(const std::string &str);
  bool Save(FILE *f) const;

  /*!
   * \brief Returns the actual frequency, as given in 'channels.conf'
   */
  int Frequency() const { return m_channelData.frequency; }

  /*!
   * \brief Returns the transponder frequency in MHz, plus the polarization in case of sat
   */
  int Transponder() const;

  /*!
   * \brief Builds the transponder from the given frequency and polarization
   * \param frequency The transponder frequency before polarization is taken into account
   * \param polarization Either 'H', 'V', 'L' or 'R' (lowercase is ok)
   * \return The adjusted frequency
   *
   * Some satellites have transponders at the same frequency, just with different
   * polarization.
   */
  static int Transponder(int frequency, char polarization);

  int Source()                      const { return m_channelData.source; }
  int Srate()                       const { return m_channelData.srate; }
  int Vpid()                        const { return m_channelData.vpid; }
  int Ppid()                        const { return m_channelData.ppid; }
  int Vtype()                       const { return m_channelData.vtype; }
  const int *Apids()                const { return m_channelData.apids; }
  const int *Dpids()                const { return m_channelData.dpids; }
  const int *Spids()                const { return m_channelData.spids; }
  int Apid(int i)                   const { return (0 <= i && i < MAXAPIDS) ? m_channelData.apids[i] : 0; }
  int Dpid(int i)                   const { return (0 <= i && i < MAXDPIDS) ? m_channelData.dpids[i] : 0; }
  int Spid(int i)                   const { return (0 <= i && i < MAXSPIDS) ? m_channelData.spids[i] : 0; }
  const char *Alang(int i)          const { return (0 <= i && i < MAXAPIDS) ? m_channelData.alangs[i] : ""; }
  const char *Dlang(int i)          const { return (0 <= i && i < MAXDPIDS) ? m_channelData.dlangs[i] : ""; }
  const char *Slang(int i)          const { return (0 <= i && i < MAXSPIDS) ? m_channelData.slangs[i] : ""; }
  int Atype(int i)                  const { return (0 <= i && i < MAXAPIDS) ? m_channelData.atypes[i] : 0; }
  int Dtype(int i)                  const { return (0 <= i && i < MAXDPIDS) ? m_channelData.dtypes[i] : 0; }
  uchar SubtitlingType(int i)       const { return (0 <= i && i < MAXSPIDS) ? m_channelData.subtitlingTypes[i] : (uchar)0; }
  uint16_t CompositionPageId(int i) const { return (0 <= i && i < MAXSPIDS) ? m_channelData.compositionPageIds[i] : (uint16_t)0; }
  uint16_t AncillaryPageId(int i)   const { return (0 <= i && i < MAXSPIDS) ? m_channelData.ancillaryPageIds[i] : (uint16_t)0; }
  int Tpid()                        const { return m_channelData.tpid; }
  const int *Caids()                const { return m_channelData.caids; }
  int Ca(int index = 0)             const { return index < MAXCAIDS ? m_channelData.caids[index] : 0; }
  int Nid()                         const { return m_channelData.nid; }
  int Tid()                         const { return m_channelData.tid; }
  int Sid()                         const { return m_channelData.sid; }
  int Rid()                         const { return m_channelData.rid; }
  int Number()                      const { return m_channelData.number; }
  bool GroupSep()                   const { return m_channelData.groupSep; }

  bool IsAtsc()                     const { return cSource::IsAtsc(m_channelData.source); }
  bool IsCable()                    const { return cSource::IsCable(m_channelData.source); }
  bool IsSat()                      const { return cSource::IsSat(m_channelData.source); }
  bool IsTerr()                     const { return cSource::IsTerr(m_channelData.source); }
  bool IsSourceType(char source)    const { return cSource::IsType(m_channelData.source, source); }

  void SetNumber(int number) { m_channelData.number = number; }

  const std::string &Parameters()     const { return m_parameters; }
  const cLinkChannels &LinkChannels() const { return m_linkChannels; }
  const cChannel *RefChannel()        const { return m_refChannel; }

  tChannelID GetChannelID() const;

  bool HasTimer(const std::vector<cTimer2> &timers) const; // TODO: cTimer2

  int Modification(int mask = CHANNELMOD_ALL);

  void CopyTransponderData(const cChannel &channel);

  bool SetTransponderData(int source, int frequency, int srate, const std::string &strParameters, bool bQuiet = false);
  void SetId(int nid, int tid, int sid, int rid = 0);
  void SetName(const std::string &strName, const std::string &strShortName, const std::string &strProvider);
  void SetPortalName(const std::string &strPortalName);
  void SetPids(int vpid, int ppid, int vtype, int *apids, int *atypes, char aLangs[][MAXLANGCODE2],
                                              int *dpids, int *dtypes, char dLangs[][MAXLANGCODE2],
                                              int *spids, char sLangs[][MAXLANGCODE2], int tpid);
  void SetCaIds(const int *caIds); // list must be zero-terminated
  void SetCaDescriptors(int level);
  //void SetLinkChannels(cLinkChannels *linkChannels);
  void SetRefChannel(cChannel *refChannel) { m_refChannel = refChannel; }
  void SetSubtitlingDescriptors(uchar *subtitlingTypes, uint16_t *compositionPageIds, uint16_t *ancillaryPageIds);

private:
  static std::string ToText(const cChannel &channel);
  std::string TransponderDataToString() const;

  std::string              m_name;
  std::string              m_shortName;
  std::string              m_provider;
  std::string              m_portalName;

  tChannelData             m_channelData;

  mutable std::string      m_nameSource;
  mutable std::string      m_shortNameSource;
  std::string              m_parameters;
  int                      m_modification;
  mutable const cSchedule *m_schedule;
  //cLinkChannels           *m_linkChannels;
  cLinkChannels            m_linkChannels;
  cChannel                *m_refChannel;
};