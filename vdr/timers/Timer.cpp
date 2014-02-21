#include "Config.h"
#include "Timers.h"
#include "Timer.h"
#include "TimerDefinitions.h"
#include "recordings/Recordings.h"
#include "recordings/Recorder.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "epg/Event.h"
#include "filesystem/Videodir.h"
#include "utils/Status.h"
#include "utils/TimeUtils.h"
#include "utils/UTF8Utils.h"
#include "utils/XBMCTinyXML.h"

TimerPtr cTimer::EmptyTimer;

cTimer::cTimer(void)
{
  time_t t = time(NULL);
  struct tm tm_r;
  struct tm *now = localtime_r(&t, &tm_r);

  m_startTime               = 0;
  m_stopTime                = 0;
  m_bPending                = false;
  m_bInVpsMargin            = false;
  m_iTimerFlags             = tfNone;
  m_epgEvent                = NULL;
  m_channel                 = cChannelManager::Get().GetByNumber(1 /* XXX */);
  m_iFirstDay               = CTimeUtils::SetTime(t, 0);
  m_iWeekdaysMask           = tdNone;
  m_iStartSecsSinceMidnight = now->tm_hour * 100 + now->tm_min;
  m_iDurationSecs           = g_setup.InstantRecordTime ? g_setup.InstantRecordTime : DEFINSTRECTIME;
  m_index                   = 0;
  m_iPriority               = g_setup.DefaultPriority;
  m_iLifetimeDays           = g_setup.DefaultLifetime;
  m_recorder                = NULL;

  Matches();
}

cTimer::cTimer(ChannelPtr channel, time_t startTime, int iDurationSecs, time_t iFirstDay, uint32_t iWeekdaysMask, uint32_t iTimerFlags, uint32_t iPriority, uint32_t iLifetimeDays, const char *strRecordingFilename)
{
  m_startTime               = 0;
  m_stopTime                = 0;
  m_bPending                = false;
  m_bInVpsMargin            = false;
  m_channel                 = channel;
  m_iStartSecsSinceMidnight = startTime;
  m_iDurationSecs           = iDurationSecs;
  m_iFirstDay               = iFirstDay;
  m_iWeekdaysMask           = iWeekdaysMask;
  m_iTimerFlags             = iTimerFlags;
  m_iPriority               = iPriority;
  m_iLifetimeDays           = iLifetimeDays;
  m_strRecordingFilename    = strRecordingFilename;
  m_epgEvent                = NULL;
  m_index                   = 0;
  m_recorder                = NULL;

  Matches();
}

cTimer::cTimer(const cEvent *Event)
{
  m_startTime            = 0;
  m_stopTime             = 0;
  m_bPending             = false;
  m_bInVpsMargin         = false;
  m_iTimerFlags          = tfActive;
  m_epgEvent             = NULL;
  m_index                = 0;
  m_channel              = cChannelManager::Get().GetByChannelID(Event->ChannelID(), true);
  m_recorder             = NULL;

  if (Event->Vps() && g_setup.UseVps)
    SetFlags(tfVps);

  time_t tstart = (m_iTimerFlags & tfVps) ? Event->Vps() : Event->StartTime();
  m_iDurationSecs = Event->Duration();

  if (!(HasFlags(tfVps)))
  {
    tstart          -= g_setup.MarginStart * 60;
    m_iDurationSecs += (g_setup.MarginStart * 60) + (g_setup.MarginStop * 60);
  }

  struct tm tm_r;
  struct tm *time = localtime_r(&tstart, &tm_r);
  m_iFirstDay     = CTimeUtils::SetTime(tstart, 0);
  m_iWeekdaysMask = tdNone;
  m_iStartSecsSinceMidnight = time->tm_hour * 100 + time->tm_min;
  m_iPriority               = g_setup.DefaultPriority;
  m_iLifetimeDays           = g_setup.DefaultLifetime;
  std::string strTitle      = Event->Title();

  if (!strTitle.empty())
    m_strRecordingFilename = strTitle;

  SetEvent(Event);
  Matches();
}

cTimer::cTimer(const cTimer &Timer)
{
  m_channel     = cChannel::EmptyChannel;
  m_epgEvent    = NULL;
  m_iTimerFlags = tfNone;
  *this         = Timer;
}

cTimer::~cTimer()
{
}

