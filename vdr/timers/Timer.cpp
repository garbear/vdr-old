#include "Config.h"
#include "Timers.h"
#include "Timer.h"
#include "TimerDefinitions.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "epg/Event.h"
#include "utils/Status.h"
#include "utils/UTF8Utils.h"
#include "utils/XBMCTinyXML.h"

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

TimerPtr cTimer::EmptyTimer;

cTimer::cTimer(void)
{
  time_t t = time(NULL);
  struct tm tm_r;
  struct tm *now = localtime_r(&t, &tm_r);

  startTime    = 0;
  stopTime     = 0;
  lastSetEvent = 0;
  deferred     = 0;
  recording    = false;
  pending      = false;
  inVpsMargin  = false;
  m_flags      = tfNone;
  m_aux        = NULL;
  event        = NULL;
  m_channel    = cChannelManager::Get().GetByNumber(1 /* XXX */);
  m_day        = SetTime(t, 0);
  m_weekdays   = 0;
  m_start      = now->tm_hour * 100 + now->tm_min;
  m_index      = 0;

  m_stop = now->tm_hour * 60 + now->tm_min + (g_setup.InstantRecordTime ? g_setup.InstantRecordTime : DEFINSTRECTIME);
  m_stop = (m_stop / 60) * 100 + (m_stop % 60);

  if (m_stop >= 2400)
    m_stop -= 2400;
  m_priority = g_setup.DefaultPriority;
  m_lifetime = g_setup.DefaultLifetime;

  Matches();
}

cTimer::cTimer(const cEvent *Event)
{
  startTime    = 0;
  stopTime     = 0;
  lastSetEvent = 0;
  deferred     = 0;
  recording    = false;
  pending      = false;
  inVpsMargin  = false;
  m_flags      = tfActive;
  m_aux        = NULL;
  event        = NULL;
  m_index      = 0;

  if (Event->Vps() && g_setup.UseVps)
    SetFlags(tfVps);

  m_channel = cChannelManager::Get().GetByChannelID(Event->ChannelID(), true);

  time_t tstart = (m_flags & tfVps) ? Event->Vps() : Event->StartTime();
  time_t tstop = tstart + Event->Duration();

  if (!(HasFlags(tfVps)))
  {
    tstop  += g_setup.MarginStop * 60;
    tstart -= g_setup.MarginStart * 60;
  }
  struct tm tm_r;
  struct tm *time = localtime_r(&tstart, &tm_r);
  m_day = SetTime(tstart, 0);
  m_weekdays = 0;
  m_start = time->tm_hour * 100 + time->tm_min;
  time = localtime_r(&tstop, &tm_r);
  m_stop = time->tm_hour * 100 + time->tm_min;
  if (m_stop >= 2400)
     m_stop -= 2400;
  m_priority = g_setup.DefaultPriority;
  m_lifetime = g_setup.DefaultLifetime;
  std::string strTitle = Event->Title();
  if (!strTitle.empty())
    m_file = strTitle;
  SetEvent(Event);
  Matches();
}

cTimer::cTimer(const cTimer &Timer)
{
  m_channel = cChannel::EmptyChannel;
  m_aux     = NULL;
  event   = NULL;
  m_flags = tfNone;
  *this   = Timer;
}

cTimer::~cTimer()
{
  free(m_aux);
}

cTimer& cTimer::operator= (const cTimer &Timer)
{
  if (&Timer != this) {
     uint OldFlags = m_flags & tfRecording;
     startTime    = Timer.startTime;
     stopTime     = Timer.stopTime;
     lastSetEvent = 0;
     deferred     = 0;
     recording    = Timer.recording;
     pending      = Timer.pending;
     inVpsMargin  = Timer.inVpsMargin;
     m_flags      = Timer.m_flags | OldFlags;
     m_channel    = Timer.m_channel;
     m_day        = Timer.m_day;
     m_weekdays   = Timer.m_weekdays;
     m_start      = Timer.m_start;
     m_stop       = Timer.m_stop;
     m_priority   = Timer.m_priority;
     m_lifetime   = Timer.m_lifetime;
     m_file       = Timer.m_file;
     free(m_aux);
     m_aux = Timer.m_aux ? strdup(Timer.m_aux) : NULL;
     event = NULL;
     Matches();
     }
  return *this;
}

