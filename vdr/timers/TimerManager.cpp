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
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>
#include <assert.h>

using namespace PLATFORM;
using namespace std;

namespace VDR
{

cTimerManager::cTimerManager(void)
 : m_maxIndex(TIMER_INVALID_INDEX)
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

TimerPtr cTimerManager::GetByIndex(unsigned int index) const
{
  CLockObject lock(m_mutex);

  TimerMap::const_iterator it = m_timers.find(index);
  if (it != m_timers.end())
    return it->second;

  return cTimer::EmptyTimer;
}

TimerVector cTimerManager::GetTimers(void) const
{
  TimerVector timers;

  CLockObject lock(m_mutex);

  for (TimerMap::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    timers.push_back(it->second);

  return timers;
}

size_t cTimerManager::TimerCount(void) const
{
  CLockObject lock(m_mutex);
  return m_timers.size();
}

bool cTimerManager::AddTimer(const TimerPtr& newTimer)
{
  if (newTimer && newTimer->Index() == TIMER_INVALID_INDEX && newTimer->IsValid())
  {
    CLockObject lock(m_mutex);

    if (TimerConflicts(*newTimer))
      return false;

    newTimer->SetIndex(++m_maxIndex);
    newTimer->RegisterObserver(this);
    m_timers.insert(std::make_pair(newTimer->Index(), newTimer));

    SetChanged();

    return true;
  }

  return false;
}

bool cTimerManager::UpdateTimer(unsigned int index, const cTimer& updatedTimer)
{
  CLockObject lock(m_mutex);

  TimerPtr timer = GetByIndex(index);
  if (timer)
  {
    // If timer is being enabled, check for conflicts
    if (!timer->IsActive() && updatedTimer.IsActive() && TimerConflicts(updatedTimer))
      return false;

    *timer = updatedTimer;
    timer->NotifyObservers();
    return true;
  }

  return false;
}

bool cTimerManager::RemoveTimer(unsigned int index, bool bInterruptRecording)
{
  CLockObject lock(m_mutex);

  TimerMap::iterator it = m_timers.find(index);
  if (it != m_timers.end())
  {
    bool bRecording = it->second->IsRecording();
    if (bRecording && !bInterruptRecording)
      return false;

    it->second->UnregisterObserver(this);
    m_timers.erase(it);

    SetChanged();

    return true;
  }

  return false;
}

bool cTimerManager::TimerConflicts(const cTimer& timer) const
{
  // TODO: include lifetime, weekday mask
  for (TimerMap::const_iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    const TimerPtr& existingTimer = it->second;

    // Timer can't conflict with itself
    if (&timer == existingTimer.get())
      continue;

    // Skip inactive timers
    if (!existingTimer->IsActive())
      continue;

    // Case 1: timer ends before existing timer starts; doesn't conflict
    if (timer.EndTime() <= existingTimer->StartTime())
      continue;

    // Case 2: timer starts after existing timer ends; doesn't conflict
    if (timer.StartTime() >= existingTimer->EndTime())
      continue;

    // Conflict detected
    return true;
  }

  return false;
}

void cTimerManager::Notify(const Observable &obs, const ObservableMessage msg)
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

bool cTimerManager::LoadTimers(const std::string &file)
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

  const TiXmlNode *timerNode = root->FirstChild(TIMER_XML_ELM_TIMER);
  while (timerNode != NULL)
  {
    TimerPtr timer = TimerPtr(new cTimer);

    if (timer->DeserialiseTimer(timerNode) && !GetByIndex(timer->Index()))
    {
      if (timer->Index() > m_maxIndex)
        m_maxIndex = timer->Index();

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

bool cTimerManager::SaveTimers(const string &file /* = ""*/)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(TIMER_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  CLockObject lock(m_mutex);

  for (TimerMap::const_iterator itTimer = m_timers.begin(); itTimer != m_timers.end(); ++itTimer)
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

}
