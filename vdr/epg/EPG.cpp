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
#include "EPGDataWriter.h"
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include "libsi/si.h"
#include "channels/ChannelFilter.h"
#include "recordings/Timers.h"
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
  m_lastDump = time(NULL);
  m_modified = 0;
}

cSchedules::~cSchedules(void)
{
}

cSchedules& cSchedules::Get(void)
{
  static cSchedules _instance;
  return _instance;
}

time_t cSchedules::Modified(void)
{
  cSchedulesLock lock(true);
  cSchedules* schedules = lock.Get();
  return schedules ? schedules->m_modified : 0;
}

void cSchedules::SetModified(SchedulePtr Schedule)
{
  Schedule->SetModified();
  m_modified = time(NULL);
}

void cSchedules::Cleanup(bool Force)
{
  if (Force)
    m_lastDump = 0;
  time_t now = time(NULL);
  if (now - m_lastDump > EPGDATAWRITEDELTA)
  {
    if (Force)
      cEpgDataWriter::Get().Perform();
    else if (!cEpgDataWriter::Get().IsRunning())
      cEpgDataWriter::Get().CreateThread();
    m_lastDump = now;
  }
}

void cSchedules::ResetVersions(void)
{
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->ResetVersions();
}

bool cSchedules::ClearAll(void)
{
  for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer))
    Timer->SetEvent(NULL);
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->Cleanup(INT_MAX);
  return true;
}

void cSchedules::CleanTables(void)
{
  time_t now = time(NULL);
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
    (*it)->Cleanup(now);
}

bool cSchedules::Save(void)
{
  assert(!Setup.EPGDirectory.empty());
  bool bReturn(true);

  isyslog("saving EPG data to '%s'", Setup.EPGDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(EPG_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    if ((*it)->Save(Setup.EPGDirectory))
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
    std::string strFilename = Setup.EPGDirectory + "/epg.xml";
    if (!xmlDoc.SafeSaveFile(strFilename))
    {
      esyslog("failed to save the EPG data: could not write to '%s'", strFilename.c_str());
      return false;
    }
  }

  return bReturn;
}

bool cSchedules::Read(void)
{
  assert(!Setup.EPGDirectory.empty());

  isyslog("reading EPG data from '%s'", Setup.EPGDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  std::string strFilename = Setup.EPGDirectory + "/epg.xml";
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
      if (schedule->Read(Setup.EPGDirectory))
        SetModified(schedule);
    }
    tableNode = tableNode->NextSibling(EPG_XML_ELM_TABLE);
  }
  return true;
}

SchedulePtr cSchedules::AddSchedule(const tChannelID& ChannelID)
{
  //ChannelID.ClrRid();
  SchedulePtr schedule = GetSchedule(ChannelID);
  if (!schedule)
  {
    schedule = SchedulePtr(new cSchedule(ChannelID));
    m_schedules.push_back(schedule);
    ChannelPtr channel = cChannelManager::Get().GetByChannelID(ChannelID);
    if (channel)
      channel->SetSchedule(schedule);
  }
  return schedule;
}

SchedulePtr cSchedules::GetSchedule(const tChannelID& ChannelID)
{
  //ChannelID.ClrRid();
  for (std::vector<SchedulePtr>::iterator it = m_schedules.begin(); it != m_schedules.end(); ++it)
  {
    if ((*it)->ChannelID() == ChannelID)
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

std::vector<SchedulePtr> cSchedules::GetUpdatedSchedules(const std::map<int, time_t>& lastUpdated, CChannelFilter& filter)
{
  std::vector<SchedulePtr> retval;
  std::map<int, time_t>::iterator it;
  std::map<int, time_t>::const_iterator previousIt;
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
