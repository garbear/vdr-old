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

#include "EPGHandlers.h"
#include "EPGHandler.h"
#include "Event.h"
#include "Schedule.h"

namespace VDR
{

cEpgHandlers& cEpgHandlers::Get(void)
{
  static cEpgHandlers _instance;
  return _instance;
}

bool cEpgHandlers::IgnoreChannel(const cChannel& Channel)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh))
  {
    if (eh->IgnoreChannel(Channel))
      return true;
  }
  return false;
}

bool cEpgHandlers::HandleEitEvent(SchedulePtr Schedule, const SI::EIT::Event *EitEvent, uint8_t TableID, uint8_t Version)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->HandleEitEvent(Schedule, EitEvent, TableID, Version))
         return true;
      }
  return false;
}

bool cEpgHandlers::HandledExternally(const cChannel *Channel)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->HandledExternally(Channel))
         return true;
      }
  return false;
}

bool cEpgHandlers::IsUpdate(tEventID EventID, const CDateTime& StartTime, uint8_t TableID, uint8_t Version)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh))
  {
    if (eh->IsUpdate(EventID, StartTime, TableID, Version))
      return true;
  }
  return false;
}

void cEpgHandlers::SetEventID(cEvent *Event, tEventID EventID)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetEventID(Event, EventID))
         return;
      }
  Event->SetEventID(EventID);
}

void cEpgHandlers::SetTitle(cEvent *Event, const std::string& strTitle)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh))
  {
    if (eh->SetTitle(Event, strTitle))
      return;
  }
  Event->SetTitle(strTitle);
}

void cEpgHandlers::SetShortText(cEvent *Event, const std::string& strShortText)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh))
  {
      return;
  }
  Event->SetShortText(strShortText);
}

void cEpgHandlers::SetDescription(cEvent *Event, const std::string& strDescription)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh))
  {
    if (eh->SetDescription(Event, strDescription))
      return;
  }
  Event->SetDescription(strDescription);
}

void cEpgHandlers::SetContents(cEvent *Event, uint8_t *Contents)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetContents(Event, Contents))
         return;
      }
  Event->SetContents(Contents);
}

void cEpgHandlers::SetParentalRating(cEvent *Event, int ParentalRating)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetParentalRating(Event, ParentalRating))
         return;
      }
  Event->SetParentalRating(ParentalRating);
}

void cEpgHandlers::SetStartTime(cEvent *Event, const CDateTime& StartTime)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh))
  {
    if (eh->SetStartTime(Event, StartTime))
      return;
  }
  Event->SetStartTime(StartTime);
}

void cEpgHandlers::SetDuration(cEvent *Event, int Duration)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetDuration(Event, Duration))
         return;
      }
  Event->SetDuration(Duration);
}

void cEpgHandlers::SetVps(cEvent *Event, const CDateTime& Vps)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh))
  {
    if (eh->SetVps(Event, Vps))
      return;
  }
  Event->SetVps(Vps);
}

void cEpgHandlers::SetComponents(cEvent *Event, CEpgComponents *Components)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetComponents(Event, Components))
         return;
      }
  Event->SetComponents(Components);
}

void cEpgHandlers::FixEpgBugs(cEvent *Event)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->FixEpgBugs(Event))
         return;
      }
  Event->FixEpgBugs();
}

void cEpgHandlers::HandleEvent(cEvent *Event)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->HandleEvent(Event))
         break;
      }
}

void cEpgHandlers::SortSchedule(SchedulePtr Schedule)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SortSchedule(Schedule))
         return;
      }
  Schedule->Sort();
}

void cEpgHandlers::DropOutdated(SchedulePtr Schedule, const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uint8_t TableID, uint8_t Version)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh))
  {
    if (eh->DropOutdated(Schedule, SegmentStart, SegmentEnd, TableID, Version))
      return;
  }

  Schedule->DropOutdated(SegmentStart, SegmentEnd, TableID, Version);
}

}
