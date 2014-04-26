#include "Event.h"
#include "EPGDefinitions.h"
#include "Schedule.h"
#include "channels/ChannelManager.h"
#include "timers/Timers.h"
#include "utils/UTF8Utils.h"
#include "utils/I18N.h"
#include "utils/CalendarUtils.h"
#include "utils/XBMCTinyXML.h"
#include "settings/Settings.h"

namespace VDR
{

cEvent::cEvent(tEventID EventID)
{
  schedule = NULL;
  eventID = EventID;
  tableID = 0xFF; // actual table ids are 0x4E..0x60
  version = 0xFF; // actual version numbers are 0..31
  runningStatus = SI::RunningStatusUndefined;
  components = NULL;
  memset(contents, 0, sizeof(contents));
  parentalRating = 0;
  starRating = 0;
  duration = 0;
  SetSeen();
}

cEvent::~cEvent()
{
  delete components;
}

int cEvent::Compare(const cListObject &ListObject) const
{
  cEvent *e = (cEvent *)&ListObject;
  return (m_startTime - e->m_startTime).GetSecondsTotal();
}

tChannelID cEvent::ChannelID(void) const
{
  return schedule ? schedule->ChannelID() : tChannelID();
}

void cEvent::SetEventID(tEventID EventID)
{
  if (eventID != EventID)
  {
    if (schedule)
      schedule->UnhashEvent(this);
    eventID = EventID;
    if (schedule)
      schedule->HashEvent(this);
  }
}

void cEvent::SetTableID(uchar TableID)
{
  tableID = TableID;
}

void cEvent::SetVersion(uchar Version)
{
  version = Version;
}

void cEvent::SetRunningStatus(int RunningStatus, cChannel *Channel)
{
  if (Channel && runningStatus != RunningStatus && (RunningStatus > SI::RunningStatusNotRunning || runningStatus > SI::RunningStatusUndefined) && Channel->HasTimer())
    isyslog("channel %d (%s) event %s status %d", Channel->Number(), Channel->Name().c_str(), ToDescr().c_str(), RunningStatus);
  runningStatus = RunningStatus;
}

void cEvent::SetTitle(const std::string& strTitle)
{
  m_strTitle = strTitle;
}

void cEvent::SetShortText(const std::string& strShortText)
{
  m_strShortText = strShortText;
}

void cEvent::SetDescription(const std::string& strDescription)
{
  m_strDescription = strDescription;
}

void cEvent::SetComponents(CEpgComponents *Components)
{
  delete components;
  components = Components;
}

void cEvent::SetContents(uchar *Contents)
{
  for (int i = 0; i < MaxEventContents; i++)
      contents[i] = Contents[i];
}

void cEvent::SetParentalRating(int ParentalRating)
{
  parentalRating = ParentalRating;
}

void cEvent::SetStartTime(const CDateTime& StartTime)
{
  if (m_startTime != StartTime)
  {
    if (schedule)
      schedule->UnhashEvent(this);
    m_startTime = StartTime;
    if (schedule)
      schedule->HashEvent(this);
  }
}

void cEvent::SetDuration(int Duration)
{
  duration = Duration;
}

void cEvent::SetVps(const CDateTime& Vps)
{
  m_vps = Vps;
}

void cEvent::SetSeen(void)
{
  seen = CDateTime::GetUTCDateTime();
}

std::string cEvent::ToDescr(void) const
{
  char vpsbuf[64] = "";
  if (HasVps())
    sprintf(vpsbuf, "(VPS: %s) ", GetVpsString().c_str());
  return StringUtils::Format("%s - %s %s '%s'", StartTime().GetAsDBDateTime().c_str(), EndTime().GetAsDBDateTime().c_str(), vpsbuf, Title().c_str());
}

bool cEvent::HasTimer(void) const
{
  return cTimers::Get().HasTimer(this);
}

bool cEvent::IsRunning(bool OrAboutToStart) const
{
  return runningStatus >= (OrAboutToStart ? SI::RunningStatusStartsInAFewSeconds : SI::RunningStatusPausing);
}

const char *cEvent::ContentToString(uchar Content)
{
  switch (Content & 0xF0)
    {
  case ecgMovieDrama:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Movie/Drama");
    case 0x01:
      return tr("Content$Detective/Thriller");
    case 0x02:
      return tr("Content$Adventure/Western/War");
    case 0x03:
      return tr("Content$Science Fiction/Fantasy/Horror");
    case 0x04:
      return tr("Content$Comedy");
    case 0x05:
      return tr("Content$Soap/Melodrama/Folkloric");
    case 0x06:
      return tr("Content$Romance");
    case 0x07:
      return tr("Content$Serious/Classical/Religious/Historical Movie/Drama");
    case 0x08:
      return tr("Content$Adult Movie/Drama");
      }
    break;
  case ecgNewsCurrentAffairs:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$News/Current Affairs");
    case 0x01:
      return tr("Content$News/Weather Report");
    case 0x02:
      return tr("Content$News Magazine");
    case 0x03:
      return tr("Content$Documentary");
    case 0x04:
      return tr("Content$Discussion/Inverview/Debate");
      }
    break;
  case ecgShow:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Show/Game Show");
    case 0x01:
      return tr("Content$Game Show/Quiz/Contest");
    case 0x02:
      return tr("Content$Variety Show");
    case 0x03:
      return tr("Content$Talk Show");
      }
    break;
  case ecgSports:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Sports");
    case 0x01:
      return tr("Content$Special Event");
    case 0x02:
      return tr("Content$Sport Magazine");
    case 0x03:
      return tr("Content$Football/Soccer");
    case 0x04:
      return tr("Content$Tennis/Squash");
    case 0x05:
      return tr("Content$Team Sports");
    case 0x06:
      return tr("Content$Athletics");
    case 0x07:
      return tr("Content$Motor Sport");
    case 0x08:
      return tr("Content$Water Sport");
    case 0x09:
      return tr("Content$Winter Sports");
    case 0x0A:
      return tr("Content$Equestrian");
    case 0x0B:
      return tr("Content$Martial Sports");
      }
    break;
  case ecgChildrenYouth:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Children's/Youth Programme");
    case 0x01:
      return tr("Content$Pre-school Children's Programme");
    case 0x02:
      return tr("Content$Entertainment Programme for 6 to 14");
    case 0x03:
      return tr("Content$Entertainment Programme for 10 to 16");
    case 0x04:
      return tr("Content$Informational/Educational/School Programme");
    case 0x05:
      return tr("Content$Cartoons/Puppets");
      }
    break;
  case ecgMusicBalletDance:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Music/Ballet/Dance");
    case 0x01:
      return tr("Content$Rock/Pop");
    case 0x02:
      return tr("Content$Serious/Classical Music");
    case 0x03:
      return tr("Content$Folk/Tradional Music");
    case 0x04:
      return tr("Content$Jazz");
    case 0x05:
      return tr("Content$Musical/Opera");
    case 0x06:
      return tr("Content$Ballet");
      }
    break;
  case ecgArtsCulture:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Arts/Culture");
    case 0x01:
      return tr("Content$Performing Arts");
    case 0x02:
      return tr("Content$Fine Arts");
    case 0x03:
      return tr("Content$Religion");
    case 0x04:
      return tr("Content$Popular Culture/Traditional Arts");
    case 0x05:
      return tr("Content$Literature");
    case 0x06:
      return tr("Content$Film/Cinema");
    case 0x07:
      return tr("Content$Experimental Film/Video");
    case 0x08:
      return tr("Content$Broadcasting/Press");
    case 0x09:
      return tr("Content$New Media");
    case 0x0A:
      return tr("Content$Arts/Culture Magazine");
    case 0x0B:
      return tr("Content$Fashion");
      }
    break;
  case ecgSocialPoliticalEconomics:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Social/Political/Economics");
    case 0x01:
      return tr("Content$Magazine/Report/Documentary");
    case 0x02:
      return tr("Content$Economics/Social Advisory");
    case 0x03:
      return tr("Content$Remarkable People");
      }
    break;
  case ecgEducationalScience:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Education/Science/Factual");
    case 0x01:
      return tr("Content$Nature/Animals/Environment");
    case 0x02:
      return tr("Content$Technology/Natural Sciences");
    case 0x03:
      return tr("Content$Medicine/Physiology/Psychology");
    case 0x04:
      return tr("Content$Foreign Countries/Expeditions");
    case 0x05:
      return tr("Content$Social/Spiritual Sciences");
    case 0x06:
      return tr("Content$Further Education");
    case 0x07:
      return tr("Content$Languages");
      }
    break;
  case ecgLeisureHobbies:
    switch (Content & 0x0F)
      {
    default:
    case 0x00:
      return tr("Content$Leisure/Hobbies");
    case 0x01:
      return tr("Content$Tourism/Travel");
    case 0x02:
      return tr("Content$Handicraft");
    case 0x03:
      return tr("Content$Motoring");
    case 0x04:
      return tr("Content$Fitness & Health");
    case 0x05:
      return tr("Content$Cooking");
    case 0x06:
      return tr("Content$Advertisement/Shopping");
    case 0x07:
      return tr("Content$Gardening");
      }
    break;
  case ecgSpecial:
    switch (Content & 0x0F)
      {
    case 0x00:
      return tr("Content$Original Language");
    case 0x01:
      return tr("Content$Black & White");
    case 0x02:
      return tr("Content$Unpublished");
    case 0x03:
      return tr("Content$Live Broadcast");
    default:
      ;
      }
    break;
  case ecgUserDefined:
    switch (Content & 0x0F)
      {
    //case 0x00: return tr("Content$"); // ???
    case 0x01:
      return tr("Content$Movie");
    case 0x02:
      return tr("Content$Sports");
    case 0x03:
      return tr("Content$News");
    case 0x04:
      return tr("Content$Children");
    case 0x05:
      return tr("Content$Education");
    case 0x06:
      return tr("Content$Series");
    case 0x07:
      return tr("Content$Music");
    case 0x08:
      return tr("Content$Religious");
    default:
      ;
      }
    break;
  default:
    ;
    }
  return "";
}

