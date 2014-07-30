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

#include "Channel.h"
#include "ChannelTypes.h"
#include "utils/Observer.h"
#include "platform/threads/mutex.h"
#include "transponders/TransponderTypes.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace VDR
{
enum eLastModifiedType
{
  CHANNELSMOD_NONE = 0, // Assigned values are for compatibility
  CHANNELSMOD_AUTO = 1,
  CHANNELSMOD_USER = 2,
};

//std::string ChannelString(const ChannelPtr &channel, int number);

class cScanList;
class CChannelFilter;

class cChannelManager : public Observer, public Observable
{
  friend class CChannelFilter;

public:
  cChannelManager();
  ~cChannelManager() { }

  static cChannelManager &Get();

  void Clear(void);

  void Notify(const Observable &obs, const ObservableMessage msg);
  void AddChannel(ChannelPtr channel);
  void AddChannels(const ChannelVector& channels);
  void RemoveChannel(ChannelPtr channel);

  bool Load(void);
  bool Load(const std::string &file);
  bool Save(const std::string &file = "");

  /*!
   * \brief Find a channel by its number
   * \param number The channel number
   * \param skipGap Positive for channel following gap, negative for channel
   *        Default (0) will return an empty pointer if the channel is a separator
   * \return The channel, or empty pointer if the number is missing or a group separator
   */
  ChannelPtr GetByNumber(int number, int skipGap = 0);

  /*!
   * \brief Find a channel by its service ID, source and transponder
   * \param serviceID The matching service ID
   * \param source The matching source
   * \param transponder A compatible transponder (see ISTRANSPONDER())
   * \return The channel, or empty pointer if the channel isn't found
   */
  ChannelPtr GetByServiceID(int serviceID, TRANSPONDER_TYPE source, int transponder) const;
  static ChannelPtr GetByServiceID(const ChannelVector& channels, int serviceID, TRANSPONDER_TYPE source, int transponder); // TODO: Remove me

  /*!
   * \brief Find a channel by its channel ID
   * \param channelID The channel ID tag
   * \param bTryWithoutRid If the search fails, repeat without comparing the RID
   * \return The channel, or empty pointer if the channel isn't found
   */
  ChannelPtr GetByChannelID(const cChannelID& channelID);

  /*!
   * \brief Find a channel by its channel ID
   * \param nid TODO
   * \param tid TODO
   * \param sid TODO
   * \return The channel, or empty pointer if the channel isn't found
   */
  ChannelPtr GetByChannelID(uint16_t nid, uint16_t tsid, uint16_t sid);

  ChannelPtr GetByChannelUID(uint32_t channelUID) const;

  /*!
   * \brief Find a channel by the source, NID and TID of a transponder ID
   * \param channelID The channel ID tag (TODO: Is this the transponder ID?)
   * \return The channel, or empty pointer if the channel isn't found
   */
  ChannelPtr GetByTransponderID(const cChannelID& channelID);

  /*!
   * \brief Returns false if another channel has the same cChannelID tag
   * \param newChannel The channel whose channel ID will be used in the search
   * \param oldChannel Skip comparing channel IDs with this channel
   * \return true if no other channels (excluding oldChannel) have the same channel ID
   */
  bool HasUniqueChannelID(const ChannelPtr &newChannel, const ChannelPtr &oldChannel = cChannel::EmptyChannel) const;

  /*!
   * \brief Return the number of the highest channel (I think - TODO)
   */
  unsigned int MaxNumber() const { return m_maxNumber; }

  void SetModified(void);

  ChannelVector GetCurrent(void) const;
  void CreateChannelGroups(bool automatic);

  unsigned int ChannelCount() const;

private:
  typedef std::map<int, ChannelVector> ChannelSidMap;
  ::PLATFORM::CMutex m_mutex;

  ChannelVector m_channels;
  ChannelSidMap m_channelSids; // Index channels by SID for fast lookups in GetBy*() methods

  unsigned int  m_maxNumber;
  unsigned int  m_maxChannelNameLength;
  unsigned int  m_maxShortChannelNameLength;

  std::string   m_strFilename;
};
}
