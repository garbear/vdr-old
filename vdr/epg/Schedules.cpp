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

#include "Schedules.h"
#include "EPGDefinitions.h"
#include "Event.h"
#include "channels/ChannelFilter.h"
#include "channels/ChannelManager.h"
#include "lib/platform/threads/threads.h"
#include "settings/Settings.h"
#include "timers/Timers.h"
#include "utils/CalendarUtils.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/UTF8Utils.h"
#include "utils/XBMCTinyXML.h"

#include "libsi/si.h"

#include <ctype.h>
#include <limits.h>
#include <time.h>

namespace VDR
{

#define EPGDATAWRITEDELTA   600 // seconds between writing the epg.data file

// --- cSchedulesLock --------------------------------------------------------

const SchedulePtr cSchedules::EmptySchedule;

cSchedulesLock::cSchedulesLock(bool WriteLock, int TimeoutMs)
{
  m_bLocked = cSchedules::Get().m_rwlock.Lock(WriteLock, TimeoutMs);
}

cSchedulesLock::~cSchedulesLock()
{
  if (m_bLocked)
    cSchedules::Get().m_rwlock.Unlock();
}

cSchedules* cSchedulesLock::Get(void) const
{
  return m_bLocked ? &cSchedules::Get() : NULL;
}

EventPtr cSchedulesLock::GetEvent(const cChannelID& channelId, tEventID eventId)
{
  EventPtr event;

  SchedulePtr schedule;

  cSchedulesLock schedulesLock(true, 10);
  cSchedules* schedules = schedulesLock.Get();

  if (schedules)
    schedule = schedules->GetSchedule(channelId);
  else
  {
    // If we don't get a write lock, let's at least get a read lock, so
    // that we can set the running status and 'seen' timestamp (well, actually
    // with a read lock we shouldn't be doing that, but it's only integers that
    // get changed, so it should be ok)
    cSchedulesLock schedulesLock;
    cSchedules* schedules = schedulesLock.Get();
    if (schedules)
      schedule = schedules->GetSchedule(channelId);
  }

  if (schedule)
    event = schedule->GetEvent(eventId);

  return event;
}

bool cSchedulesLock::AddEvent(const cChannelID& channelId, const EventPtr& event)
{
  cSchedulesLock schedulesLock(true, 10);
  cSchedules* schedules = schedulesLock.Get();

  if (schedules)
  {
    SchedulePtr schedule = schedules->AddSchedule(channelId);
    schedule->AddEvent(event);
    schedule->SetModified();
    return true;
  }
  else
  {
    //XXX fix this mess!
    dsyslog("failed to get a schedules lock while adding events");
  }
  return false;
}

// --- cSchedules ------------------------------------------------------------

cSchedules::cSchedules(void)
{
  m_bHasUnsavedData = false;
}

cSchedules::~cSchedules(void)
{
}

cSchedules& cSchedules::Get(void)
{
  static cSchedules _instance;
  return _instance;
}

CDateTime cSchedules::Modified(void)
{
  cSchedulesLock lock;
  cSchedules* schedules = lock.Get();
  CDateTime retval;
  if (schedules)
    retval = schedules->m_modified;

  return retval;
}

void cSchedules::SetModified(SchedulePtr Schedule)
{
  Schedule->SetModified();
  m_modified = CDateTime::GetUTCDateTime();
}

void cSchedules::ResetVersions(void)
{
  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->ResetVersions();
}

bool cSchedules::ClearAll(void)
{
  cTimers::Get().ClearEvents();
  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->Cleanup(CDateTime());
  m_bHasUnsavedData = false;
  return true;
}

void cSchedules::CleanTables(void)
{
  CDateTime now = CDateTime::GetUTCDateTime();
  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->Cleanup(now);
}

bool cSchedules::Save(void)
{
  assert(!cSettings::Get().m_EPGDirectory.empty());
  bool bReturn(true);

  if (!m_bHasUnsavedData)
  {
    for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
      (*it)->Save();
    return true;
  }

  isyslog("saving EPG data to '%s'", cSettings::Get().m_EPGDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(EPG_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    if ((*it)->Save())
    {
      TiXmlElement tableElement(EPG_XML_ELM_TABLE);
      TiXmlNode* textNode = root->InsertEndChild(tableElement);
      if (textNode)
        (*it)->ChannelID().Serialise(textNode);
    }
    else
    {
      bReturn = false;
    }
  }

  if (bReturn)
  {
    std::string strFilename = cSettings::Get().m_EPGDirectory + "/epg.xml";
    if (!xmlDoc.SafeSaveFile(strFilename))
    {
      esyslog("failed to save the EPG data: could not write to '%s'", strFilename.c_str());
      return false;
    }
    else
    {
      m_bHasUnsavedData = false;
    }
  }

  return bReturn;
}

bool cSchedules::Read(void)
{
  assert(!cSettings::Get().m_EPGDirectory.empty());

  isyslog("reading EPG data from '%s'", cSettings::Get().m_EPGDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  std::string strFilename = cSettings::Get().m_EPGDirectory + "/epg.xml";
  if (!xmlDoc.LoadFile(strFilename.c_str()))
  {
    esyslog("failed to open '%s'", strFilename.c_str());
    return false;
  }

  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), EPG_XML_ROOT))
  {
    esyslog("failed to find root element in '%s'", strFilename.c_str());
    return false;
  }