std::string cEvent::GetParentalRatingString(void) const
{
  static const char * const ratings[8] = { "", "G", "PG", "PG-13", "R", "NR/AO", "", "NC-17" };
  char buffer[19];
  buffer[0] = 0;
  strcpy(buffer, ratings[(parentalRating >> 10) & 0x07]);
  if (parentalRating & 0x37F)
  {
    strcat(buffer, " [");
    if (parentalRating & 0x0230)
      strcat(buffer, "V,");
    if (parentalRating & 0x000A)
      strcat(buffer, "L,");
    if (parentalRating & 0x0044)
      strcat(buffer, "N,");
    if (parentalRating & 0x0101)
      strcat(buffer, "SC,");
    if (char *s = strrchr(buffer, ','))
      s[0] = ']';
  }

  return isempty(buffer) ? "" : buffer;
}

std::string cEvent::GetStarRatingString(void) const
{
  static const char *const critiques[8] = { "", "*", "*+", "**", "**+", "***", "***+", "****" };
  return critiques[starRating & 0x07];
}

std::string cEvent::GetVpsString(void) const
{
  return m_vps.GetAsSaveString();
}

bool cEvent::Parse(const std::string& data)
{
  char *t = skipspace(data.c_str() + 1);
  switch (*data.c_str())
    {
  case 'T':
    SetTitle(t);
    break;
  case 'S':
    SetShortText(t);
    break;
  case 'D':
    strreplace(t, '|', '\n');
    SetDescription(t);
    break;
  case 'G':
    {
      memset(contents, 0, sizeof(contents));
      for (int i = 0; i < MaxEventContents; i++)
      {
        char *tail = NULL;
        int c = strtol(t, &tail, 16);
        if (0x00 < c && c <= 0xFF)
        {
          contents[i] = c;
          t = tail;
        }
        else
          break;
      }
    }
    break;
  case 'R':
    {
      int ParentalRating = 0;
      int StarRating = 0;
      sscanf(t, "%d %d", &ParentalRating, &StarRating);
      SetParentalRating(ParentalRating);
      SetStarRating(StarRating);
    }
    break;
  case 'X':
    if (!components)
      components = new CEpgComponents;
    components->SetComponent(COMPONENT_ADD_NEW, t);
    break;
  case 'V':
    SetVps(atoi(t));
    break;
  default:
    esyslog("ERROR: unexpected tag while reading EPG data: %s", data.c_str());
    return false;
    }
  return true;
}