bool cTimer::operator==(const cTimer &Timer)
{
  if (&Timer == this)
    return true;

  return (StartTime() == Timer.StartTime() &&
          StopTime() == Timer.StopTime() &&
          m_channel->GetChannelID() == Timer.Channel()->GetChannelID());
}

int cTimer::Compare(const cTimer &Timer) const
{
  time_t t1 = StartTime();
  time_t t2 = Timer.StartTime();
  int r = t1 - t2;
  if (r == 0)
    r = Timer.m_priority - m_priority;
  return r;
}

bool cTimer::SerialiseTimer(TiXmlNode *node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *timerElement = node->ToElement();
  if (timerElement == NULL)
    return false;

  timerElement->SetAttribute(TIMER_XML_ATTR_FLAGS,    m_flags);
  timerElement->SetAttribute(TIMER_XML_ATTR_CHANNEL,  Channel()->GetChannelID().Serialize().c_str());
  timerElement->SetAttribute(TIMER_XML_ATTR_DAYS,     *PrintDay(m_day, m_weekdays, true));
  timerElement->SetAttribute(TIMER_XML_ATTR_START,    m_start);
  timerElement->SetAttribute(TIMER_XML_ATTR_STOP,     m_stop);
  timerElement->SetAttribute(TIMER_XML_ATTR_PRIORITY, m_priority);
  timerElement->SetAttribute(TIMER_XML_ATTR_LIFETIME, m_lifetime);
  timerElement->SetAttribute(TIMER_XML_ATTR_FILENAME, m_file);
  timerElement->SetAttribute(TIMER_XML_ATTR_AUX,      m_aux ? m_aux : "");

  return true;
}

bool cTimer::DeserialiseTimer(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  const char *flags = elem->Attribute(TIMER_XML_ATTR_FLAGS);
  if (flags != NULL)
    m_flags = StringUtils::IntVal(flags);

  const char *channel = elem->Attribute(TIMER_XML_ATTR_CHANNEL);
  if (channel != NULL)
  {
    tChannelID chanId = tChannelID::Deserialize(channel);
    m_channel = cChannelManager::Get().GetByChannelID(chanId);
  }

  const char *days = elem->Attribute(TIMER_XML_ATTR_DAYS);
  if (days != NULL)
  {
    ParseDay(days, m_day, m_weekdays);
  }

  const char *start = elem->Attribute(TIMER_XML_ATTR_START);
  if (start != NULL)
    m_start = StringUtils::IntVal(start);

  const char *stop = elem->Attribute(TIMER_XML_ATTR_STOP);
  if (stop != NULL)
    m_stop = StringUtils::IntVal(stop);

  const char *priority = elem->Attribute(TIMER_XML_ATTR_PRIORITY);
  if (priority != NULL)
    m_priority = StringUtils::IntVal(priority);

  const char *lifetime = elem->Attribute(TIMER_XML_ATTR_LIFETIME);
  if (lifetime != NULL)
    m_lifetime = StringUtils::IntVal(lifetime);

  const char *file = elem->Attribute(TIMER_XML_ATTR_FILENAME);
  if (file != NULL)
    m_file = file;

  //XXX
//  timerElement->SetAttribute(TIMER_XML_ATTR_AUX,      m_aux ? m_aux : "");
  return false;
}

std::string cTimer::ToDescr(void) const
{
  return StringUtils::Format("%d %04d-%04d %s'%s'", Channel()->Number(), m_start, m_stop, HasFlags(tfVps) ? "VPS " : "", m_file.c_str());
}

int cTimer::TimeToInt(int t)
{
  return (t / 100 * 60 + t % 100) * 60;
}

