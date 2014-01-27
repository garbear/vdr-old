#include "EPGHandlers.h"
#include "EPGHandler.h"
#include "Schedule.h"

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

bool cEpgHandlers::HandleEitEvent(SchedulePtr Schedule, const SI::EIT::Event *EitEvent, uchar TableID, uchar Version)
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

bool cEpgHandlers::IsUpdate(tEventID EventID, time_t StartTime, uchar TableID, uchar Version)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
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

void cEpgHandlers::SetTitle(cEvent *Event, const char *Title)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetTitle(Event, Title))
         return;
      }
  Event->SetTitle(Title);
}

void cEpgHandlers::SetShortText(cEvent *Event, const char *ShortText)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetShortText(Event, ShortText))
         return;
      }
  Event->SetShortText(ShortText);
}

void cEpgHandlers::SetDescription(cEvent *Event, const char *Description)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetDescription(Event, Description))
         return;
      }
  Event->SetDescription(Description);
}

void cEpgHandlers::SetContents(cEvent *Event, uchar *Contents)
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

void cEpgHandlers::SetStartTime(cEvent *Event, time_t StartTime)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
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

void cEpgHandlers::SetVps(cEvent *Event, time_t Vps)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->SetVps(Event, Vps))
         return;
      }
  Event->SetVps(Vps);
}

void cEpgHandlers::SetComponents(cEvent *Event, cComponents *Components)
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

void cEpgHandlers::DropOutdated(SchedulePtr Schedule, time_t SegmentStart, time_t SegmentEnd, uchar TableID, uchar Version)
{
  for (cEpgHandler *eh = First(); eh; eh = Next(eh)) {
      if (eh->DropOutdated(Schedule, SegmentStart, SegmentEnd, TableID, Version))
         return;
      }
  Schedule->DropOutdated(SegmentStart, SegmentEnd, TableID, Version);
}
