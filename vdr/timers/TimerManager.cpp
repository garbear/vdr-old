/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "TimerManager.h"
#include "Timer.h"
#include "TimerDefinitions.h"
#include "utils/DateTime.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>
#include <assert.h>
#include <limits.h>

using namespace PLATFORM;
using namespace std;

namespace VDR
{

cTimerManager::cTimerManager(void)
 : m_maxID(TIMER_INVALID_ID)
{
}

cTimerManager& cTimerManager::Get(void)
{
  static cTimerManager instance;
  return instance;
}

cTimerManager::~cTimerManager(void)
{
  for (TimerMap::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    it->second->UnregisterObserver(this);
}

void cTimerManager::Start(void)
{
  if (m_timers.empty())
    LoadTimers();
  CreateThread(false);
}

void cTimerManager::Stop(void)
{
  StopThread(-1);
  m_timerNotifyEvent.Broadcast();
}

void* cTimerManager::Process(void)
{
  while (!IsStopped())
  {
    const CDateTime now = CDateTime::GetUTCDateTime();

    TimerVector idleTimers = GetIdleTimers(now);
    if (idleTimers.empty())
    {
      m_timerNotifyEvent.Wait();
      continue;
    }

    // Sort timers for greedy processing
    std::sort(idleTimers.begin(), idleTimers.end(), cTimerSorter(now));

    // Start occurring timers
    TimerVector::iterator itTimer = idleTimers.begin();
    while (itTimer != idleTimers.end() && (*itTimer)->IsOccurring(now))
      (*itTimer++)->StartRecording();

    // Sleep until first pending timer
    if (itTimer != idleTimers.end())
    {
      CDateTimeSpan waitTime = (*itTimer)->GetOccurrence(now) - now;
      m_timerNotifyEvent.Wait(std::min(ULONG_MAX, (unsigned long)waitTime.GetSecondsTotal() * 1000));
    }
  }
  return NULL;
}

bool cTimerManager::AddTimer(const TimerPtr& newTimer)
{
  if (newTimer && newTimer->ID() == TIMER_INVALID_ID && newTimer->IsValid(false))
  {
    CLockObject lock(m_mutex);

    for (TimerMap::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
      if (it->second->Conflicts(*newTimer))
        return false;

    newTimer->SetID(++m_maxID);
    newTimer->RegisterObserver(this);
    m_timers.insert(std::make_pair(newTimer->ID(), newTimer));

    SetChanged();

    return true;
  }

  return false;
}

TimerPtr cTimerManager::GetByID(unsigned int id) const
{
  CLockObject lock(m_mutex);

  TimerMap::const_iterator it = m_timers.find(id);
  if (it != m_timers.end())
    return it->second;

  return cTimer::EmptyTimer;
}

TimerVector cTimerManager::GetTimers(void) const
{
  CLockObject lock(m_mutex);

  TimerVector timers;
  for (TimerMap::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    timers.push_back(it->second);

  return timers;
}

size_t cTimerManager::TimerCount(void) const
{
  CLockObject lock(m_mutex);
  return m_timers.size();
}

bool cTimerManager::UpdateTimer(unsigned int id, const cTimer& updatedTimer)
{
  CLockObject lock(m_mutex);

  TimerPtr timer = GetByID(id);
  if (timer)
  {
    // If timer is being enabled, check for conflicts
    if (!timer->IsActive() && updatedTimer.IsActive())
    {
      for (TimerMap::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
        if (it->second->Conflicts(updatedTimer))
          return false;
    }

    *timer = updatedTimer;
    timer->NotifyObservers();
    return true;
  }

  return false;
}

bool cTimerManager::RemoveTimer(unsigned int id, bool bInterruptRecording)
{
  CLockObject lock(m_mutex);

  TimerMap::iterator it = m_timers.find(id);
  if (it != m_timers.end())
  {
    if (it->second->IsRecording(CDateTime::GetUTCDateTime()))
    {
      if (!bInterruptRecording)
        return false;
      it->second->StopRecording();
    }

    it->second->UnregisterObserver(this);
    m_timers.erase(it);

    SetChanged();

    return true;
  }

  return false;
}

TimerVector cTimerManager::GetIdleTimers(const CDateTime& now) const
{
  CLockObject lock(m_mutex);

  TimerVector idleTimers;
  for (TimerMap::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    const TimerPtr& timer = it->second;
    if (  // (1) Timer is active
          timer->IsActive() &&
          // (2) Timer is idle
          !timer->IsRecording(now) &&
          // (3) Timer is not expired
          !timer->IsExpired(now)
       )
    {
      idleTimers.push_back(it->second);
    }
  }

  return idleTimers;
}

void cTimerManager::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageTimerChanged:
    SetChanged();
    break;
  default:
    break;
  }
}

void cTimerManager::NotifyObservers()
{
  CLockObject lock(Observable::m_obsCritSection);
  if (Changed())
  {
    Observable::NotifyObservers(ObservableMessageTimerChanged);
    m_timerNotifyEvent.Broadcast();
    SaveTimers();
  }
}

bool cTimerManager::LoadTimers(void)
{
  if (!LoadTimers("special://home/system/timers.xml") &&
      !LoadTimers("special://vdr/system/timers.xml"))
  {
    // create a new empty file
    SaveTimers("special://home/system/timers.xml");
    return false;
  }

  return true;
}

bool cTimerManager::LoadTimers(const std::string& file)
{
  CLockObject lock(m_mutex);
  m_timers.clear();

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
    return false;

  m_strFilename = file;

  TiXmlElement* root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), TIMER_XML_ROOT))
  {
    esyslog("Failed to find root element '%s' in file '%s'", TIMER_XML_ROOT, file.c_str());
    return false;
  }

  const TiXmlNode* node = root->FirstChild(TIMER_XML_ELM_TIMER);
  while (node != NULL)
  {
    TimerPtr timer = TimerPtr(new cTimer);

    if (timer->Deserialise(node) && !GetByID(timer->ID()))
    {
      if (timer->ID() > m_maxID)
        m_maxID = timer->ID();

      m_timers.insert(make_pair(timer->ID(), timer));
    }

    node = node->NextSibling(TIMER_XML_ELM_TIMER);
  }

  return true;
}

bool cTimerManager::SaveTimers(const string& file /* = "" */)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(TIMER_XML_ROOT);
  TiXmlNode* root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  CLockObject lock(m_mutex);

  for (TimerMap::const_iterator itTimer = m_timers.begin(); itTimer != m_timers.end(); ++itTimer)
  {
    TiXmlElement timerElement(TIMER_XML_ELM_TIMER);
    TiXmlNode* node = root->InsertEndChild(timerElement);
    itTimer->second->Serialise(node);
  }

  if (!file.empty())
    m_strFilename = file;

  assert(!m_strFilename.empty());

  isyslog("Saving timers to '%s'", m_strFilename.c_str());
  if (!xmlDoc.SafeSaveFile(m_strFilename))
  {
    esyslog("Failed to save timers: could not write to '%s'", m_strFilename.c_str());
    return false;
  }
  return true;
}

}
