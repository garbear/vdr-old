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

#include "Schedule.h"
#include "EPGDefinitions.h"
#include "Event.h"
#include "channels/Channel.h"
#include "channels/ChannelManager.h"
#include "settings/Settings.h"
#include "timers/Timers.h"
#include "utils/log/Log.h"
#include "utils/XBMCTinyXML.h"
#include "Config.h"

#include <algorithm>

using namespace std;

namespace VDR
{

#define RUNNINGSTATUSTIMEOUT 30 // seconds before the running status is considered unknown

cSchedule::cSchedule(const cChannelID& channelID)
 : m_channelID(channelID)/*,
   m_bHasRunning(false)*/
{
}

cSchedule::~cSchedule(void)
{
  for (map<unsigned int, EventPtr>::iterator itPair = m_eventIds.begin(); itPair != m_eventIds.end(); ++itPair)
    itPair->second->UnregisterObserver(this);
}

EventVector cSchedule::Events(void) const
{
  EventVector events;

  for (map<unsigned int, EventPtr>::const_iterator itPair = m_eventIds.begin(); itPair != m_eventIds.end(); ++itPair)
    events.push_back(itPair->second);

  return events;
}

EventPtr cSchedule::GetEvent(unsigned int eventID) const
{
  if (m_eventIds.find(eventID) != m_eventIds.end())
    return m_eventIds.find(eventID)->second;

  return cEvent::EmptyEvent;
}

EventPtr cSchedule::GetEvent(const CDateTime& startTime) const
{
  if (startTime.IsValid()) // 'StartTime < 0' is apparently used with NVOD channels
  {
    for (map<unsigned int, EventPtr>::const_iterator itPair = m_eventIds.begin(); itPair != m_eventIds.end(); ++itPair)
    {
      const EventPtr& event = itPair->second;
      if (event->StartTime() == startTime)
        return event;
    }
  }

  return cEvent::EmptyEvent;
}

void cSchedule::AddEvent(const EventPtr& event)
{
  EventPtr existingEvent = GetEvent(event->ID());

  if (existingEvent)
    *existingEvent = *event;
  else
  {
    event->RegisterObserver(this);
    m_eventIds[event->ID()] = event;
    SetChanged();
  }
}

void cSchedule::DeleteEvent(unsigned int eventID)
{
  if (m_eventIds.find(eventID) != m_eventIds.end())
  {
    m_eventIds[eventID]->UnregisterObserver(this);
    m_eventIds.erase(eventID);
    SetChanged();
  }
}

/*
void cSchedule::SetRunningStatus(const EventPtr& event, int RunningStatus, cChannel *Channel)
{
  m_bHasRunning = false;
  for (EventVector::iterator it = m_events.begin(); it != m_events.end(); ++it)
  {
    if (*it == event)
    {
      if ((*it)->RunningStatus() > SI::RunningStatusNotRunning  ||
                   RunningStatus > SI::RunningStatusNotRunning)
      {
        (*it)->SetRunningStatus(RunningStatus, Channel);
        break;
      }
    }
    else if (RunningStatus >= SI::RunningStatusPausing &&
        (*it)->StartTime() < event->StartTime())
    {
      (*it)->SetRunningStatus(SI::RunningStatusNotRunning);
    }

    if ((*it)->RunningStatus() >= SI::RunningStatusPausing)
      hasRunning = true;
  }
}

void cSchedule::ClrRunningStatus(cChannel *Channel)
{
  if (hasRunning)
  {
    for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
      if ((*it)->RunningStatus() >= SI::RunningStatusPausing)
      {
        (*it)->SetRunningStatus(SI::RunningStatusNotRunning, Channel);
        hasRunning = false;
        break;
      }
    }
  }
}

void cSchedule::ResetVersions(void)
{
  for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
    (*it)->SetVersion(0xFF);
}

void cSchedule::Sort(void)
{
  // TODO: Should we be sorting by pointer?
  //events.Sort();
  std::sort(m_events.begin(), m_events.end());

  // Make sure there are no RunningStatusUndefined before the currently running event:
  if (hasRunning)
  {
    for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
      if ((*it)->RunningStatus() >= SI::RunningStatusPausing)
        break;
      (*it)->SetRunningStatus(SI::RunningStatusNotRunning);
    }
  }
}

void cSchedule::DropOutdated(const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uint8_t TableID, uint8_t Version)
{
  if (SegmentStart.IsValid() && SegmentEnd.IsValid())
  {
    for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
      const EventPtr& event = *it;
      if (event->EndTime() > SegmentStart)
      {
        if (event->StartTime() < SegmentEnd)
        {
          // The event overlaps with the given time segment.
          if (event->TableID() > TableID ||
              (event->TableID() == TableID && event->Version() != Version))
          {
            // The segment overwrites all events from tables with higher ids, and
            // within the same table id all events must have the same version.
            // We can't delete the event right here because a timer might have
            // a pointer to it, so let's set its id and start time to 0 to have it
            // "phased out":
            if (hasRunning && event->IsRunning())
              ClrRunningStatus();
            UnhashEvent(event.get());
            event->eventID = 0;
            event->m_startTime.Reset();
          }
        }
        else
          break;
      }
    }
  }
}

void cSchedule::Cleanup(void)
{
  Cleanup(CDateTime::GetUTCDateTime());
}

void cSchedule::Cleanup(const CDateTime& Time)
{
  for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); )
  {
    const EventPtr& event = *it;

    static const CDateTimeSpan oneHour(0, 1, 0, 0); // Add one hour for safety
    if (!cTimers::Get().HasTimer(event) &&
        (!Time.IsValid() ||
            (event->EndTime() + CDateTimeSpan(0, 0, cSettings::Get().m_iEPGLinger, 0) + oneHour) < Time))
    {
      modified = CDateTime::GetCurrentDateTime();
      // TODO: Need a safer way to delete while iterating!
      DelEvent(event);
    }
    else
      ++it;
  }
}
*/

void cSchedule::Notify(const Observable &obs, const ObservableMessage msg)
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

void cSchedule::NotifyObservers(void)
{
  for (map<unsigned int, EventPtr>::iterator itPair = m_eventIds.begin(); itPair != m_eventIds.end(); ++itPair)
    itPair->second->NotifyObservers(ObservableMessageEventChanged);

  if (Changed())
  {
    Observable::NotifyObservers(ObservableMessageEventChanged);
    Save();
  }
}

bool cSchedule::Load(void)
{
  assert(!cSettings::Get().m_EPGDirectory.empty());

  CXBMCTinyXML xmlDoc;
  std::string strFilename = cSettings::Get().m_EPGDirectory + "/epg_" + m_channelID.ToString() + ".xml";
  if (!xmlDoc.LoadFile(strFilename.c_str()))
  {
    esyslog("failed to open '%s'", strFilename.c_str());
    return false;
  }

  TiXmlElement* root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), EPG_XML_ELM_SCHEDULE))
  {
    esyslog("failed to find root element in '%s'", strFilename.c_str());
    return false;
  }

  map<unsigned int, EventPtr> events;

  const TiXmlNode* eventNode = root->FirstChild(EPG_XML_ELM_EVENT);
  while (eventNode != NULL)
  {
    EventPtr event;

    if (cEvent::Deserialise(event, eventNode))
      events[event->ID()] = event;

    eventNode = eventNode->NextSibling(EPG_XML_ELM_EVENT);
  }

  m_eventIds = events;

  return true;
}

