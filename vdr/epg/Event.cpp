#include "Event.h"
#include "EPGDefinitions.h"
#include "Schedule.h"
#include "channels/ChannelManager.h"
#include "recordings/Timers.h"
#include "utils/UTF8Utils.h"
#include "utils/I18N.h"
#include "utils/CalendarUtils.h"
#include "utils/XBMCTinyXML.h"

cEvent::cEvent(tEventID EventID)
{
  schedule = NULL;
  eventID = EventID;
  tableID = 0xFF; // actual table ids are 0x4E..0x60
  version = 0xFF; // actual version numbers are 0..31
  runningStatus = SI::RunningStatusUndefined;
  title = NULL;
  shortText = NULL;
  description = NULL;
  components = NULL;
  memset(contents, 0, sizeof(contents));
  parentalRating = 0;
  starRating = 0;
  startTime = 0;
  duration = 0;
  vps = 0;
  SetSeen();
}

cEvent::~cEvent()
{
  free(title);
  free(shortText);
  free(description);
  delete components;
}

int cEvent::Compare(const cListObject &ListObject) const
{
  cEvent *e = (cEvent *)&ListObject;
  return startTime - e->startTime;
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
    isyslog("channel %d (%s) event %s status %d", Channel->Number(), Channel->Name().c_str(), *ToDescr(), RunningStatus);
  runningStatus = RunningStatus;
}

void cEvent::SetTitle(const char *Title)
{
  title = strcpyrealloc(title, Title);
}

void cEvent::SetShortText(const char *ShortText)
{
  shortText = strcpyrealloc(shortText, ShortText);
}

void cEvent::SetDescription(const char *Description)
{
  description = strcpyrealloc(description, Description);
}

void cEvent::SetComponents(cComponents *Components)
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

void cEvent::SetStartTime(time_t StartTime)
{
  if (startTime != StartTime)
  {
    if (schedule)
      schedule->UnhashEvent(this);
    startTime = StartTime;
    if (schedule)
      schedule->HashEvent(this);
  }
}

void cEvent::SetDuration(int Duration)
{
  duration = Duration;
}

void cEvent::SetVps(time_t Vps)
{
  vps = Vps;
}

void cEvent::SetSeen(void)
{
  seen = time(NULL);
}

cString cEvent::ToDescr(void) const
{
  char vpsbuf[64] = "";
  if (Vps())
    sprintf(vpsbuf, "(VPS: %s) ", *GetVpsString());
  return cString::sprintf("%s %s-%s %s'%s'", *GetDateString(), *GetTimeString(), *GetEndTimeString(), vpsbuf, Title());
}

bool cEvent::HasTimer(void) const
{
  for (cTimer *t = Timers.First(); t; t = Timers.Next(t))
  {
    if (t->Event() == this)
      return true;
  }
  return false;
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

cString cEvent::GetParentalRatingString(void) const
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

  return isempty(buffer) ? NULL : buffer;
}

cString cEvent::GetStarRatingString(void) const
{
  static const char *const critiques[8] = { "", "*", "*+", "**", "**+", "***", "***+", "****" };
  return critiques[starRating & 0x07];
}

cString cEvent::GetDateString(void) const
{
  return CalendarUtils::DateString(startTime).c_str();
}

cString cEvent::GetTimeString(void) const
{
  return CalendarUtils::TimeString(startTime).c_str();
}

cString cEvent::GetEndTimeString(void) const
{
  return CalendarUtils::TimeString(startTime + duration).c_str();
}

cString cEvent::GetVpsString(void) const
{
  char buf[25];
  struct tm tm_r;
  strftime(buf, sizeof(buf), "%d.%m. %R", localtime_r(&vps, &tm_r));
  return buf;
}