bool cTimer::ParseDay(const char *s, time_t &Day, int &WeekDays)
{
  // possible formats are:
  // 19
  // 2005-03-19
  // MTWTFSS
  // MTWTFSS@19
  // MTWTFSS@2005-03-19

  Day = 0;
  WeekDays = 0;
  s = skipspace(s);
  if (!*s)
     return false;
  const char *a = strchr(s, '@');
  const char *d = a ? a + 1 : isdigit(*s) ? s : NULL;
  if (d) {
     if (strlen(d) == 10) {
        struct tm tm_r;
        if (3 == sscanf(d, "%d-%d-%d", &tm_r.tm_year, &tm_r.tm_mon, &tm_r.tm_mday)) {
           tm_r.tm_year -= 1900;
           tm_r.tm_mon--;
           tm_r.tm_hour = tm_r.tm_min = tm_r.tm_sec = 0;
           tm_r.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
           Day = mktime(&tm_r);
           }
        else
           return false;
        }
     else {
        // handle "day of month" for compatibility with older versions:
        char *tail = NULL;
        int day = strtol(d, &tail, 10);
        if (tail && *tail || day < 1 || day > 31)
           return false;
        time_t t = time(NULL);
        int DaysToCheck = 61; // 61 to handle months with 31/30/31
        for (int i = -1; i <= DaysToCheck; i++) {
            time_t t0 = IncDay(t, i);
            if (GetMDay(t0) == day) {
               Day = SetTime(t0, 0);
               break;
               }
            }
        }
     }
  if (a || !isdigit(*s)) {
     if ((a && a - s == 7) || strlen(s) == 7) {
        for (const char *p = s + 6; p >= s; p--) {
            WeekDays <<= 1;
            WeekDays |= (*p != '-');
            }
        }
     else
        return false;
     }
  return true;
}

cString cTimer::PrintDay(time_t Day, int WeekDays, bool SingleByteChars)
{
#define DAYBUFFERSIZE 64
  char buffer[DAYBUFFERSIZE];
  char *b = buffer;
  if (WeekDays) {
     // TRANSLATORS: the first character of each weekday, beginning with monday
     const char *w = trNOOP("MTWTFSS");
     if (!SingleByteChars)
        w = tr(w);
     while (*w) {
           int sl = cUtf8Utils::Utf8CharLen(w);
           if (WeekDays & 1) {
              for (int i = 0; i < sl; i++)
                  b[i] = w[i];
              b += sl;
              }
           else
              *b++ = '-';
           WeekDays >>= 1;
           w += sl;
           }
     if (Day)
        *b++ = '@';
     }
  if (Day) {
     struct tm tm_r;
     localtime_r(&Day, &tm_r);
     b += strftime(b, DAYBUFFERSIZE - (b - buffer), "%Y-%m-%d", &tm_r);
     }
  *b = 0;
  return buffer;
}

bool cTimer::Parse(const char *s)
{
  char *channelbuffer = NULL;
  char *daybuffer = NULL;
  char *filebuffer = NULL;
  free(m_aux);
  m_aux = NULL;
  //XXX Apparently sscanf() doesn't work correctly if the last %a argument
  //XXX results in an empty string (this first occurred when the EIT gathering
  //XXX was put into a separate thread - don't know why this happens...
  //XXX As a cure we copy the original string and add a blank.
  //XXX If anybody can shed some light on why sscanf() failes here, I'd love
  //XXX to hear about that!
  char *s2 = NULL;
  int l2 = strlen(s);
  while (l2 > 0 && isspace(s[l2 - 1]))
        l2--;
  if (s[l2 - 1] == ':') {
     s2 = MALLOC(char, l2 + 3);
     strcat(strn0cpy(s2, s, l2 + 1), " \n");
     s = s2;
     }
  bool result = false;
  if (8 <= sscanf(s, "%u :%a[^:]:%a[^:]:%d :%d :%d :%d :%a[^:\n]:%a[^\n]", &m_flags, &channelbuffer, &daybuffer, &m_start, &m_stop, &m_priority, &m_lifetime, &filebuffer, &m_aux)) {
     ClrFlags(tfRecording);
     if (m_aux && !*skipspace(m_aux)) {
        free(m_aux);
        m_aux = NULL;
        }
     //TODO add more plausibility checks
     result = ParseDay(daybuffer, m_day, m_weekdays);
     m_file = filebuffer;
     StringUtils::Replace(m_file, '|', ':');
     if (is_number(channelbuffer))
       m_channel = cChannelManager::Get().GetByNumber(atoi(channelbuffer));
     else
       m_channel = cChannelManager::Get().GetByChannelID(tChannelID::Deserialize(channelbuffer), true, true);
     if (!m_channel) {
        esyslog("ERROR: channel %s not defined", channelbuffer);
        result = false;
        }
     }
  free(channelbuffer);
  free(daybuffer);
  free(filebuffer);
  free(s2);
  if (result)
    Matches();
  return result;
}

