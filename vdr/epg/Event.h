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

#define MAXEPGBUGFIXLEVEL 3
#define VDR_RATINGS_PATCHED_V2

#include "Component.h"
#include "EPGTypes.h"
#include "channels/ChannelID.h"
#include "channels/ChannelTypes.h"
#include "utils/DateTime.h"
#include "utils/Observer.h"

//#include <libsi/si.h> // for SI::RunningStatus
#include <stdint.h>
#include <string>
#include <vector>

class TiXmlElement;

namespace VDR
{

class cEvent : public Observable
{
public:
  cEvent(unsigned int eventID);
  virtual ~cEvent(void) { }
  void Reset(void);

  static const EventPtr EmptyEvent;

  cEvent& operator=(const cEvent& rhs);

  /*!
   * Event ID of this event
   */
  unsigned int ID(void) const { return m_eventID; }

  /*!
   * ID used to matched events from the EIT table to channels in the VCT
   * (ATSC only)
   */
  uint32_t AtscSourceID(void) const { return m_atscSourceId; }
  void SetAtscSourceID(uint32_t atscSourceId);

  /*!
   * Title of this event
   */
  const std::string& Title(void) const { return m_strTitle; }
  void SetTitle(const std::string& strTitle);

  /*!
   * Short description of this event (typically the episode name in case of a series)
   */
  const std::string& PlotOutline(void) const { return m_strPlotOutline; }
  void SetPlotOutline(const std::string& strPlotOutline);

  /*!
   * Description of this event
   */
  const std::string& Plot(void) const { return m_strPlot; }
  void SetPlot(const std::string& strPlot);

  /*!
   * ID of the channel this event belongs to
   */
  const cChannelID& ChannelID(void) const { return m_channelID; }
  void SetChannelID(const cChannelID& channelId);

  /*!
   * Start time of this event
   */
  const CDateTime& StartTime(void) const { return m_startTime; }
  time_t StartTimeAsTime(void)     const { time_t retval; StartTime().GetAsTime(retval); return retval; }
  void SetStartTime(const CDateTime& startTime);

  /*!
   * End time of this event
   */
  const CDateTime& EndTime(void) const { return m_endTime; }
  time_t EndTimeAsTime(void)     const { time_t retval; EndTime().GetAsTime(retval); return retval; }
  void SetEndTime(const CDateTime& endTime);

  /*!
   * Duration (computed from start time and end time)
   */
  CDateTimeSpan Duration(void)    const { return m_endTime - m_startTime; }
  unsigned int DurationSecs(void) const { return (m_endTime - m_startTime).GetSecondsTotal(); }

  /*!
   * Genre and sub-genre
   */
  EPG_GENRE Genre(void) const { return m_genreType; }
  EPG_SUB_GENRE SubGenre(void) const { return m_genreSubType; }
  void SetGenre(EPG_GENRE genre, EPG_SUB_GENRE subGenre);

  /*!
   * Custom genre string, if genre and sub-genre do not match the available
   * types. Used only if Genre() == EPG_GENRE_CUSTOM. Setting a custom genre
   * will force genre to EPG_GENRE_CUSTOM.
   */
  const std::string& CustomGenre(void) const { return m_strCustomGenre; }
  void SetCustomGenre(const std::string& strCustomGenre);

  /*!
   * Parental rating of this event
   */
  uint16_t ParentalRating(void) const { return m_parentalRating; }
  std::string ParentalRatingString(void) const; // TODO: Move to EPGStringifier
  void SetParentalRating(uint16_t parentalRating);

  /*!
   * Dish/BEV star rating
   */
  uint8_t StarRating(void) const { return m_starRating; }
  const char* StarRatingString(void) const; // TODO: Move to EPGStringifier
  void SetStarRating(uint8_t starRating);

  /*!
   * Table ID this event came from (actual table ids are 0x4E..0x60)
   */
  uint8_t TableID(void) const { return m_tableID; }
  void SetTableID(uint8_t tableID);

  /*!
   * Version number of section this event came from (actual version numbers are 0..31)
   */
  uint8_t Version(void) const { return m_version; }
  void SetVersion(uint8_t version);

  /*!
   * Video Programming Service timestamp (VPS, aka "Programme Identification Label", PIL)
   */
  bool HasVps(void) const            { return m_vps.IsValid(); }
  const CDateTime& Vps(void) const   { return m_vps; }
  std::string VpsString(void) const  { return m_vps.GetAsSaveString(); }
  void SetVps(const CDateTime& vps);

  /*!
   * The stream components of this event
   */
  const std::vector<CEpgComponent>& Components(void) const { return m_components; }
  void SetComponents(const std::vector<CEpgComponent>& components);

  /*!
   * Contents of this event (Max: 4 contents)
   */
  const std::vector<uint8_t>& Contents(void) const { return m_contents; }
  uint8_t GetContents(unsigned int i = 0) const    { return i < m_contents.size() ? m_contents[i] : 0; }
  void SetContents(const std::vector<uint8_t>& contents);

  /*!
   * Convert this event to its string representation (guaranteed to be a valid filename)
   */
  std::string ToString(void) const;

  void FixEpgBugs(void);

  bool Serialise(TiXmlNode* node) const;
  static bool Deserialise(EventPtr& event, const TiXmlNode* eventNode);
  bool Deserialise(const TiXmlNode* node);

private:
  // XBMC data
  const unsigned int   m_eventID;
  uint32_t             m_atscSourceId;
  // TODO: ATSC Source ID
  std::string          m_strTitle;
  std::string          m_strPlotOutline; // sub title / short text (m_strShortText)
  std::string          m_strPlot; // description (m_strDescription)
  cChannelID           m_channelID;
  CDateTime            m_startTime;
  CDateTime            m_endTime;
  EPG_GENRE            m_genreType;
  EPG_SUB_GENRE        m_genreSubType;
  std::string          m_strCustomGenre; // Used only when m_genreType = EPG_GENRE_CUSTOM
  uint16_t             m_parentalRating;
  uint8_t              m_starRating;

  // VDR data
  uint8_t              m_tableID;
  uint8_t              m_version;
  CDateTime            m_vps;
  std::vector<CEpgComponent> m_components;
  std::vector<uint8_t> m_contents;

  /*
  // Ephemeral data
  CDateTime            m_seen;
  SI::RunningStatus    m_runningStatus;
  */
};

}
