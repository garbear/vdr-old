#pragma once

#include "Types.h"
#include "Event.h"
#include <shared_ptr/shared_ptr.hpp>

class cSchedules;

class cSchedule : public cListObject  {
private:
  tChannelID channelID;
  cList<cEvent> events;
  cHash<cEvent> eventsHashID;
  cHash<cEvent> eventsHashStartTime;
  bool hasRunning;
  time_t modified;
  time_t presentSeen;
  time_t saved;
public:
  cSchedule(tChannelID ChannelID);
  tChannelID ChannelID(void) const { return channelID; }
  time_t Modified(void) const { return modified; }
  time_t PresentSeen(void) const { return presentSeen; }
  bool PresentSeenWithin(int Seconds) const { return time(NULL) - presentSeen < Seconds; }
  void SetModified(void) { modified = time(NULL); }
  void SetSaved(void) { saved = modified; }
  void SetPresentSeen(void) { presentSeen = time(NULL); }
  void SetRunningStatus(cEvent *Event, int RunningStatus, cChannel *Channel = NULL);
  void ClrRunningStatus(cChannel *Channel = NULL);
  void ResetVersions(void);
  void Sort(void);
  void DropOutdated(time_t SegmentStart, time_t SegmentEnd, uchar TableID, uchar Version);
  void Cleanup(time_t Time);
  void Cleanup(void);
  cEvent *AddEvent(cEvent *Event);
  void DelEvent(cEvent *Event);
  void HashEvent(cEvent *Event);
  void UnhashEvent(cEvent *Event);
  const cList<cEvent> *Events(void) const { return &events; }
  const cEvent *GetPresentEvent(void) const;
  const cEvent *GetFollowingEvent(void) const;
  cEvent *GetEvent(tEventID EventID, time_t StartTime = 0);
  const cEvent *GetEventAround(time_t Time) const;
  bool Read(void);
  bool Save(void);
  bool Serialise(TiXmlNode *node) const;
  };