bool cTimer::IsSingleEvent(void) const
{
  return !m_weekdays;
}

int cTimer::GetMDay(time_t t)
{
  struct tm tm_r;
  return localtime_r(&t, &tm_r)->tm_mday;
}

int cTimer::GetWDay(time_t t)
{
  struct tm tm_r;
  int weekday = localtime_r(&t, &tm_r)->tm_wday;
  return weekday == 0 ? 6 : weekday - 1; // we start with Monday==0!
}

bool cTimer::DayMatches(time_t t) const
{
  return IsSingleEvent() ? SetTime(t, 0) == m_day : (m_weekdays & (1 << GetWDay(t))) != 0;
}

time_t cTimer::IncDay(time_t t, int Days)
{
  struct tm tm_r;
  tm tm = *localtime_r(&t, &tm_r);
  tm.tm_mday += Days; // now tm_mday may be out of its valid range
  int h = tm.tm_hour; // save original hour to compensate for DST change
  tm.tm_isdst = -1;   // makes sure mktime() will determine the correct DST setting
  t = mktime(&tm);    // normalize all values
  tm.tm_hour = h;     // compensate for DST change
  return mktime(&tm); // calculate final result
}

time_t cTimer::SetTime(time_t t, int SecondsFromMidnight)
{
  struct tm tm_r;
  tm tm = *localtime_r(&t, &tm_r);
  tm.tm_hour = SecondsFromMidnight / 3600;
  tm.tm_min = (SecondsFromMidnight % 3600) / 60;
  tm.tm_sec =  SecondsFromMidnight % 60;
  tm.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
  return mktime(&tm);
}

void cTimer::SetFile(const std::string& strFile)
{
  if (!strFile.empty())
    m_file = strFile;
}

#define EITPRESENTFOLLOWINGRATE 10 // max. seconds between two occurrences of the "EIT present/following table for the actual multiplex" (2s by the standard, using some more for safety)

bool cTimer::Matches(time_t t, bool Directly, int Margin)
{
  startTime = stopTime = 0;
  if (t == 0)
     t = time(NULL);

  int begin  = TimeToInt(m_start); // seconds from midnight
  int length = TimeToInt(m_stop) - begin;
  if (length < 0)
     length += SECSINDAY;

  if (IsSingleEvent()) {
     startTime = SetTime(m_day, begin);
     stopTime = startTime + length;
     }
  else {
     for (int i = -1; i <= 7; i++) {
         time_t t0 = IncDay(m_day ? ::max(m_day, t) : t, i);
         if (DayMatches(t0)) {
            time_t a = SetTime(t0, begin);
            time_t b = a + length;
            if ((!m_day || a >= m_day) && t < b) {
               startTime = a;
               stopTime = b;
               break;
               }
            }
         }
     if (!startTime)
        startTime = IncDay(t, 7); // just to have something that's more than a week in the future
     else if (!Directly && (t > startTime || t > m_day + SECSINDAY + 3600)) // +3600 in case of DST change
        m_day = 0;
     }

  if (t < deferred)
     return false;
  deferred = 0;

  if (HasFlags(tfActive)) {
     if (HasFlags(tfVps) && event && event->Vps()) {
        if (Margin || !Directly) {
           startTime = event->StartTime();
           stopTime = event->EndTime();
           if (!Margin) { // this is an actual check
              if (event->Schedule()->PresentSeenWithin(EITPRESENTFOLLOWINGRATE)) { // VPS control can only work with up-to-date events...
                 if (event->StartTime() > 0) // checks for "phased out" events
                    return event->IsRunning(true);
                 }
              return startTime <= t && t < stopTime; // ...otherwise we fall back to normal timer handling
              }
           }
        }
     return startTime <= t + Margin && t < stopTime; // must stop *before* stopTime to allow adjacent timers
     }
  return false;
}