cTimer& cTimer::operator= (const cTimer &Timer)
{
  if (&Timer != this)
  {
     m_startTime               = Timer.m_startTime;
     m_stopTime                = Timer.m_stopTime;
     m_lastEPGEventCheck.Reset();
     m_lastRecordingAttempt    = Timer.m_lastRecordingAttempt;
     m_bPending                = Timer.m_bPending;
     m_bInVpsMargin            = Timer.m_bInVpsMargin;
     m_iTimerFlags             = Timer.m_iTimerFlags | (m_iTimerFlags & tfRecording);
     m_channel                 = Timer.m_channel;
     m_iFirstDay               = Timer.m_iFirstDay;
     m_iWeekdaysMask           = Timer.m_iWeekdaysMask;
     m_iStartSecsSinceMidnight = Timer.m_iStartSecsSinceMidnight;
     m_iDurationSecs           = Timer.m_iDurationSecs;
     m_iPriority               = Timer.m_iPriority;
     m_iLifetimeDays           = Timer.m_iLifetimeDays;
     m_strRecordingFilename    = Timer.m_strRecordingFilename;
     m_epgEvent                = NULL;
     m_recorder                = Timer.m_recorder;

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
    r = Timer.m_iPriority - m_iPriority;
  return r;
}

bool cTimer::SerialiseTimer(TiXmlNode *node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *timerElement = node->ToElement();
  if (timerElement == NULL)
    return false;

  timerElement->SetAttribute(TIMER_XML_ATTR_ID,       m_index);
  timerElement->SetAttribute(TIMER_XML_ATTR_FLAGS,    m_iTimerFlags);
  timerElement->SetAttribute(TIMER_XML_ATTR_CHANNEL,  Channel()->GetChannelID().Serialize().c_str());
  timerElement->SetAttribute(TIMER_XML_ATTR_DAYS,     CTimeUtils::PrintDay(m_iFirstDay, m_iWeekdaysMask, true).c_str());
  timerElement->SetAttribute(TIMER_XML_ATTR_START,    m_iStartSecsSinceMidnight);
  timerElement->SetAttribute(TIMER_XML_ATTR_START,    m_iStartSecsSinceMidnight);
  timerElement->SetAttribute(TIMER_XML_ATTR_DURATION, m_iDurationSecs);
  timerElement->SetAttribute(TIMER_XML_ATTR_PRIORITY, m_iPriority);
  timerElement->SetAttribute(TIMER_XML_ATTR_LIFETIME, m_iLifetimeDays);
  timerElement->SetAttribute(TIMER_XML_ATTR_FILENAME, m_strRecordingFilename);

  return true;
}

bool cTimer::DeserialiseTimer(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_ID))
    m_index = StringUtils::IntVal(attr);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_FLAGS))
    m_iTimerFlags = StringUtils::IntVal(attr);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_CHANNEL))
  {
    tChannelID chanId = tChannelID::Deserialize(attr);
    m_channel = cChannelManager::Get().GetByChannelID(chanId);
  }

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_DAYS))
    CTimeUtils::ParseDay(attr, m_iFirstDay, m_iWeekdaysMask);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_START))
    m_iStartSecsSinceMidnight = StringUtils::IntVal(attr);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_DURATION))
    m_iDurationSecs = StringUtils::IntVal(attr);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_PRIORITY))
    m_iPriority = StringUtils::IntVal(attr);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_LIFETIME))
    m_iLifetimeDays = StringUtils::IntVal(attr);

  if (const char* attr = elem->Attribute(TIMER_XML_ATTR_FILENAME))
    m_strRecordingFilename = attr;

  return true;
}

std::string cTimer::ToDescr(void) const
{
  return StringUtils::Format("%d %04d-%04d %s'%s'", Channel()->Number(), m_iStartSecsSinceMidnight, CTimeUtils::IntToTime(CTimeUtils::TimeToInt(m_iStartSecsSinceMidnight) + m_iDurationSecs), HasFlags(tfVps) ? "VPS " : "", m_strRecordingFilename.c_str());
}

bool cTimer::IsRepeatingEvent(void) const
{
  return m_iWeekdaysMask;
}

bool cTimer::DayMatches(time_t t) const
{
  return !IsRepeatingEvent() ?
      CTimeUtils::SetTime(t, 0) == m_iFirstDay :
      (m_iWeekdaysMask & (1 << CTimeUtils::GetWDay(t))) != 0;
}

void cTimer::SetRecordingFilename(const std::string& strFile)
{
  if (!strFile.empty())
    m_strRecordingFilename = strFile;
}

#define EITPRESENTFOLLOWINGRATE 10 // max. seconds between two occurrences of the "EIT present/following table for the actual multiplex" (2s by the standard, using some more for safety)

