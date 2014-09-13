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
#include "channels/ChannelID.h"
#include "utils/DateTime.h"
#include "utils/Observer.h"

#include <map>
#include <stdint.h>

class TiXmlNode;

namespace VDR
{

class cSchedule : protected Observer, public Observable
{
public:
  cSchedule(const cChannelID& m_channelID);
  virtual ~cSchedule(void);

  static const SchedulePtr EmptySchedule;

  const cChannelID& ChannelID(void) const { return m_channelID; }

  EventVector Events(void) const;
  EventPtr GetEvent(unsigned int eventID) const;
  EventPtr GetEvent(const CDateTime& startTime) const;
  void AddEvent(const EventPtr& event);
  void DeleteEvent(unsigned int eventID);

  /*
  void SetRunningStatus(const EventPtr& event, int RunningStatus, cChannel *Channel = NULL);
  void ClrRunningStatus(cChannel *Channel = NULL);
  void ResetVersions(void);
  void Sort(void);
  void DropOutdated(const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uint8_t TableID, uint8_t Version);
  void Cleanup(const CDateTime& Time);
  void Cleanup(void);
  */

  virtual void Notify(const Observable &obs, const ObservableMessage msg);
  void NotifyObservers(void);

  bool Load(void);
  bool Serialise(TiXmlNode* node) const;

private:
  bool Save(void) const;

  const cChannelID                 m_channelID;
  std::map<unsigned int, EventPtr> m_eventIds;    // ID -> Event

  //bool             m_bHasRunning;

  //CDateTime modified;
  //CDateTime presentSeen;
  //CDateTime saved;
};

}
