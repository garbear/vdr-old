#pragma once

#include "EPGTypes.h"
#include "utils/DateTime.h"
#include "utils/List.h"
#include "utils/Tools.h" // for uchar

#include <libsi/section.h>

namespace VDR
{
class cChannel;
class CEpgComponents;

class cEpgHandler : public cListObject {
public:
  cEpgHandler(void);
          ///< Constructs a new EPG handler and adds it to the list of EPG handlers.
          ///< Whenever an event is received from the EIT data stream, the EPG
          ///< handlers are queried in the order they have been created.
          ///< As soon as one of the EPG handlers returns true in a member function,
          ///< none of the remaining handlers will be queried. If none of the EPG
          ///< handlers returns true in a particular call, the default processing
          ///< will take place.
          ///< EPG handlers will be deleted automatically at the end of the program.
  virtual ~cEpgHandler();
  virtual bool IgnoreChannel(const cChannel& Channel) { return false; }
          ///< Before any EIT data for the given Channel is processed, the EPG handlers
          ///< are asked whether this Channel shall be completely ignored. If any of
          ///< the EPG handlers returns true in this function, no EIT data at all will
          ///< be processed for this Channel.
  virtual bool HandleEitEvent(SchedulePtr Schedule, const SI::EIT::Event *EitEvent, uchar TableID, uchar Version) { return false; }
          ///< Before the raw EitEvent for the given Schedule is processed, the
          ///< EPG handlers are queried to see if any of them would like to do the
          ///< complete processing by itself. TableID and Version are from the
          ///< incoming section data.
  virtual bool HandledExternally(const cChannel *Channel) { return false; }
          ///< If any EPG handler returns true in this function, it is assumed that
          ///< the EPG for the given Channel is handled completely from some external
          ///< source. Incoming EIT data is processed as usual, but any new EPG event
          ///< will not be added to the respective schedule. It's up to the EPG
          ///< handler to take care of this.
  virtual bool IsUpdate(tEventID EventID, const CDateTime& StartTime, uchar TableID, uchar Version) { return false; }
          ///< VDR can't perform the update check (version, tid) for externally handled events,
          ///< therefore the EPG handlers have to take care of this. Otherwise the parsing of
          ///< non-updates will waste a lot of resources.
  virtual bool SetEventID(cEvent *Event, tEventID EventID) { return false; }
  virtual bool SetTitle(cEvent *Event, const std::string& strTitle) { return false; }
  virtual bool SetShortText(cEvent *Event, const std::string& strShortText) { return false; }
  virtual bool SetDescription(cEvent *Event, const std::string& strDescription) { return false; }
  virtual bool SetContents(cEvent *Event, uchar *Contents) { return false; }
  virtual bool SetParentalRating(cEvent *Event, int ParentalRating) { return false; }
  virtual bool SetStartTime(cEvent *Event, const CDateTime& StartTime) { return false; }
  virtual bool SetDuration(cEvent *Event, int Duration) { return false; }
  virtual bool SetVps(cEvent *Event, const CDateTime& Vps) { return false; }
  virtual bool SetComponents(cEvent *Event, CEpgComponents *Components) { return false; }
  virtual bool FixEpgBugs(cEvent *Event) { return false; }
          ///< Fixes some known problems with EPG data.
  virtual bool HandleEvent(cEvent *Event) { return false; }
          ///< After all modifications of the Event have been done, the EPG handler
          ///< can take a final look at it.
  virtual bool SortSchedule(SchedulePtr Schedule) { return false; }
          ///< Sorts the Schedule after the complete table has been processed.
  virtual bool DropOutdated(SchedulePtr Schedule, const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uchar TableID, uchar Version) { return false; }
          ///< Takes a look at all EPG events between SegmentStart and SegmentEnd and
          ///< drops outdated events.
  };
}