bool cTimer::Matches(time_t checkTime, bool bDirectly, int iMarginSeconds)
{
  m_startTime = 0;
  m_stopTime  = 0;

  if (checkTime == 0)
    checkTime = time(NULL);

  int begin  = CTimeUtils::TimeToInt(m_iStartSecsSinceMidnight); // seconds from midnight
  int length = m_iDurationSecs;
  if (length < 0)
    length += SECSINDAY;

  if (!IsRepeatingEvent())
  {
    m_startTime = CTimeUtils::SetTime(m_iFirstDay, begin);
    m_stopTime  = m_startTime + length;
  }
  else
  {
    for (int i = -1; i <= 7; i++)
    {
      time_t t0 = CTimeUtils::IncDay(m_iFirstDay ? ::max(m_iFirstDay, checkTime) : checkTime, i);
      if (DayMatches(t0))
      {
        time_t a = CTimeUtils::SetTime(t0, begin);
        time_t b = a + length;
        if ((!m_iFirstDay || a >= m_iFirstDay) && checkTime < b)
        {
          m_startTime = a;
          m_stopTime = b;
          break;
        }
      }
    }
    if (!m_startTime)
      m_startTime = CTimeUtils::IncDay(checkTime, 7); // just to have something that's more than a week in the future
    else if (!bDirectly && (checkTime > m_startTime || checkTime > m_iFirstDay + SECSINDAY + 3600)) // +3600 in case of DST change
      m_iFirstDay = 0;
  }

  if (HasFlags(tfActive))
  {
    if (HasFlags(tfVps) && m_epgEvent && m_epgEvent->Vps())
    {
      if (iMarginSeconds || !bDirectly)
      {
        m_startTime = m_epgEvent->StartTime();
        m_stopTime = m_epgEvent->EndTime();
        if (!iMarginSeconds)
        { // this is an actual check
          if (m_epgEvent->Schedule()->PresentSeenWithin(EITPRESENTFOLLOWINGRATE))
          { // VPS control can only work with up-to-date events...
            if (m_epgEvent->StartTime() > 0) // checks for "phased out" events
              return m_epgEvent->IsRunning(true);
          }
          return m_startTime <= checkTime && checkTime < m_stopTime; // ...otherwise we fall back to normal timer handling
        }
      }
    }
    return m_startTime <= checkTime + iMarginSeconds && checkTime < m_stopTime; // must stop *before* stopTime to allow adjacent timers
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
  if (HasFlags(tfActive) && m_channel->GetChannelID() == Event->ChannelID())
  {
    bool UseVps = HasFlags(tfVps) && Event->Vps();
    Matches(UseVps ? Event->Vps() : Event->StartTime(), true);
    int overlap = 0;
    if (UseVps)
      overlap = (m_startTime == Event->Vps()) ? FULLMATCH + (Event->IsRunning() ? 200 : 100) : 0;
    if (!overlap)
    {
      if (m_startTime <= Event->StartTime() && Event->EndTime() <= m_stopTime)
        overlap = FULLMATCH;
      else if (m_stopTime <= Event->StartTime() || Event->EndTime() <= m_startTime)
        overlap = 0;
      else
        overlap = (::min(m_stopTime, Event->EndTime()) - ::max(m_startTime, Event->StartTime())) * FULLMATCH / ::max(Event->Duration(), 1);
    }
    m_startTime = m_stopTime = 0;
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
  return !IsRepeatingEvent() &&
      !Recording() &&
      StopTime() + EXPIRELATENCY <= time(NULL) &&
      (!HasFlags(tfVps) || !m_epgEvent || !m_epgEvent->Vps());
}

time_t cTimer::StartTime(void) const
{
  return m_startTime;
}

time_t cTimer::StopTime(void) const
{
  return m_stopTime;
}

#define EPGLIMITBEFORE   (1 * 3600) // Time in seconds before a timer's start time and
#define EPGLIMITAFTER    (1 * 3600) // after its stop time within which EPG events will be taken into consideration.

void cTimer::SetEventFromSchedule(cSchedules *Schedules)
{
  cSchedulesLock SchedulesLock;
  if (!Schedules) {
     m_lastEPGEventCheck.Reset(); // forces setting the event, even if the schedule hasn't been modified
     if (!(Schedules = SchedulesLock.Get()))
        return;
     }
  SchedulePtr Schedule = Schedules->GetSchedule(Channel());
  if (Schedule && Schedule->Events()->First()) {
     time_t now = time(NULL);
     if (!m_lastEPGEventCheck.IsValid() || Schedule->Modified() >= m_lastEPGEventCheck) {
        m_lastEPGEventCheck = now;
        const cEvent *Event = NULL;
        if (HasFlags(tfVps) && Schedule->Events()->First()->Vps()) {
           if (m_epgEvent && m_epgEvent->StartTime() > 0) { // checks for "phased out" events
              if (Recording())
                 return; // let the recording end first
              if (now <= m_epgEvent->EndTime() || Matches(0, true))
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
  if (m_epgEvent)
    isyslog("timer %s set to no event", ToDescr().c_str());
  m_epgEvent = NULL;
}

void cTimer::SetEvent(const cEvent *Event)
{
  if (m_epgEvent != Event)
  { //XXX TODO check event data, too???
    if (Event)
    {
      isyslog("timer %s set to event %s", ToDescr().c_str(), Event->ToDescr().c_str());
      if (m_recorder)
        m_recorder->SetEvent(Event);
    }
    else
      isyslog("timer %s set to no event", ToDescr().c_str());
    m_epgEvent = Event;
  }
}

void cTimer::SetRecording(cRecorder* recorder)
{
  m_recorder = recorder;
  if (m_recorder)
    SetFlags(tfRecording);
  else
    ClrFlags(tfRecording);
  isyslog("timer %s %s", ToDescr().c_str(), m_recorder ? "start" : "stop");
}

void cTimer::SetPending(bool Pending)
{
  m_bPending = Pending;
}

void cTimer::SetInVpsMargin(bool InVpsMargin)
{
  if (InVpsMargin && !m_bInVpsMargin)
     isyslog("timer %s entered VPS margin", ToDescr().c_str());
  m_bInVpsMargin = InVpsMargin;
}

void cTimer::SetDay(time_t Day)
{
  m_iFirstDay = Day;
}

void cTimer::SetWeekDays(int WeekDays)
{
  m_iWeekdaysMask = WeekDays;
}

void cTimer::SetStart(int Start)
{
  m_iStartSecsSinceMidnight = Start;
  Matches();
}

void cTimer::SetDuration(int iDurationSecs)
{
  m_iDurationSecs = iDurationSecs;
  Matches();
}

void cTimer::SetPriority(int Priority)
{
  m_iPriority = Priority;
}

void cTimer::SetLifetimeDays(int iLifetimeDays)
{
  m_iLifetimeDays = iLifetimeDays;
}

void cTimer::SetFlags(uint Flags)
{
  m_iTimerFlags |= Flags;
}

void cTimer::ClrFlags(uint Flags)
{
  m_iTimerFlags &= ~Flags;
}

void cTimer::InvFlags(uint Flags)
{
  m_iTimerFlags ^= Flags;
}

bool cTimer::HasFlags(uint Flags) const
{
  return (m_iTimerFlags & Flags) == Flags;
}

void cTimer::Skip(void)
{
  m_iFirstDay = CTimeUtils::IncDay(CTimeUtils::SetTime(StartTime(), 0), 1);
  m_startTime = 0;
  ClearEvent();
}

void cTimer::OnOff(void)
{
  if (!IsRepeatingEvent())
    InvFlags(tfActive);
  else if (m_iFirstDay)
  {
    m_iFirstDay = 0;
    ClrFlags(tfActive);
  }
  else if (HasFlags(tfActive))
    Skip();
  else
    SetFlags(tfActive);

  ClearEvent();
  Matches(); // refresh start and end time
}

bool cTimer::StartRecording(void)
{
  static time_t LastNoDiskSpaceMessage = 0;
  disk_space_t space;

  // already recording
  if (Recording())
    return true;

  CDateTime now = CDateTime::GetCurrentDateTime();
  if (m_lastRecordingAttempt.IsValid() && (now - m_lastRecordingAttempt).GetSecondsTotal() < RECORDING_START_INTERVAL_SECS)
    return false;

  SetPending(true);
  m_lastRecordingAttempt = now;

  // check free disk space
  Recordings.AssertFreeDiskSpace(Priority(), !Pending());
  VideoDiskSpace(space);
  if (space.free < MINFREEDISKSPACE)
  {
    if (time(NULL) - LastNoDiskSpaceMessage > DISKCHECKINTERVAL)
    {
      isyslog("not enough disk space to start recording %s", ToDescr().c_str());
      LastNoDiskSpaceMessage = time(NULL);
    }
    return false;
  }
  LastNoDiskSpaceMessage = 0;

  ChannelPtr channel = Channel();
  if (channel)
  {
    DevicePtr device = cDeviceManager::Get().GetDevice(*channel, Priority(), false);
    if (device)
    {
      // switch channels
      dsyslog("switching device %d to channel %d", device->CardIndex(), channel->Number());
      if (!device->Channel()->SwitchChannel(channel))
      {
//        XXX why? ShutdownHandler.RequestEmergencyExit();
        return false;
      }

      TimerPtr timerPtr = cTimers::Get().GetTimer(this);
      cRecording recording(timerPtr, Event());
//      cRecordingUserCommand::Get().InvokeCommand(RUC_BEFORERECORDING, recording.FileName());
      isyslog("start recording to '%s'", recording.FileName().c_str());

      // create the directory for the recording
      if (MakeDirs(recording.FileName(), true))
      {
        // start recording
        cRecorder* recorder = new cRecorder(recording.FileName(), channel, Priority());
        if (device->Receiver()->AttachReceiver(recorder))
        {
          recording.WriteInfo();
//          cStatus::MsgRecording(device, recording.Name(), recording.FileName(), true);
//          if (!Timer && !cReplayControl::LastReplayed()) // an instant recording, maybe from cRecordControls::PauseLiveVideo()
//                       cReplayControl::SetRecording(fileName);
          Recordings.AddByName(recording.FileName());
          SetRecording(recorder);
        }
        else
        {
          // failed to attach receiver
          DELETENULL(recorder);
        }
      }
      else
      {
        // failed to create directory
        esyslog("failed to create directory for recording");
      }
    }
    else
    {
      // failed to find device for channel
//      SetDeferred(DEFERTIMER);
      isyslog("no free DVB device to record channel '%s'", channel->Name().c_str());
    }
  }
  return false;
}

void cTimer::SwitchTransponder(time_t Now)
{
  bool InVpsMargin = false;
  bool NeedsTransponder = false;
  ChannelPtr channel = Channel();

  if (HasFlags(tfActive) && !Recording())
  {
    if (HasFlags(tfVps))
    {
      if (Matches(Now, true, cSetup::Get().VpsMargin))
      {
        InVpsMargin = true;
        SetInVpsMargin(InVpsMargin);
      }
      else if (Event())
      {
        InVpsMargin = Event()->StartTime() <= Now && Now < Event()->EndTime();
        NeedsTransponder = Event()->StartTime() - Now < VPSLOOKAHEADTIME * 3600 && !Event()->SeenWithin(VPSUPTODATETIME);
      }
      else
      {
        cSchedulesLock SchedulesLock;
        cSchedules *Schedules = SchedulesLock.Get();
        if (Schedules)
        {
          SchedulePtr Schedule = Schedules->GetSchedule(channel->GetChannelID());
          InVpsMargin = !Schedule; // we must make sure we have the schedule
          NeedsTransponder = Schedule && !Schedule->PresentSeenWithin(VPSUPTODATETIME);
        }
      }
//      InhibitEpgScan |= InVpsMargin | NeedsTransponder;
    }
    else
    {
      NeedsTransponder = Matches(Now, true, TIMERLOOKAHEADTIME);
    }
  }

  if (NeedsTransponder || InVpsMargin)
  {
    // Find a device that provides the required transponder:
    DevicePtr device = cDeviceManager::Get().GetDevice(*channel, MINPRIORITY, false);
    if (!device && InVpsMargin)
      device = cDeviceManager::Get().GetDevice(*channel, LIVEPRIORITY, false);

    // Switch the device to the transponder:
    if (device)
    {
      if (!device->Channel()->IsTunedToTransponder(*channel))
      {
        dsyslog("switching device %d to channel '%s'", device->CardIndex(), Channel()->Name().c_str());
        if (device->Channel()->SwitchChannel(channel))
          device->Channel()->SetOccupied(TIMERDEVICETIMEOUT);
      }
    }
  }
}

bool cTimer::CheckRecordingStatus(time_t Now)
{
  if (m_recorder)
  {
    if (!m_recorder->IsAttached() || !Matches(Now))
    {
      SetPending(false);
      m_recorder->DetachDevice();
      delete m_recorder;
      SetRecording(NULL);

      return false;
    }
    Recordings.AssertFreeDiskSpace(Priority());
  }
  return true;
}

bool cTimer::RecordingAttemptAllowed(void) const
{
  return !m_lastRecordingAttempt.IsValid() ||
      (CDateTime::GetCurrentDateTime() - m_lastRecordingAttempt).GetSecondsTotal() > RECORDING_START_INTERVAL_SECS;
}
