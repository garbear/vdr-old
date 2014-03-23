/*
 * timers.c: Timer handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: timers.c 2.18 2013/03/29 15:37:16 kls Exp $
 */

#include "Timers.h"
#include "Timer.h"
#include "TimerDefinitions.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "recordings/Recordings.h"
#include "utils/Status.h"
#include "utils/UTF8Utils.h"
#include "utils/XBMCTinyXML.h"

using namespace std;
using namespace PLATFORM;

namespace VDR
{

cTimers& cTimers::Get(void)
{
  static cTimers _instance;
  return _instance;
}

cTimers::cTimers(void)
{
  m_iState             = 0;
  m_lastDeleteExpired = 0;
  m_maxIndex        = 0;
}

TimerPtr cTimers::GetTimer(cTimer *Timer)
{
  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if (it->second->Channel() == Timer->Channel()
        && ((it->second->WeekDays()
            && it->second->WeekDays() == Timer->WeekDays())
            || (!it->second->WeekDays() && it->second->Day() == Timer->Day()))
        && it->second->Start() == Timer->Start()
        && it->second->DurationSecs() == Timer->DurationSecs())
    {
      return it->second;
    }
  }
  return cTimer::EmptyTimer;
}

TimerPtr cTimers::GetNextPendingTimer(const CDateTime& Now)
{
  static TimerPtr LastPending;
  TimerPtr t0;
  bool bGetNext(false);
  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if (!it->second->Recording() && it->second->Matches(Now))
    {
      if (it->second->Pending() && it->second->RecordingAttemptAllowed())
      {
        if (bGetNext)
        {
          LastPending = it->second;
          return it->second;
        }
        if (LastPending && *LastPending == *it->second)
        {
          bGetNext = true;
        }
      }
      if (!t0 || it->second->Priority() > t0->Priority())
      {
        t0 = it->second;
      }
    }
  }
  if (!t0)
    LastPending = cTimer::EmptyTimer;
  return t0;
}

TimerPtr cTimers::GetMatch(const cEvent *Event, eTimerMatch *Match)
{
  TimerPtr t;
  eTimerMatch m = tmNone;
  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    eTimerMatch tm = it->second->MatchesEvent(Event);
    if (tm > m)
    {
      t = it->second;
      m = tm;
      if (m == tmFull)
        break;
    }
  }
  if (Match)
    *Match = m;
  return t;
}

TimerPtr cTimers::GetNextActiveTimer(void)
{
  TimerPtr retval;
  TimerPtr timer;
  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    timer = it->second;
    timer->Matches();
    if ((timer->HasFlags(tfActive)) &&
        (!retval || (timer->EndTime() > CDateTime::GetUTCDateTime() && timer->Compare(*retval) < 0)))
      retval = timer;
  }
  return retval;
}

cRecording* cTimers::GetActiveRecording(const ChannelPtr channel)
{
  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if (it->second->Recording() && it->second->Channel().get() == channel.get())
    {
      Recordings.Load();
      cRecording matchRec(it->second, it->second->Event());
      {
        CThreadLock RecordingsLock(&Recordings);
        return Recordings.GetByName(matchRec.FileName());
      }
      break;
    }
  }

  return NULL;
}

TimerPtr cTimers::GetByIndex(size_t index)
{
  CLockObject lock(m_mutex);
  std::map<size_t, TimerPtr>::iterator it = m_timers.find(index);
  return it != m_timers.end() ? it->second : cTimer::EmptyTimer;
}

std::vector<TimerPtr> cTimers::GetTimers(void) const
{
  std::vector<TimerPtr> timers;
  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    timers.push_back(it->second);
  return timers;
}

void cTimers::SetModified(void)
{
  {
    SetChanged();
    NotifyObservers(ObservableMessageTimerChanged);
    m_iState++;
  }
  Save();
}

void cTimers::Add(TimerPtr Timer)
{
  {
    CLockObject lock(m_mutex);
    Timer->SetIndex(++m_maxIndex);
    m_timers.insert(make_pair(m_maxIndex, Timer));
    SetChanged();
    NotifyObservers(ObservableMessageTimerAdded);
  }

  Save();
}

void cTimers::Del(TimerPtr Timer)
{
  {
    CLockObject lock(m_mutex);
    std::map<size_t, TimerPtr>::iterator it = m_timers.find(Timer->Index());
    if (it != m_timers.end())
    {
      m_timers.erase(it);
      SetChanged();
      NotifyObservers(ObservableMessageTimerDeleted);
    }
  }

  Save();
}

bool cTimers::Modified(int &State)
{
  bool Result = m_iState != State;
  State = m_iState;
  return Result;
}

void cTimers::SetEvents(void)
{
  CLockObject lock(m_mutex);
  CDateTime now = CDateTime::GetUTCDateTime();
  if ((now - m_lastSetEvents).GetSecondsTotal() < 5)
     return;
  cSchedulesLock SchedulesLock(false, 100);
  cSchedules *Schedules = SchedulesLock.Get();
  if (Schedules)
  {
    if (!m_lastSetEvents.IsValid() || Schedules->Modified() >= m_lastSetEvents)
    {
      for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
      {
        it->second->SetEventFromSchedule(Schedules);
      }
    }
  }
  m_lastSetEvents = now;
}

