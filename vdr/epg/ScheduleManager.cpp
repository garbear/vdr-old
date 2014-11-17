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

#include "ScheduleManager.h"
#include "EPGDefinitions.h"
#include "Event.h"
#include "channels/ChannelFilter.h"
#include "channels/ChannelManager.h"
#include "settings/Settings.h"
#include "transponders/Transponder.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

using namespace PLATFORM;
using namespace std;

namespace VDR
{

cScheduleManager& cScheduleManager::Get(void)
{
  static cScheduleManager _instance;
  return _instance;
}

cScheduleManager::~cScheduleManager(void)
{
  for (map<cChannelID, SchedulePtr>::iterator itPair = m_schedules.begin(); itPair != m_schedules.end(); ++itPair)
    itPair->second->UnregisterObserver(this);
}

void cScheduleManager::AddEvent(const EventPtr& event, const cTransponder& transponder)
{
  cChannelID channelId = event->ChannelID();

  if (!channelId.IsValid() && event->AtscSourceID() != 0)
  {
    ChannelPtr channel = cChannelManager::Get().GetByFrequencyAndATSCSourceId(transponder.FrequencyHz(),event->AtscSourceID());
    if (channel)
      channelId = channel->ID();
    else
    {
      dsyslog("failed to find channel for event - freq=%u source=%u",transponder.FrequencyHz(), event->AtscSourceID());
      return;
    }
  }

  CLockObject lock(m_mutex);

  if (m_schedules.find(channelId) == m_schedules.end())
  {
    SchedulePtr schedule = SchedulePtr(new cSchedule(channelId));
    schedule->RegisterObserver(this);
    m_schedules[channelId] = schedule;
  }

  m_schedules[channelId]->AddEvent(event);
}

EventPtr cScheduleManager::GetEvent(const cChannelID& channelId, unsigned int eventId) const
{
  CLockObject lock(m_mutex);

  if (m_schedules.find(channelId) != m_schedules.end())
  {
    const SchedulePtr& schedule = m_schedules.find(channelId)->second;
    return schedule->GetEvent(eventId);
  }

  return cEvent::EmptyEvent;
}

/*
void cScheduleManager::ResetVersions(void)
{
  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->ResetVersions();
}

bool cScheduleManager::ClearAll(void)
{
  cTimers::Get().ClearEvents();
  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->Cleanup(CDateTime());
  m_bHasUnsavedData = false;
  return true;
}

void cScheduleManager::CleanTables(void)
{
  CDateTime now = CDateTime::GetUTCDateTime();
  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->Cleanup(now);
}
*/

EventVector cScheduleManager::GetEvents(const cChannelID& channelID) const
{
  EventVector events;

  map<cChannelID, SchedulePtr>::const_iterator itPair = m_schedules.find(channelID);
  if (itPair != m_schedules.end())
    events = itPair->second->Events();

  return events;
}

vector<cChannelID> cScheduleManager::GetUpdatedChannels(const std::map<int, CDateTime>& lastUpdated, CChannelFilter& filter) const
{
  vector<cChannelID> retval;
  std::map<int, CDateTime>::iterator it;
  std::map<int, CDateTime>::const_iterator previousIt;
  for (map<cChannelID, SchedulePtr>::const_iterator itPair = m_schedules.begin(); itPair != m_schedules.end(); ++itPair)
  {
    const SchedulePtr& schedule = itPair->second;

    /* TODO
    const EventVector& events = (*itSchedule)->Events();
    EventPtr lastEvent;
    if (!events.empty())
      lastEvent = events[events.size() - 1];
    if (!lastEvent)
      continue;
    */

    const ChannelPtr& channel = cChannelManager::Get().GetByChannelID(schedule->ChannelID());
    if (!channel || !filter.PassFilter(channel))
      continue;

    /* TODO
    uint32_t channelId = (*itSchedule)->ChannelID().Hash();
    previousIt = lastUpdated.find(channelId);
    if (previousIt != lastUpdated.end() && previousIt->second >= lastEvent->StartTime())
      continue;
    */

    retval.push_back(schedule->ChannelID());
  }

  return retval;
}

void cScheduleManager::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageEventChanged:
    SetChanged();
    break;
  default:
    break;
  }
}

void cScheduleManager::NotifyObservers(void)
{
  CLockObject lock(m_mutex);

  for (map<cChannelID, SchedulePtr>::iterator itPair = m_schedules.begin(); itPair != m_schedules.end(); ++itPair)
    itPair->second->NotifyObservers();

  if (Changed())
  {
    Observable::NotifyObservers(ObservableMessageEventChanged);
    Save();
  }
}

bool cScheduleManager::Load(void)
{
  assert(!cSettings::Get().m_EPGDirectory.empty());

  isyslog("Reading EPG data from '%s'", cSettings::Get().m_EPGDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  std::string strFilename = cSettings::Get().m_EPGDirectory + "/epg.xml";
  if (!xmlDoc.LoadFile(strFilename.c_str()))
  {
    esyslog("Failed to open '%s'", strFilename.c_str());
    return false;
  }

  TiXmlElement* root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), EPG_XML_ROOT))
  {
    esyslog("Failed to find root element in '%s'", strFilename.c_str());
    return false;
  }

  map<cChannelID, SchedulePtr> schedules;
  const TiXmlNode* scheduleNode = root->FirstChild(EPG_XML_ELM_SCHEDULE);
  while (scheduleNode != NULL)
  {
    cChannelID channelId;
    if (!channelId.Deserialise(scheduleNode))
      return false;

    SchedulePtr schedule = SchedulePtr(new cSchedule(channelId));
    if (!schedule->Load())
      return false;
    schedules[channelId] = schedule;

    scheduleNode = scheduleNode->NextSibling(EPG_XML_ELM_SCHEDULE);
  }

  CLockObject lock(m_mutex);
  m_schedules = schedules;
  return true;
}

bool cScheduleManager::Save(void) const
{
  assert(!cSettings::Get().m_EPGDirectory.empty());
  bool bReturn(true);

  isyslog("Saving EPG data to '%s'", cSettings::Get().m_EPGDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(EPG_XML_ROOT);
  TiXmlNode* root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  for (map<cChannelID, SchedulePtr>::const_iterator itPair = m_schedules.begin(); itPair != m_schedules.end(); ++itPair)
  {
    TiXmlElement scheduleElement(EPG_XML_ELM_SCHEDULE);
    TiXmlNode* textNode = root->InsertEndChild(scheduleElement);
    if (textNode)
      itPair->second->ChannelID().Serialise(textNode);
  }

  if (bReturn)
  {
    std::string strFilename = cSettings::Get().m_EPGDirectory + "/epg.xml";
    if (!xmlDoc.SafeSaveFile(strFilename))
    {
      esyslog("Failed to save the EPG data: could not write to '%s'", strFilename.c_str());
      bReturn = false;
    }
  }

  return bReturn;
}

}
