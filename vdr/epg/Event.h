#pragma once

#define MAXEPGBUGFIXLEVEL 3
#define VDR_RATINGS_PATCHED_V2

#include "Components.h"
#include "utils/Tools.h"
#include "utils/DateTime.h"

class TiXmlElement;

enum { MaxEventContents = 4 };

enum eEventContentGroup {
  ecgMovieDrama               = 0x10,
  ecgNewsCurrentAffairs       = 0x20,
  ecgShow                     = 0x30,
  ecgSports                   = 0x40,
  ecgChildrenYouth            = 0x50,
  ecgMusicBalletDance         = 0x60,
  ecgArtsCulture              = 0x70,
  ecgSocialPoliticalEconomics = 0x80,
  ecgEducationalScience       = 0x90,
  ecgLeisureHobbies           = 0xA0,
  ecgSpecial                  = 0xB0,
  ecgUserDefined              = 0xF0
  };

enum eDumpMode { dmAll, dmPresent, dmFollowing, dmAtTime };

class cSchedule;

typedef u_int32_t tEventID;

class cEvent : public cListObject {
  friend class cSchedule;
private:
  // The sequence of these parameters is optimized for minimal memory waste!
  cSchedule *schedule;     // The Schedule this event belongs to
  tEventID eventID;        // Event ID of this event
  uchar tableID;           // Table ID this event came from
  uchar version;           // Version number of section this event came from
  uchar runningStatus;     // 0=undefined, 1=not running, 2=starts in a few seconds, 3=pausing, 4=running
  uint16_t parentalRating; // Parental rating of this event
  uint8_t starRating;      // Dish/BEV star rating
  std::string m_strTitle;  // Title of this event
  std::string m_strShortText; // Short description of this event (typically the episode name in case of a series)
  std::string m_strDescription; // Description of this event
  CEpgComponents *components;       // The stream components of this event
  uchar contents[MaxEventContents]; // Contents of this event
  time_t startTime;        // Start time of this event
  int duration;            // Duration of this event in seconds
  time_t vps;              // Video Programming Service timestamp (VPS, aka "Programme Identification Label", PIL)
  time_t seen;             // When this event was last seen in the data stream
public:
  cEvent(tEventID EventID);
  ~cEvent();
  virtual int Compare(const cListObject &ListObject) const;
  tChannelID ChannelID(void) const;
  const cSchedule *Schedule(void) const { return schedule; }
  tEventID EventID(void) const { return eventID; }
  uchar TableID(void) const { return tableID; }
  uchar Version(void) const { return version; }
  int RunningStatus(void) const { return runningStatus; }
  std::string Title(void) const { return m_strTitle; }
  std::string ShortText(void) const { return m_strShortText; }
  std::string Description(void) const { return m_strDescription; }
  const CEpgComponents *Components(void) const { return components; }
  uchar Contents(int i = 0) const { return (0 <= i && i < MaxEventContents) ? contents[i] : uchar(0); }
  int ParentalRating(void) const { return parentalRating; }
  uint8_t StarRating(void) const { return starRating; }
  time_t StartTime(void) const { return startTime; }
  CDateTime StartTimeAsDateTime(void) const { return CDateTime(startTime).GetAsUTCDateTime(); }
  time_t EndTime(void) const { return startTime + duration; }
  CDateTime EndTimeAsDateTime(void) const { return CDateTime(startTime + duration).GetAsUTCDateTime(); }
  int Duration(void) const { return duration; }
  bool HasVps(void) const { return vps; }
  CDateTime Vps(void) const { return CDateTime(vps).GetAsUTCDateTime(); }
  time_t Seen(void) const { return seen; }
  bool SeenWithin(int Seconds) const { return time(NULL) - seen < Seconds; }
  bool HasTimer(void) const;
  bool IsRunning(bool OrAboutToStart = false) const;
  static const char *ContentToString(uchar Content);
  std::string GetParentalRatingString(void) const;
  std::string GetStarRatingString(void) const;
  std::string GetDateString(void) const;
  std::string GetTimeString(void) const;
  std::string GetEndTimeString(void) const;
  std::string GetVpsString(void) const;
  void SetEventID(tEventID EventID);
  void SetTableID(uchar TableID);
  void SetVersion(uchar Version);
  void SetRunningStatus(int RunningStatus, cChannel *Channel = NULL);
  void SetTitle(const std::string& strTitle);
  void SetShortText(const std::string& strShortText);
  void SetDescription(const std::string& strDescription);
  void SetComponents(CEpgComponents *Components); // Will take ownership of Components!
  void SetContents(uchar *Contents);
  void SetParentalRating(int ParentalRating);
  void SetStarRating(uint8_t StarRating) { starRating = StarRating; }
  void SetStartTime(time_t StartTime);
  void SetDuration(int Duration);
  void SetVps(time_t Vps);
  void SetSeen(void);
  std::string ToDescr(void) const;
  bool Parse(const std::string& data);
  void FixEpgBugs(void);
  static bool Deserialise(cSchedule* schedule, const TiXmlNode *eventNode);
  bool Serialise(TiXmlElement* element) const;
  };