#define MAXEPGBUGFIXSTATS 13
#define MAXEPGBUGFIXCHANS 100
struct tEpgBugFixStats
{
  int hits;
  int n;
  tChannelID channelIDs[MAXEPGBUGFIXCHANS];
  tEpgBugFixStats(void)
  {
    hits = n = 0;
  }
};

tEpgBugFixStats EpgBugFixStats[MAXEPGBUGFIXSTATS];

static void EpgBugFixStat(int Number, tChannelID ChannelID)
{
  if (0 <= Number && Number < MAXEPGBUGFIXSTATS)
  {
    tEpgBugFixStats *p = &EpgBugFixStats[Number];
    p->hits++;
    int i = 0;
    for (; i < p->n; i++)
    {
      if (p->channelIDs[i] == ChannelID)
        break;
    }
    if (i == p->n && p->n < MAXEPGBUGFIXCHANS)
      p->channelIDs[p->n++] = ChannelID;
  }
}

#if 0
void ReportEpgBugFixStats(bool Force)
{
  if (g_setup.EPGBugfixLevel > 0)
  {
    static time_t LastReport = 0;
    time_t now = time(NULL);
    if (now - LastReport > 3600 || Force)
    {
      LastReport = now;
      struct tm tm_r;
      struct tm *ptm = localtime_r(&now, &tm_r);
      if (ptm->tm_hour != 5)
        return;
    }
    else
      return;
    bool GotHits = false;
    char buffer[1024];
    for (int i = 0; i < MAXEPGBUGFIXSTATS; i++)
    {
      const char *delim = " ";
      tEpgBugFixStats *p = &EpgBugFixStats[i];
      if (p->hits)
      {
        bool PrintedStats = false;
        char *q = buffer;
        *buffer = 0;
        for (int c = 0; c < p->n; c++)
        {
          ChannelPtr channel = cChannelManager::Get().GetByChannelID(
              p->channelIDs[c], true);
          if (channel)
          {
            if (!GotHits)
            {
              dsyslog("=====================");
              dsyslog("EPG bugfix statistics");
              dsyslog("=====================");
              dsyslog(
                  "IF SOMEBODY WHO IS IN CHARGE OF THE EPG DATA FOR ONE OF THE LISTED");
              dsyslog(
                  "CHANNELS READS THIS: PLEASE TAKE A LOOK AT THE FUNCTION cEvent::FixEpgBugs()");
              dsyslog(
                  "IN VDR/epg.c TO LEARN WHAT'S WRONG WITH YOUR DATA, AND FIX IT!");
              dsyslog("=====================");
              dsyslog("Fix Hits Channels");
              GotHits = true;
            }
            if (!PrintedStats)
            {
              q += snprintf(q, sizeof(buffer) - (q - buffer), "%-3d %-4d", i,
                  p->hits);
              PrintedStats = true;
            }
            q += snprintf(q, sizeof(buffer) - (q - buffer), "%s%s", delim,
                channel->Name().c_str());
            delim = ", ";
            if (q - buffer > 80)
            {
              q += snprintf(q, sizeof(buffer) - (q - buffer), "%s...", delim);
              break;
            }
          }
        }
        if (*buffer)
          dsyslog("%s", buffer);
      }
      p->hits = p->n = 0;
    }
    if (GotHits)
      dsyslog("=====================");
  }
}