bool cEvent::Parse(char *s)
{
  char *t = skipspace(s + 1);
  switch (*s)
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
      components = new cComponents;
    components->SetComponent(COMPONENT_ADD_NEW, t);
    break;
  case 'V':
    SetVps(atoi(t));
    break;
  default:
    esyslog("ERROR: unexpected tag while reading EPG data: %s", s);
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

void cEvent::FixEpgBugs(void)
{
  if (isempty(title)) {
     // we don't want any "(null)" titles
     title = strcpyrealloc(title, tr("No title"));
     EpgBugFixStat(12, ChannelID());
     }

  if (g_setup.EPGBugfixLevel == 0)
     goto Final;

  // Some TV stations apparently have their own idea about how to fill in the
  // EPG data. Let's fix their bugs as good as we can:

  // Some channels put the ShortText in quotes and use either the ShortText
  // or the Description field, depending on how long the string is:
  //
  // Title
  // "ShortText". Description
  //
  if ((shortText == NULL) != (description == NULL))
  {
    char *p = shortText ? shortText : description;
    if (*p == '"')
    {
      const char *delim = "\".";
      char *e = strstr(p + 1, delim);
      if (e)
      {
        *e = 0;
        char *s = strdup(p + 1);
        char *d = strdup(e + strlen(delim));
        free(shortText);
        free(description);
        shortText = s;
        description = d;
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
  if (shortText && !description)
  {
    if (*shortText == ' ')
    {
      memmove(shortText, shortText + 1, strlen(shortText));
      description = shortText;
      shortText = NULL;
      EpgBugFixStat(2, ChannelID());
    }
  }

  // Sometimes they repeat the Title in the ShortText:
  //
  // Title
  // Title
  //
  if (shortText && strcmp(title, shortText) == 0)
  {
    free(shortText);
    shortText = NULL;
    EpgBugFixStat(3, ChannelID());
  }

  // Some channels put the ShortText between double quotes, which is nothing
  // but annoying (some even put a '.' after the closing '"'):
  //
  // Title
  // "ShortText"[.]
  //
  if (shortText && *shortText == '"')
  {
    int l = strlen(shortText);
    if (l > 2
        && (shortText[l - 1] == '"'
            || (shortText[l - 1] == '.' && shortText[l - 2] == '"')))
    {
      memmove(shortText, shortText + 1, l);
      char *p = strrchr(shortText, '"');
      if (p)
        *p = 0;
      EpgBugFixStat(4, ChannelID());
    }
  }

  if (g_setup.EPGBugfixLevel <= 1)
    goto Final;

  // Some channels apparently try to do some formatting in the texts,
  // which is a bad idea because they have no way of knowing the width
  // of the window that will actually display the text.
  // Remove excess whitespace:
  title = compactspace(title);
  shortText = compactspace(shortText);
  description = compactspace(description);

#define MAX_USEFUL_EPISODE_LENGTH 40
  // Some channels put a whole lot of information in the ShortText and leave
  // the Description totally empty. So if the ShortText length exceeds
  // MAX_USEFUL_EPISODE_LENGTH, let's put this into the Description
  // instead:
  if (!isempty(shortText) && isempty(description))
  {
    if (strlen(shortText) > MAX_USEFUL_EPISODE_LENGTH)
    {
      free(description);
      description = shortText;
      shortText = NULL;
      EpgBugFixStat(6, ChannelID());
    }
  }

  // Some channels put the same information into ShortText and Description.
  // In that case we delete one of them:
  if (shortText && description && strcmp(shortText, description) == 0)
  {
    if (strlen(shortText) > MAX_USEFUL_EPISODE_LENGTH)
    {
      free(shortText);
      shortText = NULL;
    }
    else
    {
      free(description);
      description = NULL;
    }
    EpgBugFixStat(7, ChannelID());
  }

  // Some channels use the ` ("backtick") character, where a ' (single quote)
  // would be normally used. Actually, "backticks" in normal text don't make
  // much sense, so let's replace them:
  strreplace(title, '`', '\'');
  strreplace(shortText, '`', '\'');
  strreplace(description, '`', '\'');

  if (g_setup.EPGBugfixLevel <= 2)
     goto Final;

  // The stream components have a "description" field which some channels
  // apparently have no idea of how to set correctly:
  if (components)
  {
    for (int i = 0; i < components->NumComponents(); i++)
    {
      tComponent *p = components->Component(i);
      switch (p->stream)
        {
      case 0x01:
        { // video
          if (p->description)
          {
            if (strcasecmp(p->description, "Video") == 0
                || strcasecmp(p->description, "Bildformat") == 0)
            {
              // Yes, we know it's video - that's what the 'stream' code
              // is for! But _which_ video is it?
              free(p->description);
              p->description = NULL;
              EpgBugFixStat(8, ChannelID());
            }
          }
          if (!p->description)
          {
            switch (p->type)
              {
            case 0x01:
            case 0x05:
              p->description = strdup("4:3");
              break;
            case 0x02:
            case 0x03:
            case 0x06:
            case 0x07:
              p->description = strdup("16:9");
              break;
            case 0x04:
            case 0x08:
              p->description = strdup(">16:9");
              break;
            case 0x09:
            case 0x0D:
              p->description = strdup("HD 4:3");
              break;
            case 0x0A:
            case 0x0B:
            case 0x0E:
            case 0x0F:
              p->description = strdup("HD 16:9");
              break;
            case 0x0C:
            case 0x10:
              p->description = strdup("HD >16:9");
              break;
            default:
              ;
              }
            EpgBugFixStat(9, ChannelID());
          }
        }
        break;
      case 0x02:
        { // audio
          if (p->description)
          {
            if (strcasecmp(p->description, "Audio") == 0)
            {
              // Yes, we know it's audio - that's what the 'stream' code
              // is for! But _which_ audio is it?
              free(p->description);
              p->description = NULL;
              EpgBugFixStat(10, ChannelID());
            }
          }
          if (!p->description)
          {
            switch (p->type)
              {
            case 0x05:
              p->description = strdup("Dolby Digital");
              break;
            default:
              ; // all others will just display the language
              }
            EpgBugFixStat(11, ChannelID());
          }
        }
        break;
      default:
        ;
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
      tComponent *p = components->Component(i);
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
  cEvent* event(NULL);
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
    event = schedule->GetEvent(EventID, (time_t)StringUtils::IntVal(start));
    if (!event)
    {
      event = new cEvent(EventID);
      schedule->AddEvent(event);
    }

    event->duration = (int)StringUtils::IntVal(duration);
    event->tableID  = (uchar)StringUtils::IntVal(tableId);
    event->version  = (uchar)StringUtils::IntVal(version);

    const TiXmlNode *titleNode = elem->FirstChild(EPG_XML_ELM_TITLE);
    if (titleNode)
    {
      delete event->title;
      event->title = strdup(titleNode->ToElement()->GetText());
    }

    const TiXmlNode *shortTextNode = elem->FirstChild(EPG_XML_ELM_SHORT_TEXT);
    if (shortTextNode)
    {
      delete event->shortText;
      event->shortText = strdup(shortTextNode->ToElement()->GetText());
    }

    const TiXmlNode *descriptionNode = elem->FirstChild(EPG_XML_ELM_DESCRIPTION);
    if (descriptionNode)
    {
      delete event->description;
      event->description = strdup(descriptionNode->ToElement()->GetText());
    }

    const char* parental  = elem->Attribute(EPG_XML_ATTR_PARENTAL);
    if (parental)
      event->parentalRating = (uint16_t)StringUtils::IntVal(parental);

    const char* starRating  = elem->Attribute(EPG_XML_ATTR_STAR);
    if (starRating)
      event->starRating = (uint8_t)StringUtils::IntVal(starRating);

    const char* vps  = elem->Attribute(EPG_XML_ATTR_VPS);
    if (vps)
      event->vps = (time_t)StringUtils::IntVal(vps);

    const TiXmlNode *contentsNode = elem->FirstChild(EPG_XML_ELM_CONTENTS);
    int iPtr(0);
    while (contentsNode != NULL)
    {
      event->contents[iPtr++] = (uchar)StringUtils::IntVal(contentsNode->ToElement()->GetText());
      contentsNode = contentsNode->NextSibling(EPG_XML_ELM_CONTENTS);
    }

    //XXX todo: components
    return true;
  }

  return false;
}

bool cEvent::Serialise(TiXmlElement* element) const
{
  assert(element);

  if (startTime + duration + g_setup.EPGLinger * 60 >= time(NULL))
  {
    TiXmlElement eventElement(EPG_XML_ELM_EVENT);
    TiXmlNode *eventNode = element->InsertEndChild(eventElement);

    TiXmlElement* eventNodeElement = eventNode->ToElement();
    if (!eventNodeElement)
      return false;

    eventNodeElement->SetAttribute(EPG_XML_ATTR_EVENT_ID,   eventID);
    eventNodeElement->SetAttribute(EPG_XML_ATTR_START_TIME, startTime);
    eventNodeElement->SetAttribute(EPG_XML_ATTR_DURATION,   duration);
    eventNodeElement->SetAttribute(EPG_XML_ATTR_TABLE_ID,   tableID);
    eventNodeElement->SetAttribute(EPG_XML_ATTR_VERSION,    version);

    if (!isempty(title))
      AddEventElement(eventNodeElement, EPG_XML_ELM_TITLE,       title);
    if (!isempty(shortText))
      AddEventElement(eventNodeElement, EPG_XML_ELM_SHORT_TEXT,  shortText);
    if (!isempty(description))
      AddEventElement(eventNodeElement, EPG_XML_ELM_DESCRIPTION, description);
    if (contents[0])
    {
      TiXmlElement contentsElement(EPG_XML_ELM_CONTENTS);
      TiXmlNode* contentsNode = eventNodeElement->InsertEndChild(contentsElement);
      if (contentsNode)
      {
        TiXmlElement* contentsElem = contentsNode->ToElement();
        if (contentsElem)
        {
          for (int i = 0; Contents(i); i++)
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
          tComponent *p = components->Component(i);

          TiXmlElement componentElement(EPG_XML_ELM_COMPONENT);
          TiXmlNode* componentNode = componentsNode->InsertEndChild(componentElement);
          if (componentNode)
          {
            TiXmlElement* componentElem = componentNode->ToElement();
            if (componentElem)
            {
              TiXmlText* text = new TiXmlText(StringUtils::Format("%s", p->ToString().c_str()));
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

    if (vps)
      eventNodeElement->SetAttribute(EPG_XML_ATTR_VPS, vps);
  }

  return true;
}