  const TiXmlNode *tableNode = root->FirstChild(EPG_XML_ELM_TABLE);
  while (tableNode != NULL)
  {
    cChannelID channelId;
    channelId.Deserialise(tableNode);

    SchedulePtr schedule = AddSchedule(channelId);
    if (schedule)
    {
      if (schedule->Read())
      {
        SetModified(schedule);
        schedule->SetSaved();
      }
      else
      {
        DelSchedule(schedule);
      }
    }
    tableNode = tableNode->NextSibling(EPG_XML_ELM_TABLE);
  }

  return true;
}

void cSchedules::DelSchedule(SchedulePtr schedule)
{
  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    if ((*it)->ChannelID() == schedule->ChannelID())
    {
      m_bHasUnsavedData = true;
      m_schedules.erase(it);
      return;
    }
  }
}

SchedulePtr cSchedules::AddSchedule(const cChannelID& ChannelID)
{
  SchedulePtr schedule = GetSchedule(ChannelID);
  if (!schedule)
  {
    schedule = SchedulePtr(new cSchedule(ChannelID));
    m_schedules.push_back(schedule);
    ChannelPtr channel = cChannelManager::Get().GetByChannelID(ChannelID);
    if (channel)
      channel->SetSchedule(schedule);
    dsyslog("created new EPG table for channel %u-%u-%u", ChannelID.Nid(), ChannelID.Sid(), ChannelID.Tsid());
    m_bHasUnsavedData = true;
  }
  return schedule;
}

SchedulePtr cSchedules::GetSchedule(const cChannelID& ChannelID)
{
  for (ScheduleVector::const_iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    const cChannelID& checkId = (*it)->ChannelID();
    if (checkId == ChannelID)
      return *it;
  }
  return EmptySchedule;
}

SchedulePtr cSchedules::GetSchedule(ChannelPtr channel, bool bAddIfMissing)
{
  assert(channel);

  SchedulePtr schedule;

  if (!channel->HasSchedule() && bAddIfMissing)
  {
    schedule = GetSchedule(channel->ID());
    if (!schedule)
      schedule = AddSchedule(channel->ID());
    channel->SetSchedule(schedule);
  }

  if (!schedule && channel->HasSchedule())
    schedule = channel->Schedule();

  return schedule;
}

ScheduleVector cSchedules::GetUpdatedSchedules(const std::map<int, CDateTime>& lastUpdated, CChannelFilter& filter)
{
  ScheduleVector retval;
  std::map<int, CDateTime>::iterator it;
  std::map<int, CDateTime>::const_iterator previousIt;
  for (ScheduleVector::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    const EventVector& events = (*it)->Events();
    EventPtr lastEvent;
    if (!events.empty())
      lastEvent = events[events.size() - 1];
    if (!lastEvent)
      continue;

    const ChannelPtr channel = cChannelManager::Get().GetByChannelID((*it)->ChannelID());
    if (!channel)
      continue;

    if (!filter.PassFilter(channel))
      continue;

    uint32_t channelId = (*it)->ChannelID().Hash();
    previousIt = lastUpdated.find(channelId);
    if (previousIt != lastUpdated.end() && previousIt->second >= lastEvent->StartTime())
      continue;

    retval.push_back(*it);
  }

  return retval;
}

}