static void StripControlCharacters(char *s)
{
  if (s)
  {
    int len = strlen(s);
    while (len > 0)
    {
      int l = cUtf8Utils::Utf8CharLen(s);
      uchar *p = (uchar *) s;
      if (l == 2 && *p == 0xC2) // UTF-8 sequence
        p++;
      if (*p == 0x86 || *p == 0x87)
      {
        memmove(s, p + 1, len - l + 1); // we also copy the terminating 0!
        len -= l;
        l = 0;
      }
      s += l;
      len -= l;
    }
  }
}

#endif

void cEvent::FixEpgBugs(void)
{
  if (m_strTitle.empty()) {
     // we don't want any "(null)" titles
    m_strTitle = tr("No title");
    EpgBugFixStat(12, ChannelID());
  }

  if (cSettings::Get().m_iEPGBugfixLevel == 0)
    goto Final;

  // Some TV stations apparently have their own idea about how to fill in the
  // EPG data. Let's fix their bugs as good as we can:

  // Some channels put the ShortText in quotes and use either the ShortText
  // or the Description field, depending on how long the string is:
  //
  // Title
  // "ShortText". Description
  //
  if (m_strShortText.empty() != !m_strDescription.empty())
  {
    std::string strCheck = !m_strShortText.empty() ? m_strShortText : m_strDescription;
    if (!strCheck.empty() && strCheck.at(0) == '"')
    {
      const char *delim = "\".";
      strCheck.erase(0, 1);
      size_t delimPos = strCheck.find(delim);
      if (delimPos != std::string::npos)
      {
        m_strDescription = m_strShortText = strCheck;
        m_strDescription.erase(0, delimPos);
        m_strShortText.erase(delimPos);
        EpgBugFixStat(1, ChannelID());
      }
    }
  }

  // Some channels put the Description into the ShortText (preceded
  // by a blank) if there is no actual ShortText and the Description
  // is short enough:
  //
  // Title
  //  Description
  //
  if (!m_strShortText.empty() && m_strDescription.empty())
  {
    StringUtils::Trim(m_strShortText);
    m_strDescription = m_strShortText;
    m_strShortText.clear();
    EpgBugFixStat(2, ChannelID());
  }

  // Sometimes they repeat the Title in the ShortText:
  //
  // Title
  // Title
  //
  if (!m_strShortText.empty() && m_strTitle == m_strShortText)
  {
    m_strShortText.clear();
    EpgBugFixStat(3, ChannelID());
  }

  // Some channels put the ShortText between double quotes, which is nothing
  // but annoying (some even put a '.' after the closing '"'):
  //
  // Title
  // "ShortText"[.]
  //
  if (!m_strShortText.empty() && m_strShortText.at(0) == '"')
  {
    size_t len = m_strShortText.size();
    if (len > 2 &&
        (m_strShortText.at(len - 1) == '"' ||
            (m_strShortText.at(len - 1) == '.' && m_strShortText.at(len - 2) == '"')))
    {
      m_strShortText.erase(0, 1);
      const char* tmp = m_strShortText.c_str();
      const char *p = strrchr(tmp, '"');
      if (p)
        m_strShortText.erase(m_strShortText.size() - (p - tmp));
      EpgBugFixStat(4, ChannelID());
    }
  }

  if (cSettings::Get().m_iEPGBugfixLevel <= 1)
    goto Final;

  // Some channels apparently try to do some formatting in the texts,
  // which is a bad idea because they have no way of knowing the width
  // of the window that will actually display the text.
  // Remove excess whitespace:
  StringUtils::Trim(m_strTitle);
  StringUtils::Trim(m_strShortText);
  StringUtils::Trim(m_strDescription);

#define MAX_USEFUL_EPISODE_LENGTH 40
  // Some channels put a whole lot of information in the ShortText and leave
  // the Description totally empty. So if the ShortText length exceeds
  // MAX_USEFUL_EPISODE_LENGTH, let's put this into the Description
  // instead:
  if (!m_strShortText.empty() && m_strDescription.empty())
  {
    if (m_strShortText.size() > MAX_USEFUL_EPISODE_LENGTH)
    {
      m_strDescription.swap(m_strShortText);
      EpgBugFixStat(6, ChannelID());
    }
  }

  // Some channels put the same information into ShortText and Description.
  // In that case we delete one of them:
  if (!m_strShortText.empty() && !m_strDescription.empty() && m_strShortText == m_strDescription)
  {
    if (m_strShortText.size() > MAX_USEFUL_EPISODE_LENGTH)
      m_strShortText.clear();
    else
      m_strDescription.clear();

    EpgBugFixStat(7, ChannelID());
  }

  // Some channels use the ` ("backtick") character, where a ' (single quote)
  // would be normally used. Actually, "backticks" in normal text don't make
  // much sense, so let's replace them:
  StringUtils::Replace(m_strTitle, '`', '\'');
  StringUtils::Replace(m_strShortText, '`', '\'');
  StringUtils::Replace(m_strDescription, '`', '\'');

  if (cSettings::Get().m_iEPGBugfixLevel <= 2)
    goto Final;

  // The stream components have a "description" field which some channels
  // apparently have no idea of how to set correctly:
  if (components)
  {
    for (int i = 0; i < components->NumComponents(); i++)
    {
      CEpgComponent *p = components->Component(i);
      switch (p->Stream())
      {
      case 0x01:
        { // video
          if (!p->Description().empty())
          {
            if (StringUtils::EqualsNoCase(p->Description(), "Video") ||
                StringUtils::EqualsNoCase(p->Description(), "Bildformat"))
            {
              // Yes, we know it's video - that's what the 'stream' code
              // is for! But _which_ video is it?
              p->SetDescription("");
              EpgBugFixStat(8, ChannelID());
            }
          }
          if (p->Description().empty())
          {
            switch (p->Type())
              {
            case 0x01:
            case 0x05:
              p->SetDescription("4:3");
              break;
            case 0x02:
            case 0x03:
            case 0x06:
            case 0x07:
              p->SetDescription("16:9");
              break;
            case 0x04:
            case 0x08:
              p->SetDescription(">16:9");
              break;
            case 0x09:
            case 0x0D:
              p->SetDescription("HD 4:3");
              break;
            case 0x0A:
            case 0x0B:
            case 0x0E:
            case 0x0F:
              p->SetDescription("HD 16:9");
              break;
            case 0x0C:
            case 0x10:
              p->SetDescription("HD >16:9");
              break;
            default:
              break;
              }
            EpgBugFixStat(9, ChannelID());
          }
        }
        break;
      case 0x02:
        { // audio
          if (!p->Description().empty())
          {
            if (StringUtils::EqualsNoCase(p->Description(), "Audio"))
            {
              // Yes, we know it's audio - that's what the 'stream' code
              // is for! But _which_ audio is it?
              p->SetDescription("");
              EpgBugFixStat(10, ChannelID());
            }
          }
          if (p->Description().empty())
          {
            switch (p->Type())
            {
            case 0x05:
              p->SetDescription("Dolby Digital");
              break;
            default:
              break; // all others will just display the language
            }
            EpgBugFixStat(11, ChannelID());
          }
        }
        break;
      default:
        break;
      }
    }
  }

  Final:

  // XXX is this just gui code that can't handle this?

#if 0
  // VDR can't usefully handle newline characters in the title, shortText or component description of EPG
  // data, so let's always convert them to blanks (independent of the setting of EPGBugfixLevel):
  strreplace(title, '\n', ' ');
  strreplace(shortText, '\n', ' ');
  if (components)
  {
    for (int i = 0; i < components->NumComponents(); i++)
    {
      CEpgComponent *p = components->Component(i);
      if (p->description)
        strreplace(p->description, '\n', ' ');
    }
  }
  // Same for control characters:
  StripControlCharacters(title);
  StripControlCharacters(shortText);
  StripControlCharacters(description);
#else
  (void)0;
#endif
}

