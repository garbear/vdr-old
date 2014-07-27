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

class cScanList;
class CChannelFilter;

class cChannelManager : public Observer, public Observable
{
  friend class CChannelFilter;

public:
  cChannelManager(void);
  ~cChannelManager(void) { }

  static cChannelManager &Get(void);

  void AddChannel(const ChannelPtr& channel);
  void AddChannels(const ChannelVector& channels);

  /*!
   * Merge discovered channel data into matching channel (per TSID and SID).
   * Merges channel params:
   *   - NID, TSID and SID
   *   - Stream details
   *   - CA Descriptors
   */
  void MergeChannelProps(const ChannelPtr& channel);

  /*!
   * Merge discovered channel names and transponder modulation mode into
   * matching channel (per TSID and SID). Merges channel params:
   *   - Name, short name and provider
   *   - Transponder modulation mode
   * TODO: Separate names and modulation
   */
  void MergeChannelNamesAndModulation(const ChannelPtr& channel);

  ChannelPtr GetByChannelID(const cChannelID& channelID) const;
  ChannelPtr GetByChannelUID(uint32_t channelUid) const;
  ChannelVector GetCurrent(void) const;
  size_t ChannelCount() const;

  void RemoveChannel(const ChannelPtr& channel);
  void Clear(void);

  void Notify(const Observable &obs, const ObservableMessage msg);
  void NotifyObservers(void);

  bool Load(void);
  bool Load(const std::string &file);
  bool Save(const std::string &file = "");

  void CreateChannelGroups(bool automatic);

private:
  ChannelVector    m_channels;
  std::string      m_strFilename;
  PLATFORM::CMutex m_mutex;
};
}
