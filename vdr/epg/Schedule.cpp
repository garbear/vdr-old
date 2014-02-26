#include "Schedule.h"
#include "EPG.h"
#include "EPGDefinitions.h"
#include "Config.h"
#include "channels/Channel.h"
#include "channels/ChannelManager.h"
#include "utils/XBMCTinyXML.h"

#define RUNNINGSTATUSTIMEOUT 30 // seconds before the running status is considered unknown

cSchedule::cSchedule(tChannelID ChannelID)
{
  channelID   = ChannelID;
  hasRunning  = false;
}

cEvent *cSchedule::AddEvent(cEvent *Event)
{
  events.Add(Event);
  Event->schedule = this;
  HashEvent(Event);
  return Event;
}

void cSchedule::DelEvent(cEvent *Event)
{
  if (Event->schedule == this)
  {
    if (hasRunning && Event->IsRunning())
      ClrRunningStatus();
    UnhashEvent(Event);
    events.Del(Event);
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

const cEvent *cSchedule::GetPresentEvent(void) const
{
  const cEvent *pe = NULL;
  CDateTime now = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
  for (cEvent *p = events.First(); p; p = events.Next(p))
  {
    if (p->StartTime() <= now)
      pe = p;
    else if (p->StartTime() > now + CDateTimeSpan(0, 1, 0, 0))
      break;
    if (p->SeenWithin(RUNNINGSTATUSTIMEOUT)
        && p->RunningStatus() >= SI::RunningStatusPausing)
      return p;
  }
  return pe;
}

const cEvent *cSchedule::GetFollowingEvent(void) const
{
  const cEvent *p = GetPresentEvent();
  if (p)
    p = events.Next(p);
  else
  {
    CDateTime now = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
    for (p = events.First(); p; p = events.Next(p))
    {
      if (p->StartTime() >= now)
        break;
    }
  }
  return p;
}

cEvent *cSchedule::GetEvent(tEventID EventID, CDateTime StartTime /* = CDateTime::GetCurrentDateTime() */)
{
  // Returns the event info with the given StartTime or, if no actual StartTime
  // is given, the one with the given EventID.
  if (StartTime.IsValid()) // 'StartTime < 0' is apparently used with NVOD channels
  {
    time_t tmStart;
    StartTime.GetAsTime(tmStart);
    return eventsHashStartTime.Get(tmStart);
  }
  else
    return eventsHashID.Get(EventID);
}

void cSchedule::SetRunningStatus(cEvent *Event, int RunningStatus, cChannel *Channel)
{
  hasRunning = false;
  for (cEvent *p = events.First(); p; p = events.Next(p))
  {
    if (p == Event)
    {
      if (p->RunningStatus() > SI::RunningStatusNotRunning
          || RunningStatus > SI::RunningStatusNotRunning)
      {
        p->SetRunningStatus(RunningStatus, Channel);
        break;
      }
    }
    else if (RunningStatus >= SI::RunningStatusPausing
        && p->StartTime() < Event->StartTime())
      p->SetRunningStatus(SI::RunningStatusNotRunning);
    if (p->RunningStatus() >= SI::RunningStatusPausing)
      hasRunning = true;
  }
}

void cSchedule::ClrRunningStatus(cChannel *Channel)
{
  if (hasRunning)
  {
    for (cEvent *p = events.First(); p; p = events.Next(p))
    {
      if (p->RunningStatus() >= SI::RunningStatusPausing)
      {
        p->SetRunningStatus(SI::RunningStatusNotRunning, Channel);
        hasRunning = false;
        break;
      }
    }
  }
}

void cSchedule::ResetVersions(void)
{
  for (cEvent *p = events.First(); p; p = events.Next(p))
    p->SetVersion(0xFF);
}

void cSchedule::Sort(void)
{
  events.Sort();
  // Make sure there are no RunningStatusUndefined before the currently running event:
  if (hasRunning)
  {
    for (cEvent *p = events.First(); p; p = events.Next(p))
    {
      if (p->RunningStatus() >= SI::RunningStatusPausing)
        break;
      p->SetRunningStatus(SI::RunningStatusNotRunning);
    }
  }
}

void cSchedule::DropOutdated(const CDateTime& SegmentStart, const CDateTime& SegmentEnd, uchar TableID, uchar Version)
{
  if (SegmentStart.IsValid() && SegmentEnd.IsValid())
  {
    for (cEvent *p = events.First(); p; p = events.Next(p))
    {
      if (p->EndTime() > SegmentStart)
      {
        if (p->StartTime() < SegmentEnd)
        {
          // The event overlaps with the given time segment.
          if (p->TableID() > TableID
              || (p->TableID() == TableID && p->Version() != Version))
          {
            // The segment overwrites all events from tables with higher ids, and
            // within the same table id all events must have the same version.
            // We can't delete the event right here because a timer might have
            // a pointer to it, so let's set its id and start time to 0 to have it
            // "phased out":
            if (hasRunning && p->IsRunning())
              ClrRunningStatus();
            UnhashEvent(p);
            p->eventID = 0;
            p->m_startTime.Reset();
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
  Cleanup(CDateTime::GetCurrentDateTime().GetAsUTCDateTime());
}

void cSchedule::Cleanup(const CDateTime& Time)
{
  cEvent *Event;
  while ((Event = events.First()) != NULL)
  {
    if (!Event->HasTimer() && (!Time.IsValid() || (Event->EndTime() + CDateTimeSpan(0, 1, g_setup.EPGLinger, 0)) < Time)) // adding one hour for safety
    {
      modified = CDateTime::GetCurrentDateTime();
      DelEvent(Event);
    }
    else
      break;
  }
}

bool cSchedule::Read(void)
{
  assert(!g_setup.EPGDirectory.empty());

  CXBMCTinyXML xmlDoc;
  std::string strFilename = g_setup.EPGDirectory + "/epg_" + channelID.Serialize() + ".xml";
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
  assert(!g_setup.EPGDirectory.empty());

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

  std::string strFilename = g_setup.EPGDirectory + "/epg_" + channelID.Serialize() + ".xml";
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

  epgElement->SetAttribute(EPG_XML_ATTR_CHANNEL, channelID.Serialize());

  cEvent* p;
  for (p = events.First(); p; p = events.Next(p))
  {
    if (!p->Serialise(epgElement))
      return false;
  }

  return true;
}