void AddEventElement(TiXmlElement* eventElement, const std::string& strElement, const std::string& strText)
{
  TiXmlElement textElement(strElement);
  TiXmlNode* textNode = eventElement->InsertEndChild(textElement);
  if (textNode)
  {
    TiXmlText* text = new TiXmlText(StringUtils::Format("%s", strText.c_str()));
    textNode->LinkEndChild(text);
  }
}

bool cEvent::Deserialise(cSchedule* schedule, const TiXmlNode *eventNode)
{
  bool bNewEvent(false);
  EventPtr event;
  assert(schedule);
  assert(eventNode);

  const TiXmlElement *elem = eventNode->ToElement();
  if (elem == NULL)
    return false;

  const char* id       = elem->Attribute(EPG_XML_ATTR_EVENT_ID);
  const char* start    = elem->Attribute(EPG_XML_ATTR_START_TIME);
  const char* duration = elem->Attribute(EPG_XML_ATTR_DURATION);
  const char* tableId  = elem->Attribute(EPG_XML_ATTR_TABLE_ID);
  const char* version  = elem->Attribute(EPG_XML_ATTR_VERSION);
  if (id && start && duration && tableId && version)
  {
    tEventID EventID = (tEventID)StringUtils::IntVal(id);
    CDateTime startTime = CDateTime((time_t)StringUtils::IntVal(start));

    event = schedule->GetEvent(EventID, startTime);

    if (!event)
    {
      event = EventPtr(new cEvent(EventID));
      event->m_startTime = startTime;
      bNewEvent = true;
    }

    event->duration = (int)StringUtils::IntVal(duration);
    event->tableID  = (uchar)StringUtils::IntVal(tableId);
    event->version  = (uchar)StringUtils::IntVal(version);

    const TiXmlNode *titleNode = elem->FirstChild(EPG_XML_ELM_TITLE);
    if (titleNode)
    {
      event->m_strTitle = titleNode->ToElement()->GetText();
    }

    const TiXmlNode *shortTextNode = elem->FirstChild(EPG_XML_ELM_SHORT_TEXT);
    if (shortTextNode)
    {
      event->m_strShortText = shortTextNode->ToElement()->GetText();
    }

    const TiXmlNode *descriptionNode = elem->FirstChild(EPG_XML_ELM_DESCRIPTION);
    if (descriptionNode)
    {
      event->m_strDescription = descriptionNode->ToElement()->GetText();
    }

    const char* parental  = elem->Attribute(EPG_XML_ATTR_PARENTAL);
    if (parental)
      event->parentalRating = (uint16_t)StringUtils::IntVal(parental);

    const char* starRating  = elem->Attribute(EPG_XML_ATTR_STAR);
    if (starRating)
      event->starRating = (uint8_t)StringUtils::IntVal(starRating);

    const char* vps  = elem->Attribute(EPG_XML_ATTR_VPS);
    if (vps)
      event->m_vps = CDateTime((time_t)StringUtils::IntVal(vps));

    const TiXmlNode *contentsNode = elem->FirstChild(EPG_XML_ELM_CONTENTS);
    int iPtr(0);
    while (contentsNode != NULL)
    {
      event->contents[iPtr++] = (uchar)StringUtils::IntVal(contentsNode->ToElement()->GetText());
      contentsNode = contentsNode->NextSibling(EPG_XML_ELM_CONTENTS);
    }

    const TiXmlNode *componentsNode = elem->FirstChild(EPG_XML_ELM_COMPONENTS);
    if (componentsNode)
    {
      int iPtr(0);
      const TiXmlNode *componentNode = componentsNode->FirstChild(EPG_XML_ELM_COMPONENTS);
      while (componentNode != NULL)
      {
        const char* num  = componentNode->ToElement()->Attribute(EPG_XML_ATTR_COMPONENT_ID);
        if (num)
          event->components->SetComponent((int)StringUtils::IntVal(num), componentNode->ToElement()->GetText());
        componentNode = componentNode->NextSibling(EPG_XML_ELM_COMPONENT);
      }
    }

    if (bNewEvent)
      schedule->AddEvent(event);
    else
      schedule->HashEvent(event.get());

    return true;
  }

  return false;
}

