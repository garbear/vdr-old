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

cTimers& cTimers::Get(void)
{
  static cTimers _instance;
  return _instance;
}

cTimers::cTimers(void)
{
  m_iState             = 0;
  m_lastSetEvents     = 0;
  m_lastDeleteExpired = 0;
  m_maxIndex        = 0;
}

TimerPtr cTimers::GetTimer(cTimer *Timer)
{
  CLockObject lock(m_mutex);
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if ((*it)->Channel() == Timer->Channel() &&
       (
           ((*it)->WeekDays() && (*it)->WeekDays() == Timer->WeekDays()) ||
           (!(*it)->WeekDays() && (*it)->Day() == Timer->Day())) &&
       (*it)->Start() == Timer->Start() &&
       (*it)->DurationSecs() == Timer->DurationSecs())
    {
      return (*it);
    }
  }
  return cTimer::EmptyTimer;
}

TimerPtr cTimers::GetNextPendingTimer(time_t Now)
{
  static TimerPtr LastPending;
  TimerPtr t0;
  bool bGetNext(false);
  CLockObject lock(m_mutex);
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if (!(*it)->Recording() && (*it)->Matches(Now))
    {
      if ((*it)->Pending())
      {
        if (bGetNext)
        {
          LastPending = *it;
          return *it;
        }
        if (LastPending && *LastPending == **it)
        {
          bGetNext = true;
        }
      }
      if (!t0 || (*it)->Priority() > t0->Priority())
      {
        t0 = (*it);
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
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    eTimerMatch tm = (*it)->MatchesEvent(Event);
    if (tm > m)
    {
      t = (*it);
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
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    timer = *it;
    timer->Matches();
    if ((timer->HasFlags(tfActive)) &&
        (!retval || (timer->StopTime() > time(NULL) && timer->Compare(*retval) < 0)))
      retval = timer;
  }
  return retval;
}

cRecording* cTimers::GetActiveRecording(const ChannelPtr channel)
{
  CLockObject lock(m_mutex);
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if ((*it)->Recording() && (*it)->Channel().get() == channel.get())
    {
      Recordings.Load();
      cRecording matchRec((*it), (*it)->Event());
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
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if ((*it)->Index() == index)
      return *it;
  }
  return cTimer::EmptyTimer;
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

void cTimers::Add(TimerPtr Timer, TimerPtr After)
{
  {
    CLockObject lock(m_mutex);

    if (After)
    {
      std::vector<TimerPtr>::iterator it = std::find(m_timers.begin(), m_timers.end(), After);
      if (it != m_timers.end())
      {
        m_timers.insert(it, Timer);
        SetChanged();
        NotifyObservers(ObservableMessageTimerAdded);
        return;
      }
    }

    Timer->SetIndex(++m_maxIndex);
    m_timers.push_back(Timer);
    SetChanged();
    NotifyObservers(ObservableMessageTimerAdded);
  }

  Save();
}

void cTimers::Ins(TimerPtr Timer, TimerPtr Before)
{
  CLockObject lock(m_mutex);
  if (Before)
  {
    std::vector<TimerPtr>::iterator previous = m_timers.end();
    for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    {
      if (**it == *Before)
      {
        m_timers.insert(previous, Timer);
        SetChanged();
        NotifyObservers(ObservableMessageTimerAdded);

        return;
      }
      previous = it;
    }
  }
}

void cTimers::Del(TimerPtr Timer, bool DeleteObject)
{
  {
    CLockObject lock(m_mutex);
    std::vector<TimerPtr>::iterator it = std::find(m_timers.begin(), m_timers.end(), Timer);
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
  if (time(NULL) - m_lastSetEvents < 5)
     return;
  cSchedulesLock SchedulesLock(false, 100);
  cSchedules *Schedules = SchedulesLock.Get();
  if (Schedules)
  {
    if (!m_lastSetEvents || Schedules->Modified() >= m_lastSetEvents)
    {
      for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
      {
        (*it)->SetEventFromSchedule(Schedules);
      }
    }
  }
  m_lastSetEvents = time(NULL);
}

void cTimers::DeleteExpired(void)
{
  CLockObject lock(m_mutex);
  if (time(NULL) - m_lastDeleteExpired < 30)
     return;
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end();)
  {
    if ((*it)->Expired())
    {
      isyslog("deleting timer %s", (*it)->ToDescr().c_str());
      it = m_timers.erase(it);
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
  time_t t1 = a->StartTime();
  time_t t2 = b->StartTime();
  int r = t1 - t2;
  if (r == 0)
    r = b->m_iPriority - a->m_iPriority;
  return r;
}

void cTimers::ClearEvents(void)
{
  CLockObject lock(m_mutex);
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    (*it)->ClearEvent();
}

bool cTimers::HasTimer(const cEvent* event) const
{
  CLockObject lock(m_mutex);
  for (std::vector<TimerPtr>::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if ((*it)->Event() == event)
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
    // log
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
      m_timers.push_back(timer);
    }
    else
    {
      // log
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

  for (std::vector<TimerPtr>::const_iterator itTimer = m_timers.begin(); itTimer != m_timers.end(); ++itTimer)
  {
    TiXmlElement timerElement(TIMER_XML_ELM_TIMER);
    TiXmlNode *timerNode = root->InsertEndChild(timerElement);
    (*itTimer)->SerialiseTimer(timerNode);
  }

  if (!file.empty())
    m_strFilename = file;

  assert(!m_strFilename.empty());

  isyslog("saving channel configuration to '%s'", m_strFilename.c_str());
  if (!xmlDoc.SafeSaveFile(m_strFilename))
  {
    esyslog("failed to save the channel configuration: could not write to '%s'", m_strFilename.c_str());
    return false;
  }
  return true;
}

void cTimers::StartNewRecordings(time_t Now)
{
  //TODO
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
  time_t Now = time(0);

  // Process ongoing recordings:
  for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    (*it)->CheckRecordingStatus(Now);

  // Start new recordings:
  StartNewRecordings(Now);

  // Make sure timers "see" their channel early enough:
  static time_t LastTimerCheck = 0;
  if (Now - LastTimerCheck > TIMERCHECKDELTA)
  { // don't do this too often
    for (std::vector<TimerPtr>::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
      (*it)->SwitchTransponder(Now);
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
