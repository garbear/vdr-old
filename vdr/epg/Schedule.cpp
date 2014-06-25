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
#include "Components.h"
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

namespace VDR
{

#define RUNNINGSTATUSTIMEOUT 30 // seconds before the running status is considered unknown

cSchedule::cSchedule(const cChannelID& ChannelID)
{
  channelID   = ChannelID;
  hasRunning  = false;
}

void cSchedule::AddEvent(const EventPtr& event)
{
  EventPtr existingEvent = GetEvent(event->EventID(), event->StartTime());
  if (existingEvent)
  {
    // We have found an existing event, either through its event ID or its start time.
    //*existingEvent = *event;
    existingEvent->SetSeen();
    existingEvent->SetEventID(event->EventID());
    existingEvent->SetStartTime(event->StartTime());
    existingEvent->SetDuration(event->Duration());
    existingEvent->SetVersion(event->Version());
    existingEvent->SetTitle(event->Title());
    existingEvent->SetShortText(event->ShortText());
    existingEvent->SetDescription(event->Description());
    existingEvent->SetTableID(event->TableID());
    existingEvent->SetComponents(const_cast<CEpgComponents*>(event->Components()));
    existingEvent->FixEpgBugs();
  }
  else
  {
    event->schedule = this;
    m_events.push_back(event);
    HashEvent(event.get());
  }
}

void cSchedule::DelEvent(const EventPtr& event)
{
  if (event->schedule == this)
  {
    if (hasRunning && event->IsRunning())
      ClrRunningStatus();
    UnhashEvent(event.get());
    EventVector::iterator it = std::find(m_events.begin(), m_events.end(), event);
    if (it != m_events.end())
      m_events.erase(it);
  }
}

void cSchedule::HashEvent(cEvent *Event)
{
  eventsHashID.Add(Event, Event->EventID());
  if (Event->StartTime().IsValid()) // 'StartTime < 0' is apparently used with NVOD channels
    eventsHashStartTime.Add(Event, Event->StartTimeAsTime());
}

void cSchedule::UnhashEvent(cEvent *Event)
{
  eventsHashID.Del(Event, Event->EventID());
  if (Event->StartTime().IsValid()) // 'StartTime < 0' is apparently used with NVOD channels
    eventsHashStartTime.Del(Event, Event->StartTimeAsTime());
}

EventPtr cSchedule::GetPresentEvent(void) const
{
  EventPtr presentEvent;
  CDateTime now = CDateTime::GetUTCDateTime();
  for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
  {
    const EventPtr& event = *it;

    if (event->StartTime() <= now)
      presentEvent = event;
    else if (event->StartTime() > now + CDateTimeSpan(0, 1, 0, 0))
      break;

    if (event->SeenWithin(RUNNINGSTATUSTIMEOUT) &&
          event->RunningStatus() >= SI::RunningStatusPausing)
    {
      presentEvent = event;
      break;
    }
  }
  return presentEvent;
}

EventPtr cSchedule::GetFollowingEvent(void) const
{
  EventPtr event = GetPresentEvent();

  if (event)
  {
    // Get the next event
    for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
      if (event == *it && (it + 1) != m_events.end())
      {
        event = *(it + 1);
        break;
      }
    }
  }
  else
  {
    CDateTime now = CDateTime::GetUTCDateTime();
    for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
      if ((*it)->StartTime() >= now)
      {
        event = *it;
        break;
      }
    }
  }
  return event;
}

EventPtr cSchedule::GetEvent(tEventID EventID, CDateTime StartTime /* = CDateTime::GetCurrentDateTime() */)
{
  EventPtr event;

  // Returns the event info with the given StartTime or, if no actual StartTime
  // is given, the one with the given EventID.
  cEvent* eventRawPtr = NULL;
  if (StartTime.IsValid()) // 'StartTime < 0' is apparently used with NVOD channels
  {
    time_t tmStart;
    StartTime.GetAsTime(tmStart);
    eventRawPtr = eventsHashStartTime.Get(tmStart);
  }
  else
    eventRawPtr = eventsHashID.Get(EventID);

  if (eventRawPtr)
  {
    // Look for corresponding shared pointer
    for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
      if (eventRawPtr == it->get())
      {
        event = *it;
        break;
      }
    }
  }

  return event;
}

void cSchedule::SetRunningStatus(const EventPtr& event, int RunningStatus, cChannel *Channel)
{
  hasRunning = false;
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

bool cSchedule::Read(void)
{
  assert(!cSettings::Get().m_EPGDirectory.empty());

  CXBMCTinyXML xmlDoc;
  std::string strFilename = cSettings::Get().m_EPGDirectory + "/epg_" + channelID.ToString() + ".xml";
  if (!xmlDoc.LoadFile(strFilename.c_str()))
  {
    esyslog("failed to open '%s'", strFilename.c_str());
    return false;
  }

  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), EPG_XML_ELM_TABLE))
  {
    esyslog("failed to find root element in '%s'", strFilename.c_str());
    return false;
  }

  const TiXmlNode *eventNode = root->FirstChild(EPG_XML_ELM_EVENT);
  while (eventNode != NULL)
  {
    if (!cEvent::Deserialise(this, eventNode))
      break;
    eventNode = eventNode->NextSibling(EPG_XML_ELM_EVENT);
  }

  Sort();

  return true;
}

bool cSchedule::Save(void)
{
  assert(!cSettings::Get().m_EPGDirectory.empty());

  Cleanup();

  if (saved == modified)
    return true;

  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(EPG_XML_ELM_TABLE);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  if (!Serialise(root))
  {
    delete decl;
    return false;
  }

  std::string strFilename = cSettings::Get().m_EPGDirectory + "/epg_" + channelID.ToString() + ".xml";
  if (!xmlDoc.SafeSaveFile(strFilename))
  {
    esyslog("failed to save the EPG data: could not write to '%s'", strFilename.c_str());
    return false;
  }

  saved = modified;

  ChannelPtr channel = cChannelManager::Get().GetByChannelID(ChannelID());
  if (channel)
    dsyslog("EPG for channel '%s' saved", channel->Name().c_str());
  return true;
}

bool cSchedule::Serialise(TiXmlNode *node) const
{
  if (node == NULL)
    return false;

  TiXmlElement* epgElement = node->ToElement();
  if (!epgElement)
    return false;

  if (!channelID.Serialise(node))
    return false;

  for (EventVector::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
  {
    if (!(*it)->Serialise(epgElement))
      return false;
  }

  return true;
}

}
