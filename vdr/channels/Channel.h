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

//#include "config.h"
//#include "thread.h"
#include "tools.h"

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

class cLinkChannels : public cList<cLinkChannel>
{
};

class cSchedule;

class cChannel : public cListObject
{
  friend class cSchedules;
  friend class cMenuEditChannel;
  friend class cDvbSourceParam;

public:
  cChannel();
  cChannel(const cChannel &Channel);
  ~cChannel();

  cChannel& operator= (const cChannel &Channel);

  cString ToText() const;
  bool Parse(const char *s);
  bool Save(FILE *f);

  const char *Name() const;
  const char *ShortName(bool OrName = false) const;
  const char *Provider() const { return provider; }
  const char *PortalName() const { return portalName; }
  int Frequency() const { return frequency; } ///< Returns the actual frequency, as given in 'channels.conf'
  int Transponder() const;                    ///< Returns the transponder frequency in MHz, plus the polarization in case of sat
  static int Transponder(int Frequency, char Polarization); ///< builds the transponder from the given Frequency and Polarization

  int Source()             const { return m_source; }
  int Srate()              const { return srate; }
  int Vpid()               const { return vpid; }
  int Ppid()               const { return ppid; }
  int Vtype()              const { return vtype; }
  const int *Apids()       const { return apids; }
  const int *Dpids()       const { return dpids; }
  const int *Spids()       const { return spids; }
  int Apid(int i)          const { return (0 <= i && i < MAXAPIDS) ? apids[i] : 0; }
  int Dpid(int i)          const { return (0 <= i && i < MAXDPIDS) ? dpids[i] : 0; }
  int Spid(int i)          const { return (0 <= i && i < MAXSPIDS) ? spids[i] : 0; }
  const char *Alang(int i) const { return (0 <= i && i < MAXAPIDS) ? alangs[i] : ""; }
  const char *Dlang(int i) const { return (0 <= i && i < MAXDPIDS) ? dlangs[i] : ""; }
  const char *Slang(int i) const { return (0 <= i && i < MAXSPIDS) ? slangs[i] : ""; }
  int Atype(int i)         const { return (0 <= i && i < MAXAPIDS) ? atypes[i] : 0; }
  int Dtype(int i)         const { return (0 <= i && i < MAXDPIDS) ? dtypes[i] : 0; }
  uchar SubtitlingType(int i)       const { return (0 <= i && i < MAXSPIDS) ? subtitlingTypes[i] : uchar(0); }
  uint16_t CompositionPageId(int i) const { return (0 <= i && i < MAXSPIDS) ? compositionPageIds[i] : uint16_t(0); }
  uint16_t AncillaryPageId(int i)   const { return (0 <= i && i < MAXSPIDS) ? ancillaryPageIds[i] : uint16_t(0); }
  int Tpid()               const { return tpid; }
  const int *Caids()       const { return caids; }
  int Ca(int Index = 0)    const { return Index < MAXCAIDS ? caids[Index] : 0; }
  int Nid()                const { return nid; }
  int Tid()                const { return tid; }
  int Sid()                const { return sid; }
  int Rid()                const { return rid; }
  int Number()             const { return number; }

  void SetNumber(int Number) { number = Number; }

  bool GroupSep()          const { return groupSep; }
  const std::string &Parameters() const { return parameters; }
  const cLinkChannels* LinkChannels() const { return linkChannels; }
  const cChannel *RefChannel() const { return refChannel; }
  bool IsAtsc()            const { return cSource::IsAtsc(m_source); }
  bool IsCable()           const { return cSource::IsCable(m_source); }
  bool IsSat()             const { return cSource::IsSat(m_source); }
  bool IsTerr()            const { return cSource::IsTerr(m_source); }
  bool IsSourceType(char Source) const { return cSource::IsType(m_source, Source); }
  tChannelID GetChannelID() const { return tChannelID(m_source, nid, (nid || tid) ? tid : Transponder(), sid, rid); }

  bool HasTimer() const;
  int Modification(int Mask = CHANNELMOD_ALL);
  void CopyTransponderData(const cChannel *Channel);

  bool SetTransponderData(int Source, int Frequency, int Srate, const char *Parameters, bool Quiet = false);
  void SetId(int Nid, int Tid, int Sid, int Rid = 0);
  void SetName(const char *Name, const char *ShortName, const char *Provider);
  void SetPortalName(const char *PortalName);
  void SetPids(int Vpid, int Ppid, int Vtype, int *Apids, int *Atypes, char ALangs[][MAXLANGCODE2], int *Dpids, int *Dtypes, char DLangs[][MAXLANGCODE2], int *Spids, char SLangs[][MAXLANGCODE2], int Tpid);
  void SetCaIds(const int *CaIds); // list must be zero-terminated
  void SetCaDescriptors(int Level);
  void SetLinkChannels(cLinkChannels *LinkChannels);
  void SetRefChannel(cChannel *RefChannel);
  void SetSubtitlingDescriptors(uchar *SubtitlingTypes, uint16_t *CompositionPageIds, uint16_t *AncillaryPageIds);

private:
  static cString ToText(const cChannel *Channel);
  cString TransponderDataToString() const;

  char                    *name;
  char                    *shortName;
  char                    *provider;
  char                    *portalName;
  int                      __BeginData__;
  int                      frequency; // MHz
  int                      m_source;
  int                      srate;
  int                      vpid;
  int                      ppid;
  int                      vtype;
  int                      apids[MAXAPIDS + 1]; // list is zero-terminated
  int                      atypes[MAXAPIDS + 1]; // list is zero-terminated
  char                     alangs[MAXAPIDS][MAXLANGCODE2];
  int                      dpids[MAXDPIDS + 1]; // list is zero-terminated
  int                      dtypes[MAXAPIDS + 1]; // list is zero-terminated
  char                     dlangs[MAXDPIDS][MAXLANGCODE2];
  int                      spids[MAXSPIDS + 1]; // list is zero-terminated
  char                     slangs[MAXSPIDS][MAXLANGCODE2];
  uchar                    subtitlingTypes[MAXSPIDS];
  uint16_t                 compositionPageIds[MAXSPIDS];
  uint16_t                 ancillaryPageIds[MAXSPIDS];
  int                      tpid;
  int                      caids[MAXCAIDS + 1]; // list is zero-terminated
  int                      nid;
  int                      tid;
  int                      sid;
  int                      rid;
  int                      number;    // Sequence number assigned on load
  bool                     groupSep;
  int                      __EndData__;
  mutable std::string      nameSource;
  mutable std::string      shortNameSource;
  std::string              parameters;
  int                      modification;
  mutable const cSchedule *schedule;
  cLinkChannels           *linkChannels;
  cChannel                *refChannel;
};
