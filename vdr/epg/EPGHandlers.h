#pragma once

#include "Types.h"
#include "Event.h"
#include "utils/Tools.h"
#include "utils/DateTime.h"
#include <libsi/section.h>
#include <time.h>

namespace VDR
{
class cEpgHandler;
class cChannel;
class cEvent;

class cEpgHandlers : public cList<cEpgHandler> {
public:
  static cEpgHandlers& Get(void);
  bool IgnoreChannel(const cChannel& Channel);
  bool HandleEitEvent(SchedulePtr Schedule, const SI::EIT::Event *EitEvent, uchar TableID, uchar Version);
  bool HandledExternally(const cChannel *Channel);
  bool IsUpdate(tEventID EventID, const CDateTime& StartTime, uchar TableID, uchar Version);
  void SetEventID(cEvent *Event, tEventID EventID);
  void SetTitle(cEvent *Event, const std::string& Title);
  void SetShortText(cEvent *Event, const std::string& ShortText);
  void SetDescription(cEvent *Event, const std::string& Description);
  void SetContents(cEvent *Event, uchar *Contents);
  void SetParentalRating(cEvent *Event, int ParentalRating);
  void SetStartTime(cEvent *Event, const CDateTime& StartTime);
  void SetDuration(cEvent *Event, int Duration);
  void SetVps(cEvent *Event, const CDateTime& Vps);
  void SetComponents(cEvent *Event, CEpgComponents *Components);
  void FixEpgBugs(cEvent *Event);
  void HandleEvent(cEvent *Event);
  void SortSchedule(SchedulePtr Schedule);
  void DropOutdated(SchedulePtr Schedule, const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uchar TableID, uchar Version);
  };
}
