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

#include "Channel.h"
#include "utils/Observer.h"
#include "platform/threads/mutex.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

enum eLastModifiedType
{
  CHANNELSMOD_NONE = 0, // Assigned values are for compatibility
  CHANNELSMOD_AUTO = 1,
  CHANNELSMOD_USER = 2,
};

//std::string ChannelString(const ChannelPtr &channel, int number);

class cScanList;

class cChannelManager : public Observer
{
public:
  cChannelManager();
  ~cChannelManager() { }

  static cChannelManager &Get();

  void Clear(void);

  void Notify(const Observable &obs, const ObservableMessage msg);
  void AddChannel(ChannelPtr channel);
  void RemoveChannel(ChannelPtr channel);

  bool Load(const std::string &file);
  bool LoadConf(const std::string &file);
  bool Save(const std::string &file);
  bool SaveConf(const std::string &file);

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
  ChannelPtr GetByServiceID(unsigned short serviceID, int source, int transponder);

  /*!
   * \brief Find a channel by its channel ID
   * \param channelID The channel ID tag
   * \param bTryWithoutRid If the search fails, repeat without comparing the RID
   * \param bTryWithoutPolarization If the search fails, repeat after removing
   *        polarization from the TID
   * \return The channel, or empty pointer if the channel isn't found
   */
  ChannelPtr GetByChannelID(const tChannelID &channelID, bool bTryWithoutRid = false, bool bTryWithoutPolarization = false);

  /*!
   * \brief Find a channel by its channel ID
   * \param nid TODO
   * \param tid TODO
   * \param sid TODO
   * \return The channel, or empty pointer if the channel isn't found
   */
  ChannelPtr GetByChannelID(int nid, int tid, int sid);

  /*!
   * \brief Find a channel by the source, NID and TID of a transponder ID
   * \param channelID The channel ID tag (TODO: Is this the transponder ID?)
   * \return The channel, or empty pointer if the channel isn't found
   */
  ChannelPtr GetByTransponderID(tChannelID channelID);

  /*!
   * \brief Get next channel group (TODO)
   */
  int GetNextGroup(unsigned int index) const;

  /*!
   * \brief Get previous channel group (TODO)
   */
  int GetPrevGroup(unsigned int index) const;

  /*!
   * \brief Get next normal channel (not group) (TODO)
   */
  int GetNextNormal(unsigned int index) const;

  /*!
   * \brief Get previous normal channel (not group) (TODO)
   */
  int GetPrevNormal(unsigned int index) const;

  /*!
   * \brief The documentation claims: "Recalculate 'number' based on channel type" (TODO)
   */
  void ReNumber();

  /*!
   * \brief Returns false if another channel has the same tChannelID tag
   * \param newChannel The channel whose channel ID will be used in the search
   * \param oldChannel Skip comparing channel IDs with this channel
   * \return true if no other channels (excluding oldChannel) have the same channel ID
   */
  bool HasUniqueChannelID(const ChannelPtr &newChannel, const ChannelPtr &oldChannel = ChannelPtr()) const;

  /*!
   * \brief ??? (TODO)
   */
  bool SwitchTo(int number);

  /*!
   * \brief Return the number of the highest channel (I think - TODO)
   */
  unsigned int MaxNumber() const { return m_maxNumber; }

  /*!
   * \brief Calculates and returns the maximum length of all channel names (excluding group separators)
   */
  unsigned int MaxChannelNameLength();

  /*!
   * \brief Calculates and returns the maximum length of all channel short names (excluding group separators)
   */
  unsigned int MaxShortChannelNameLength();

  /*!
   * \brief Returns 0 if no channels have been modified, 1 if an automatic
   *        modification has been made, and 2 if the user has made a modification.
   *        Calling this function resets the 'modified' flag to 0.
   */
  eLastModifiedType Modified();
  void SetModified(bool ByUser = false);

  /*!
   * \brief It looks like this creates a new channel, adds it to the vector, and re-computes m_channelSids (TODO)
   */
  ChannelPtr NewChannel(const cChannel& transponder, const std::string& name, const std::string& shortName, const std::string& provider, int nid, int tid, int sid, int rid = 0);

  // wtf...
  int BeingEdited() const { return m_beingEdited; }
  void IncBeingEdited() { m_beingEdited++; }
  void DecBeingEdited() { m_beingEdited--; }

  void AddTransponders(cScanList* list) const;

  // XXX refactor NIT
  std::vector<ChannelPtr>::const_iterator Iterator(void) const { return m_channels.begin(); }
  bool HasNext(const std::vector<ChannelPtr>::const_iterator& it) const { return it != m_channels.end(); }
  void Next(std::vector<ChannelPtr>::const_iterator& it) const { ++it; }

private:
  typedef std::vector<ChannelPtr>      ChannelVector;
  typedef std::map<int, ChannelVector> ChannelSidMap;

  ChannelVector m_channels;
  ChannelSidMap m_channelSids; // Index channels by SID for fast lookups in GetBy*() methods

  unsigned int  m_maxNumber;
  unsigned int  m_maxChannelNameLength;
  unsigned int  m_maxShortChannelNameLength;

  eLastModifiedType m_modified;

  int           m_beingEdited;
};