#define FULLMATCH 1000

eTimerMatch cTimer::MatchesEvent(const cEvent *Event, int *Overlap)
{
  // Overlap is the percentage of the Event's duration that is covered by
  // this timer (based on FULLMATCH for finer granularity than just 100).
  // To make sure a VPS timer can be distinguished from a plain 100% overlap,
  // it gets an additional 100 added, and a VPS event that is actually running
  // gets 200 added to the FULLMATCH.
  if (HasFlags(tfActive) && m_channel->GetChannelID() == Event->ChannelID()) {
     bool UseVps = HasFlags(tfVps) && Event->Vps();
     Matches(UseVps ? Event->Vps() : Event->StartTime(), true);
     int overlap = 0;
     if (UseVps)
        overlap = (startTime == Event->Vps()) ? FULLMATCH + (Event->IsRunning() ? 200 : 100) : 0;
     if (!overlap) {
        if (startTime <= Event->StartTime() && Event->EndTime() <= stopTime)
           overlap = FULLMATCH;
        else if (stopTime <= Event->StartTime() || Event->EndTime() <= startTime)
           overlap = 0;
        else
           overlap = (::min(stopTime, Event->EndTime()) - ::max(startTime, Event->StartTime())) * FULLMATCH / ::max(Event->Duration(), 1);
        }
     startTime = stopTime = 0;
     if (Overlap)
        *Overlap = overlap;
     if (UseVps)
        return overlap > FULLMATCH ? tmFull : tmNone;
     return overlap >= FULLMATCH ? tmFull : overlap > 0 ? tmPartial : tmNone;
     }
  return tmNone;
}

#define EXPIRELATENCY 60 // seconds (just in case there's a short glitch in the VPS signal)

bool cTimer::Expired(void) const
{
  return IsSingleEvent() && !Recording() && StopTime() + EXPIRELATENCY <= time(NULL) && (!HasFlags(tfVps) || !event || !event->Vps());
}

time_t cTimer::StartTime(void) const
{
  return startTime;
}

time_t cTimer::StopTime(void) const
{
  return stopTime;
}

#define EPGLIMITBEFORE   (1 * 3600) // Time in seconds before a timer's start time and
#define EPGLIMITAFTER    (1 * 3600) // after its stop time within which EPG events will be taken into consideration.

void cTimer::SetEventFromSchedule(cSchedules *Schedules)
{
  cSchedulesLock SchedulesLock;
  if (!Schedules) {
     lastSetEvent = 0; // forces setting the event, even if the schedule hasn't been modified
     if (!(Schedules = SchedulesLock.Get()))
        return;
     }
  SchedulePtr Schedule = Schedules->GetSchedule(Channel());
  if (Schedule && Schedule->Events()->First()) {
     time_t now = time(NULL);
     if (!lastSetEvent || Schedule->Modified() >= lastSetEvent) {
        lastSetEvent = now;
        const cEvent *Event = NULL;
        if (HasFlags(tfVps) && Schedule->Events()->First()->Vps()) {
           if (event && event->StartTime() > 0) { // checks for "phased out" events
              if (Recording())
                 return; // let the recording end first
              if (now <= event->EndTime() || Matches(0, true))
                 return; // stay with the old event until the timer has completely expired
              }
           // VPS timers only match if their start time exactly matches the event's VPS time:
           for (const cEvent *e = Schedule->Events()->First(); e; e = Schedule->Events()->Next(e)) {
               if (e->StartTime() && e->RunningStatus() != SI::RunningStatusNotRunning) { // skip outdated events
                  int overlap = 0;
                  MatchesEvent(e, &overlap);
                  if (overlap > FULLMATCH) {
                     Event = e;
                     break; // take the first matching event
                     }
                  }
               }
           }
        else {
           // Normal timers match the event they have the most overlap with:
           int Overlap = 0;
           // Set up the time frame within which to check events:
           Matches(0, true);
           time_t TimeFrameBegin = StartTime() - EPGLIMITBEFORE;
           time_t TimeFrameEnd   = StopTime()  + EPGLIMITAFTER;
           for (const cEvent *e = Schedule->Events()->First(); e; e = Schedule->Events()->Next(e)) {
               if (e->EndTime() < TimeFrameBegin)
                  continue; // skip events way before the timer starts
               if (e->StartTime() > TimeFrameEnd)
                  break; // the rest is way after the timer ends
               int overlap = 0;
               MatchesEvent(e, &overlap);
               if (overlap && overlap >= Overlap) {
                  if (Event && overlap == Overlap && e->Duration() <= Event->Duration())
                     continue; // if overlap is the same, we take the longer event
                  Overlap = overlap;
                  Event = e;
                  }
               }
           }
        SetEvent(Event);
        }
     }
}

