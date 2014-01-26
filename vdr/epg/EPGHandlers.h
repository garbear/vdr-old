#pragma once

#include "Event.h"
#include "utils/Tools.h"
#include <libsi/section.h>
#include <time.h>

class cEpgHandler;
class cChannel;
class cEvent;
class cSchedule;

class cEpgHandlers : public cList<cEpgHandler> {
public:
  static cEpgHandlers& Get(void);
  bool IgnoreChannel(const cChannel& Channel);
  bool HandleEitEvent(cSchedule *Schedule, const SI::EIT::Event *EitEvent, uchar TableID, uchar Version);
  bool HandledExternally(const cChannel *Channel);
  bool IsUpdate(tEventID EventID, time_t StartTime, uchar TableID, uchar Version);
  void SetEventID(cEvent *Event, tEventID EventID);
  void SetTitle(cEvent *Event, const char *Title);
  void SetShortText(cEvent *Event, const char *ShortText);
  void SetDescription(cEvent *Event, const char *Description);
  void SetContents(cEvent *Event, uchar *Contents);
  void SetParentalRating(cEvent *Event, int ParentalRating);
  void SetStartTime(cEvent *Event, time_t StartTime);
  void SetDuration(cEvent *Event, int Duration);
  void SetVps(cEvent *Event, time_t Vps);
  void SetComponents(cEvent *Event, cComponents *Components);
  void FixEpgBugs(cEvent *Event);
  void HandleEvent(cEvent *Event);
  void SortSchedule(cSchedule *Schedule);
  void DropOutdated(cSchedule *Schedule, time_t SegmentStart, time_t SegmentEnd, uchar TableID, uchar Version);
  };
