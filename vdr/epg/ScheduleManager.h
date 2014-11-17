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

#include "EPGTypes.h"
#include "Schedule.h"
#include "channels/ChannelID.h"
#include "lib/platform/threads/mutex.h"
#include "utils/DateTime.h"
#include "utils/Observer.h"

#include <map>
#include <vector>

namespace VDR
{

class CChannelFilter;
class cTransponder;

class cScheduleManager : protected Observer, public Observable
{
public:
  static cScheduleManager& Get(void);
  virtual ~cScheduleManager(void);

  /*
  void ResetVersions(void);
  bool ClearAll(void);
  void CleanTables(void);
  */

  void AddEvent(const EventPtr& event, const cTransponder& transponder);
  EventPtr GetEvent(const cChannelID& channelId, unsigned int eventId) const;
  EventVector GetEvents(const cChannelID& channelID) const;
  std::vector<cChannelID> GetUpdatedChannels(const std::map<int, CDateTime>& lastUpdated, CChannelFilter& filter) const;

  virtual void Notify(const Observable &obs, const ObservableMessage msg);
  void NotifyObservers(void);

  /*!
   * EPG data is saved to the EPG folder (special://home/epg by default).
   * cScheduleManager saves an index of channel IDs to epg.xml. Each channel ID
   * corresponds to an EPG schedule for that channel. Schedules are saved to
   * their own channel-specific file named epg_<CHANNEL_ID>.xml.
   */
  bool Load(void);

private:
  cScheduleManager(void) { }

  bool Save(void) const;

  std::map<cChannelID, SchedulePtr> m_schedules;
  PLATFORM::CMutex                  m_mutex;
};

}
