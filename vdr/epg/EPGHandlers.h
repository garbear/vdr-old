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
#include "utils/DateTime.h"
#include "utils/List.h"

#include <libsi/section.h>
#include <stdint.h>
#include <string>

namespace VDR
{
class CEpgComponents;
class cEpgHandler;
class cChannel;
class cEvent;

class cEpgHandlers : public cList<cEpgHandler> {
public:
  static cEpgHandlers& Get(void);
  bool IgnoreChannel(const cChannel& Channel);
  bool HandleEitEvent(SchedulePtr Schedule, const SI::EIT::Event *EitEvent, uint8_t TableID, uint8_t Version);
  bool HandledExternally(const cChannel *Channel);
  bool IsUpdate(tEventID EventID, const CDateTime& StartTime, uint8_t TableID, uint8_t Version);
  void SetEventID(cEvent *Event, tEventID EventID);
  void SetTitle(cEvent *Event, const std::string& Title);
  void SetShortText(cEvent *Event, const std::string& ShortText);
  void SetDescription(cEvent *Event, const std::string& Description);
  void SetContents(cEvent *Event, uint8_t *Contents);
  void SetParentalRating(cEvent *Event, int ParentalRating);
  void SetStartTime(cEvent *Event, const CDateTime& StartTime);
  void SetDuration(cEvent *Event, int Duration);
  void SetVps(cEvent *Event, const CDateTime& Vps);
  void SetComponents(cEvent *Event, CEpgComponents *Components);
  void FixEpgBugs(cEvent *Event);
  void HandleEvent(cEvent *Event);
  void SortSchedule(SchedulePtr Schedule);
  void DropOutdated(SchedulePtr Schedule, const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uint8_t TableID, uint8_t Version);
  };
}