bool cSchedule::Save(void) const
{
  assert(!cSettings::Get().m_EPGDirectory.empty());

  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  try
  {
    TiXmlElement rootElement(EPG_XML_ELM_SCHEDULE);
    TiXmlNode* root = xmlDoc.InsertEndChild(rootElement);
    if (root == NULL)
      throw false;

    TiXmlElement* epgElement = root->ToElement();
    if (!epgElement)
      throw false;

    if (!m_channelID.Serialise(root))
      throw false;

    for (map<unsigned int, EventPtr>::const_iterator itPair = m_eventIds.begin(); itPair != m_eventIds.end(); ++itPair)
    {
      TiXmlElement eventElement(EPG_XML_ELM_EVENT);
      TiXmlNode* eventNode = root->InsertEndChild(eventElement);
      if (!eventNode)
        throw false;

      if (!itPair->second->Serialise(eventNode))
        throw false;
    }
  }
  catch (const bool& bSuccess)
  {
    esyslog("Failed to save schedule for channel %s", m_channelID.ToString().c_str());
    //delete decl; // TODO: Do we need to delete decl?
    return bSuccess;
  }

  std::string strFilename = cSettings::Get().m_EPGDirectory + "/epg_" + m_channelID.ToString() + ".xml";
  if (!xmlDoc.SafeSaveFile(strFilename))
  {
    esyslog("failed to save the EPG data: could not write to '%s'", strFilename.c_str());
    return false;
  }

  ChannelPtr channel = cChannelManager::Get().GetByChannelID(ChannelID());
  if (channel)
    dsyslog("EPG for channel '%s' saved", channel->Name().c_str());

  return true;
}

}
