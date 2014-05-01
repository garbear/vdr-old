#pragma once

#include "EPGTypes.h"
#include "channels/ChannelID.h"
#include "utils/DateTime.h"
#include "utils/Hash.h"
#include "utils/List.h"

#include <stdint.h>

class TiXmlNode;

namespace VDR
{
class cChannel;

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
  void DropOutdated(const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uint8_t TableID, uint8_t Version);
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
