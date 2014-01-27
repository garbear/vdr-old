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
#include "recordings/Timers.h"
#include "EPGDefinitions.h"
#include "platform/threads/threads.h"
#include "utils/XBMCTinyXML.h"

#include "vdr/utils/CalendarUtils.h"
#include "vdr/utils/UTF8Utils.h"

#define EPGDATAWRITEDELTA   600 // seconds between writing the epg.data file

// --- cSchedulesLock --------------------------------------------------------

cSchedulesLock::cSchedulesLock(bool WriteLock, int TimeoutMs)
{
  m_bLocked = cSchedules::Get().rwlock.Lock(WriteLock, TimeoutMs);
}

cSchedulesLock::~cSchedulesLock()
{
  if (m_bLocked)
    cSchedules::Get().rwlock.Unlock();
}

cSchedules* cSchedulesLock::Get(void) const
{
  return m_bLocked ? &cSchedules::Get() : NULL;
}

// --- cSchedules ------------------------------------------------------------

cSchedules::cSchedules(void)
{
  lastDump = time(NULL);
  modified = 0;
}

cSchedules& cSchedules::Get(void)
{
  static cSchedules _instance;
  return _instance;
}

void cSchedules::SetDataDirectory(const char *FileName)
{
  cSchedulesLock lock(true);
  cSchedules* schedules = lock.Get();
  if (schedules)
  {
    schedules->m_strDirectory = FileName ? FileName : "";
    dsyslog("EPG directory: %s", schedules->m_strDirectory.c_str());
    cEpgDataWriter::Get().SetDump(!schedules->m_strDirectory.empty());
  }
}

time_t cSchedules::Modified(void)
{
  cSchedulesLock lock(true);
  cSchedules* schedules = lock.Get();
  return schedules ? schedules->modified : 0;
}

void cSchedules::SetModified(cSchedule *Schedule)
{
  Schedule->SetModified();
  modified = time(NULL);
}

void cSchedules::Cleanup(bool Force)
{
  if (Force)
    lastDump = 0;
  time_t now = time(NULL);
  if (now - lastDump > EPGDATAWRITEDELTA)
  {
    if (Force)
      cEpgDataWriter::Get().Perform();
    else if (!cEpgDataWriter::Get().IsRunning())
      cEpgDataWriter::Get().CreateThread();
    lastDump = now;
  }
}

void cSchedules::ResetVersions(void)
{
  for (cSchedule *Schedule = First(); Schedule; Schedule = Next(Schedule))
    Schedule->ResetVersions();
}

bool cSchedules::ClearAll(void)
{
  for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer))
    Timer->SetEvent(NULL);
  for (cSchedule *Schedule = First(); Schedule; Schedule = Next(Schedule))
    Schedule->Cleanup(INT_MAX);
  return true;
}

bool cSchedules::Save(void)
{
  assert(!m_strDirectory.empty());
  bool bReturn(true);

  isyslog("saving EPG data to '%s'", m_strDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(EPG_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  for (cSchedule* schedule = First(); schedule; schedule = Next(schedule))
  {
    if (schedule->Save(m_strDirectory))
    {
      TiXmlElement tableElement(EPG_XML_ELM_TABLE);
      TiXmlNode* textNode = root->InsertEndChild(tableElement);
      if (textNode)
      {
        TiXmlText* text = new TiXmlText(StringUtils::Format("%s", schedule->ChannelID().Serialize().c_str()));
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
    std::string strFilename = m_strDirectory + "/epg.xml";
    std::string strTempFile = strFilename + ".tmp";
    if (xmlDoc.SaveFile(strTempFile))
    {
      CFile::Delete(strFilename);
      CFile::Rename(strTempFile, strFilename);
      return true;
    }
    else
    {
      CFile::Delete(strTempFile);
      esyslog("failed to save the EPG data: could not write to '%s'", strTempFile.c_str());
      return false;
    }
  }

  return bReturn;
}

bool cSchedules::Read(void)
{
  assert(!m_strDirectory.empty());

  isyslog("reading EPG data from '%s'", m_strDirectory.c_str());

  CXBMCTinyXML xmlDoc;
  std::string strFilename = m_strDirectory + "/epg.xml";
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
    cSchedule* schedule = AddSchedule(tChannelID::Deserialize(tableElem->GetText()));
    if (schedule)
    {
      if (schedule->Read(m_strDirectory))
        SetModified(schedule);
    }
    tableNode = tableNode->NextSibling(EPG_XML_ELM_TABLE);
  }
  return true;
}

cSchedule *cSchedules::AddSchedule(tChannelID ChannelID)
{
  //ChannelID.ClrRid();
  cSchedule *p = (cSchedule *) GetSchedule(ChannelID);
  if (!p)
  {
    p = new cSchedule(ChannelID);
    Add(p);
    ChannelPtr channel = cChannelManager::Get().GetByChannelID(ChannelID);
    if (channel)
      channel->SetSchedule(p);
  }
  return p;
}

const cSchedule *cSchedules::GetSchedule(tChannelID ChannelID) const
{
  //ChannelID.ClrRid();
  for (cSchedule *p = First(); p; p = Next(p))
  {
    if (p->ChannelID() == ChannelID)
      return p;
  }
  return NULL;
}

const cSchedule *cSchedules::GetSchedule(const cChannel *channel, bool bAddIfMissing) const
{
  if (!channel->HasSchedule() && bAddIfMissing)
  {
    cSchedule *Schedule = new cSchedule(channel->GetChannelID());
    ((cSchedules *) this)->Add(Schedule);
    channel->SetSchedule(Schedule);
  }

  return channel->HasSchedule() ? channel->Schedule() : NULL;
}