void cTimer::ClearEvent(void)
{
  if (event)
    isyslog("timer %s set to no event", ToDescr().c_str());
  event = NULL;
}

void cTimer::SetEvent(const cEvent *Event)
{
  if (event != Event) { //XXX TODO check event data, too???
     if (Event)
        isyslog("timer %s set to event %s", ToDescr().c_str(), Event->ToDescr().c_str());
     else
        isyslog("timer %s set to no event", ToDescr().c_str());
     event = Event;
     }
}

void cTimer::SetRecording(bool Recording)
{
  recording = Recording;
  if (recording)
     SetFlags(tfRecording);
  else
     ClrFlags(tfRecording);
  isyslog("timer %s %s", ToDescr().c_str(), recording ? "start" : "stop");
}

void cTimer::SetPending(bool Pending)
{
  pending = Pending;
}

void cTimer::SetInVpsMargin(bool InVpsMargin)
{
  if (InVpsMargin && !inVpsMargin)
     isyslog("timer %s entered VPS margin", ToDescr().c_str());
  inVpsMargin = InVpsMargin;
}

void cTimer::SetDay(time_t Day)
{
  m_day = Day;
}

void cTimer::SetWeekDays(int WeekDays)
{
  m_weekdays = WeekDays;
}

void cTimer::SetStart(int Start)
{
  m_start = Start;
  Matches();
}

void cTimer::SetStop(int Stop)
{
  m_stop = Stop;
  Matches();
}

void cTimer::SetPriority(int Priority)
{
  m_priority = Priority;
}

void cTimer::SetLifetime(int Lifetime)
{
  m_lifetime = Lifetime;
}

void cTimer::SetAux(const char *Aux)
{
  free(m_aux);
  m_aux = strdup(Aux);
}

void cTimer::SetDeferred(int Seconds)
{
  deferred = time(NULL) + Seconds;
  isyslog("timer %s deferred for %d seconds", ToDescr().c_str(), Seconds);
}

void cTimer::SetFlags(uint Flags)
{
  m_flags |= Flags;
}

void cTimer::ClrFlags(uint Flags)
{
  m_flags &= ~Flags;
}

void cTimer::InvFlags(uint Flags)
{
  m_flags ^= Flags;
}

bool cTimer::HasFlags(uint Flags) const
{
  return (m_flags & Flags) == Flags;
}

void cTimer::Skip(void)
{
  m_day = IncDay(SetTime(StartTime(), 0), 1);
  startTime = 0;
  ClearEvent();
}

void cTimer::OnOff(void)
{
  if (IsSingleEvent())
     InvFlags(tfActive);
  else if (m_day) {
     m_day = 0;
     ClrFlags(tfActive);
     }
  else if (HasFlags(tfActive))
     Skip();
  else
     SetFlags(tfActive);
  ClearEvent();
  Matches(); // refresh start and end time
}