bool cEvent::Serialise(TiXmlElement* element) const
{
  assert(element);

  if (EndTime() + CDateTimeSpan(0, 0, cSettings::Get().m_iEPGLinger, 0) >= CDateTime::GetUTCDateTime())
  {
    TiXmlElement eventElement(EPG_XML_ELM_EVENT);
    TiXmlNode *eventNode = element->InsertEndChild(eventElement);

    TiXmlElement* eventNodeElement = eventNode->ToElement();
    if (!eventNodeElement)
      return false;

    time_t tmStart;
    m_startTime.GetAsTime(tmStart);

    eventNodeElement->SetAttribute(EPG_XML_ATTR_EVENT_ID,   eventID);
    eventNodeElement->SetAttribute(EPG_XML_ATTR_START_TIME, tmStart);
    eventNodeElement->SetAttribute(EPG_XML_ATTR_DURATION,   duration);
    eventNodeElement->SetAttribute(EPG_XML_ATTR_TABLE_ID,   tableID);
    eventNodeElement->SetAttribute(EPG_XML_ATTR_VERSION,    version);

    if (!m_strTitle.empty())
      AddEventElement(eventNodeElement, EPG_XML_ELM_TITLE,       m_strTitle.c_str());
    if (!m_strShortText.empty())
      AddEventElement(eventNodeElement, EPG_XML_ELM_SHORT_TEXT,  m_strShortText.c_str());
    if (!m_strDescription.empty())
      AddEventElement(eventNodeElement, EPG_XML_ELM_DESCRIPTION, m_strDescription.c_str());
    if (contents[0])
    {
      for (int i = 0; Contents(i); i++)
      {
        TiXmlElement contentsElement(EPG_XML_ELM_CONTENTS);
        TiXmlNode* contentsNode = eventNodeElement->InsertEndChild(contentsElement);
        if (contentsNode)
        {
          TiXmlElement* contentsElem = contentsNode->ToElement();
          if (contentsElem)
          {
            TiXmlText* text = new TiXmlText(StringUtils::Format("%u", Contents(i)));
            if (text)
              contentsElem->LinkEndChild(text);
          }
        }
      }
    }

    if (parentalRating)
      eventNodeElement->SetAttribute(EPG_XML_ATTR_PARENTAL, parentalRating);

    if (starRating)
      eventNodeElement->SetAttribute(EPG_XML_ATTR_STAR, starRating);

    if (components)
    {
      TiXmlElement componentsElement(EPG_XML_ELM_COMPONENTS);
      TiXmlNode* componentsNode = eventNodeElement->InsertEndChild(componentsElement);
      if (componentsNode)
      {
        for (int i = 0; i < components->NumComponents(); i++)
        {
          CEpgComponent *p = components->Component(i);

          TiXmlElement componentElement(EPG_XML_ELM_COMPONENT);
          TiXmlNode* componentNode = componentsNode->InsertEndChild(componentElement);
          if (componentNode)
          {
            TiXmlElement* componentElem = componentNode->ToElement();
            if (componentElem)
            {
              TiXmlText* text = new TiXmlText(StringUtils::Format("%s", p->Serialise().c_str()));
              if (text)
              {
                componentElem->LinkEndChild(text);
                componentElem->SetAttribute(EPG_XML_ATTR_COMPONENT_ID, i);
              }
            }
          }
        }
      }
    }

    if (m_vps.IsValid())
    {
      time_t tmVps;
      m_vps.GetAsTime(tmVps);
      eventNodeElement->SetAttribute(EPG_XML_ATTR_VPS, tmVps);
    }
  }

  return true;
}

}
