#pragma once

#include "Types.h"
#include "Event.h"
#include "utils/DateTime.h"
#include <shared_ptr/shared_ptr.hpp>

namespace VDR
{
class cSchedules;

class cSchedule : public cListObject  {
private:
  tChannelID channelID;
  EventVector m_events;
  cHash<cEvent> eventsHashID; // TODO: hash table of shared pointers
  cHash<cEvent> eventsHashStartTime;
  bool hasRunning;
  CDateTime modified;
  CDateTime presentSeen;
  CDateTime saved;
public:
  cSchedule(tChannelID ChannelID);
  tChannelID ChannelID(void) const { return channelID; }
  CDateTime Modified(void) const { return modified; }
  CDateTime PresentSeen(void) const { return presentSeen; }
  bool PresentSeenWithin(int Seconds) const { return (CDateTime::GetCurrentDateTime() - presentSeen).GetSecondsTotal() < Seconds; }
  void SetModified(void) { modified = CDateTime::GetCurrentDateTime(); }
  void SetSaved(void) { saved = modified; }
  void SetPresentSeen(void) { presentSeen = CDateTime::GetCurrentDateTime(); }
  void SetRunningStatus(const EventPtr& event, int RunningStatus, cChannel *Channel = NULL);
  void ClrRunningStatus(cChannel *Channel = NULL);
  void ResetVersions(void);
  void Sort(void);
  void DropOutdated(const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uchar TableID, uchar Version);
  void Cleanup(const CDateTime& Time);
  void Cleanup(void);
  void AddEvent(const EventPtr& event);
  void DelEvent(const EventPtr& event);
  void HashEvent(cEvent *Event);
  void UnhashEvent(cEvent *Event);
  const EventVector& Events(void) const { return m_events; }
  EventPtr GetPresentEvent(void) const;
  EventPtr GetFollowingEvent(void) const;
  EventPtr GetEvent(tEventID EventID, CDateTime StartTime = CDateTime::GetCurrentDateTime());
  bool Read(void);
  bool Save(void);
  bool Serialise(TiXmlNode *node) const;
  };
}
