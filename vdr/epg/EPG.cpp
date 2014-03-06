/*
 * epg.c: Electronic Program Guide
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version (as used in VDR before 1.3.0) written by
 * Robert Schneider <Robert.Schneider@web.de> and Rolf Hakenes <hakenes@hippomi.de>.
 *
 * $Id: epg.c 2.23.1.1 2013/09/01 09:16:53 kls Exp $
 */

#include "EPG.h"
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include "libsi/si.h"
#include "channels/ChannelFilter.h"
#include "timers/Timers.h"
#include "EPGDefinitions.h"
#include "platform/threads/threads.h"
#include "utils/XBMCTinyXML.h"

#include "vdr/utils/CalendarUtils.h"
#include "vdr/utils/UTF8Utils.h"

#define EPGDATAWRITEDELTA   600 // seconds between writing the epg.data file

// --- cSchedulesLock --------------------------------------------------------

SchedulePtr cSchedules::EmptySchedule = SchedulePtr();

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
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->ResetVersions();
}

bool cSchedules::ClearAll(void)
{
  cTimers::Get().ClearEvents();
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->Cleanup(CDateTime());
  m_bHasUnsavedData = false;
  return true;
}

void cSchedules::CleanTables(void)
{
  CDateTime now = CDateTime::GetUTCDateTime();
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->Cleanup(now);
}

bool cSchedules::Save(void)
{
  assert(!g_setup.EPGDirectory.empty());
  bool bReturn(true);

  if (!m_bHasUnsavedData)
  {
    for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
      (*it)->Save();
    return true;
  }

  isyslog("saving EPG data to '%s'", g_setup.EPGDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(EPG_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    if ((*it)->Save())
    {
      TiXmlElement tableElement(EPG_XML_ELM_TABLE);
      TiXmlNode* textNode = root->InsertEndChild(tableElement);
      if (textNode)
      {
        TiXmlText* text = new TiXmlText(StringUtils::Format("%s", (*it)->ChannelID().Serialize().c_str()));
        textNode->LinkEndChild(text);
      }
    }
    else
    {
      bReturn = false;
    }
  }

  if (bReturn)
  {
    std::string strFilename = g_setup.EPGDirectory + "/epg.xml";
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
  assert(!g_setup.EPGDirectory.empty());

  isyslog("reading EPG data from '%s'", g_setup.EPGDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  std::string strFilename = g_setup.EPGDirectory + "/epg.xml";
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
    const TiXmlElement *tableElem = tableNode->ToElement();
    SchedulePtr schedule = AddSchedule(tChannelID::Deserialize(tableElem->GetText()));
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
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    if ((*it)->ChannelID() == schedule->ChannelID())
    {
      m_bHasUnsavedData = true;
      m_schedules.erase(it);
      return;
    }
  }
}

SchedulePtr cSchedules::AddSchedule(const tChannelID& ChannelID)
{
  tChannelID id(ChannelID);
  id.ClrRid();
  SchedulePtr schedule = GetSchedule(id);
  if (!schedule)
  {
    schedule = SchedulePtr(new cSchedule(id));
    m_schedules.push_back(schedule);
    ChannelPtr channel = cChannelManager::Get().GetByChannelID(id);
    if (channel)
      channel->SetSchedule(schedule);
    m_bHasUnsavedData = true;
  }
  return schedule;
}

SchedulePtr cSchedules::GetSchedule(const tChannelID& ChannelID)
{
  tChannelID id(ChannelID);
  tChannelID checkId;
  id.ClrRid();
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    checkId = (*it)->ChannelID();
    checkId.ClrRid();
    if (checkId == id)
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
    schedule = GetSchedule(channel->GetChannelID());
    if (!schedule)
      schedule = AddSchedule(channel->GetChannelID());
    channel->SetSchedule(schedule);
  }

  if (!schedule && channel->HasSchedule())
    schedule = channel->Schedule();

  return schedule;
}

std::vector<SchedulePtr> cSchedules::GetUpdatedSchedules(const std::map<int, CDateTime>& lastUpdated, CChannelFilter& filter)
{
  std::vector<SchedulePtr> retval;
  std::map<int, CDateTime>::iterator it;
  std::map<int, CDateTime>::const_iterator previousIt;
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    cEvent *lastEvent = (*it)->Events()->Last();
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