void cTimers::DeleteExpired(void)
{
  CLockObject lock(m_mutex);
  if (time(NULL) - m_lastDeleteExpired < 30)
     return;
  for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end();)
  {
    if (it->second->Expired())
    {
      isyslog("deleting timer %s", it->second->ToDescr().c_str());
      m_timers.erase(it++);
      SetChanged();
      NotifyObservers(ObservableMessageTimerDeleted);
    }
    else
    {
      ++it;
    }
  }
  m_lastDeleteExpired = time(NULL);
}

int cTimer::CompareTimers(const cTimer *a, const cTimer *b)
{
  int diff = (a->StartTime() - b->StartTime()).GetSecondsTotal();
  return (diff == 0) ?
      b->m_iPriority - a->m_iPriority :
      diff;
}

void cTimers::ClearEvents(void)
{
  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    it->second->ClearEvent();
}

bool cTimers::HasTimer(const cEvent* event) const
{
  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if (it->second->Event() == event)
      return true;
  }
  return false;
}

bool cTimers::Load(void)
{
  if (!Load("special://home/system/timers.xml") &&
      !Load("special://vdr/system/timers.xml"))
  {
    // create a new empty file
    Save("special://home/system/timers.xml");
    return false;
  }

  return true;
}

bool cTimers::Load(const std::string &file)
{
  CLockObject lock(m_mutex);
  m_timers.clear();

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
    return false;

  m_strFilename = file;

  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), TIMER_XML_ROOT))
  {
    esyslog("failed to find root element '%s' in file '%s'", TIMER_XML_ROOT, file.c_str());
    return false;
  }

  const char* maxIndex = root->Attribute(TIMER_XML_ATTR_MAX_INDEX);
  if (maxIndex != NULL)
    m_maxIndex = StringUtils::IntVal(maxIndex);

  const TiXmlNode *timerNode = root->FirstChild(TIMER_XML_ELM_TIMER);
  while (timerNode != NULL)
  {
    TimerPtr timer = TimerPtr(new cTimer);
    if (timer && timer->DeserialiseTimer(timerNode))
    {
      if (m_timers.find(timer->Index()) == m_timers.end())
        m_timers.insert(make_pair(timer->Index(), timer));
    }
    else
    {
      esyslog("failed to find deserialise timer in file '%s'", file.c_str());
    }
    timerNode = timerNode->NextSibling(TIMER_XML_ELM_TIMER);
  }

  return true;
}


bool cTimers::Save(const string &file /* = ""*/)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(TIMER_XML_ROOT);
  rootElement.SetAttribute(TIMER_XML_ATTR_MAX_INDEX, m_maxIndex);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  CLockObject lock(m_mutex);

  for (std::map<size_t, TimerPtr>::const_iterator itTimer = m_timers.begin(); itTimer != m_timers.end(); ++itTimer)
  {
    TiXmlElement timerElement(TIMER_XML_ELM_TIMER);
    TiXmlNode *timerNode = root->InsertEndChild(timerElement);
    itTimer->second->SerialiseTimer(timerNode);
  }

  if (!file.empty())
    m_strFilename = file;

  assert(!m_strFilename.empty());

  isyslog("saving timers to '%s'", m_strFilename.c_str());
  if (!xmlDoc.SafeSaveFile(m_strFilename))
  {
    esyslog("failed to save timers: could not write to '%s'", m_strFilename.c_str());
    return false;
  }
  return true;
}

void cTimers::StartNewRecordings(const CDateTime& Now)
{
  TimerPtr Timer = GetNextPendingTimer(Now);
  if (Timer)
    Timer->StartRecording();
}

void cTimers::Process(void)
{
  CLockObject lock(m_mutex);

  // Assign events to timers:
  SetEvents();

  // Must do all following calls with the exact same time!
  CDateTime Now = CDateTime::GetUTCDateTime();

  // Process ongoing recordings:
  for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    it->second->CheckRecordingStatus(Now);

  // Start new recordings:
  StartNewRecordings(Now);

  // Make sure timers "see" their channel early enough:
  static CDateTime LastTimerCheck;
  if (!LastTimerCheck.IsValid() || (Now - LastTimerCheck).GetSecondsTotal() > TIMERCHECKDELTA)
  { // don't do this too often
    for (std::map<size_t, TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
      it->second->SwitchTransponder(Now);
    LastTimerCheck = Now;
  }

  // Delete expired timers:
  DeleteExpired();
}

size_t cTimers::Size(void)
{
  CLockObject lock(m_mutex);
  return m_timers.size();
}

TimerPtr cTimers::GetTimerForRecording(const cRecording* recording) const
{
  TimerPtr retval;
  if (!recording)
    return retval;

  CLockObject lock(m_mutex);
  for (std::map<size_t, TimerPtr>::const_iterator it = m_timers.begin(); !retval && it != m_timers.end(); ++it)
  {
    cRecording* tmrRecording = it->second->GetRecording();
    if (tmrRecording && *tmrRecording == *recording)
      retval = it->second;
  }
  return retval;
}

}
