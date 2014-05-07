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

#include "RecordingInfo.h"
#include "Recording.h"
#include "RecordingDefinitions.h"
#include "channels/ChannelManager.h"
#include "Config.h"
#include "devices/Remux.h" // For MAX*PIDS
#include "epg/Event.h"
#include "epg/Components.h"
#include "utils/log/Log.h"
#include "utils/XBMCTinyXML.h"

#include <string>

using namespace std;

#define DECIMAL_POINT_C  '.'

namespace VDR
{

///< Converts the given double value to a string, making sure it uses a '.' as
///< the decimal point, independent of the currently selected locale.
///< If Format is given, it will be used instead of the default.
std::string dtoa(double d, const char *Format = "%f")
{
  static const lconv *loc = localeconv();
  char buf[16];
  snprintf(buf, sizeof(buf), Format, d);
  if (*loc->decimal_point != DECIMAL_POINT_C)
     strreplace(buf, *loc->decimal_point, DECIMAL_POINT_C);
  return buf;
}

cRecordingInfo::cRecordingInfo(ChannelPtr channel, const cEvent *Event)
{
  m_channel          = channel;
  m_channelID        = m_channel ? m_channel->GetChannelID() : cChannelID::InvalidID;
  m_ownEvent         = Event ? NULL : new cEvent(0);
  m_event            = m_ownEvent ? m_ownEvent : Event;
  m_dFramesPerSecond = DEFAULTFRAMESPERSECOND;
  m_iPriority        = MAXPRIORITY;
  m_iLifetime        = MAXLIFETIME;

  if (m_channel)
  {
    // Since the EPG data's component records can carry only a single
    // language code, let's see whether the channel's PID data has
    // more information:
    CEpgComponents *Components = (CEpgComponents *)m_event->Components();
    if (!Components)
      Components = new CEpgComponents;
    for (int i = 0; i < MAXAPIDS; i++)
    {
      string alang = m_channel->GetAudioStream(i).alang;
      if (!alang.empty())
      {
        CEpgComponent *Component = Components->GetComponent(i, 2, 3);
        if (!Component)
          Components->SetComponent(COMPONENT_ADD_NEW, 2, 3, alang.c_str(), NULL);
        else
          Component->SetLanguage(alang);
      }
    }
    // There's no "multiple languages" for Dolby Digital tracks, but
    // we do the same procedure here, too, in case there is no component
    // information at all:
    for (int i = 0; i < MAXDPIDS; i++)
    {
      string dlang = m_channel->GetDataStream(i).dlang;
      if (!dlang.empty())
      {
        CEpgComponent *Component = Components->GetComponent(i, 4, 0); // AC3 component according to the DVB standard
        if (!Component)
          Component = Components->GetComponent(i, 2, 5); // fallback "Dolby" component according to the "Premiere pseudo standard"
        if (!Component)
          Components->SetComponent(COMPONENT_ADD_NEW, 2, 5, dlang.c_str(), NULL);
        else
          Component->SetLanguage(dlang);
      }
    }
    // The same applies to subtitles:
    for (int i = 0; i < MAXSPIDS; i++)
    {
      string slang = m_channel->GetSubtitleStream(i).slang;
      if (!slang.empty())
      {
        CEpgComponent *Component = Components->GetComponent(i, 3, 3);
        if (!Component)
          Components->SetComponent(COMPONENT_ADD_NEW, 3, 3, slang.c_str(), NULL);
        else
          Component->SetLanguage(slang);
      }
    }
    if (Components != m_event->Components())
      ((cEvent *)m_event)->SetComponents(Components);
  }
}

cRecordingInfo::cRecordingInfo(const std::string& strFileName)
{
  m_channelID        = cChannelID::InvalidID;
  m_ownEvent         = new cEvent(0);
  m_event            = m_ownEvent;
  m_dFramesPerSecond = DEFAULTFRAMESPERSECOND;
  m_iPriority        = MAXPRIORITY;
  m_iLifetime        = MAXLIFETIME;
  m_strFilename      = StringUtils::Format("%s%s", strFileName.c_str(), INFOFILESUFFIX);
}

cRecordingInfo::~cRecordingInfo()
{
  delete m_ownEvent;
}

void cRecordingInfo::SetData(const char *Title, const char *ShortText, const char *Description)
{
  if (!isempty(Title))
     ((cEvent *)m_event)->SetTitle(Title);
  if (!isempty(ShortText))
     ((cEvent *)m_event)->SetShortText(ShortText);
  if (!isempty(Description))
     ((cEvent *)m_event)->SetDescription(Description);
}

void cRecordingInfo::SetFramesPerSecond(double FramesPerSecond)
{
  m_dFramesPerSecond = FramesPerSecond;
}

bool cRecordingInfo::Read(const std::string& strFilename /* = "" */)
{
  if (!m_ownEvent)
    return false;

  std::string useFilename(strFilename.empty() ? m_strFilename : strFilename);
  if (useFilename.empty())
    return false;
  m_strFilename = useFilename;

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(m_strFilename))
    return false;

  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), RECORDING_XML_ROOT))
  {
    esyslog("failed to find root element '%s' in file '%s'", RECORDING_XML_ROOT, m_strFilename.c_str());
    return false;
  }

  if (const TiXmlNode *recordingNode = root->FirstChild(RECORDING_XML_ELM_RECORDING))
  {
    if (const TiXmlElement *recordingElem = recordingNode->ToElement())
    {
      if (const char* attr = recordingElem->Attribute(RECORDING_XML_ATTR_CHANNEL_ID))
        m_channelID.Deserialize(attr);

      if (m_channelID.Valid())
        m_channel = cChannelManager::Get().GetByChannelID(m_channelID);
      else if (const char* attr = recordingElem->Attribute(RECORDING_XML_ATTR_CHANNEL_UID))
        m_channel = cChannelManager::Get().GetByChannelUID(StringUtils::IntVal(attr));

      if (const char* attr = recordingElem->Attribute(RECORDING_XML_ATTR_FPS))
        m_dFramesPerSecond = atod(attr);

      if (const char* attr = recordingElem->Attribute(RECORDING_XML_ATTR_PRIORITY))
        m_iPriority = StringUtils::IntVal(attr);

      if (const char* attr = recordingElem->Attribute(RECORDING_XML_ATTR_LIFETIME))
        m_iLifetime = StringUtils::IntVal(attr);

      if (const TiXmlNode *eventNode = recordingElem->FirstChild(RECORDING_XML_ELM_EPG_EVENT))
      {
        if (const TiXmlElement *elem = eventNode->ToElement())
        {
          if (const char* attr = elem->Attribute(RECORDING_XML_ATTR_EPG_EVENT_ID))
            m_ownEvent->SetEventID(StringUtils::IntVal(attr));

          if (const char* attr = elem->Attribute(RECORDING_XML_ATTR_EPG_START_TIME))
            m_ownEvent->SetStartTime(CDateTime((time_t)StringUtils::IntVal(attr)));

          if (const char* attr = elem->Attribute(RECORDING_XML_ATTR_EPG_DURATION))
            m_ownEvent->SetDuration(StringUtils::IntVal(attr));

          if (const char* attr = elem->Attribute(RECORDING_XML_ATTR_EPG_TABLE))
            m_ownEvent->SetTableID(StringUtils::IntVal(attr));

          if (const char* attr = elem->Attribute(RECORDING_XML_ATTR_EPG_VERSION))
            m_ownEvent->SetVersion(StringUtils::IntVal(attr));

          return true;
        }
      }
    }
  }

  return false;
}

bool cRecordingInfo::Write(const std::string& strFilename /* = "" */) const
{
  std::string useFilename(strFilename.empty() ? m_strFilename : strFilename);
  if (useFilename.empty())
    return false;
  m_strFilename = useFilename;

  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(RECORDING_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  TiXmlElement recordingElement(RECORDING_XML_ELM_RECORDING);

  if (m_channelID.Valid())
    recordingElement.SetAttribute(RECORDING_XML_ATTR_CHANNEL_ID, m_channelID.Serialize().c_str());

  if (m_channel)
  {
    recordingElement.SetAttribute(RECORDING_XML_ATTR_CHANNEL_NAME, m_channel->Name().c_str());
    recordingElement.SetAttribute(RECORDING_XML_ATTR_CHANNEL_UID,  m_channel->Hash());
  }

  recordingElement.SetAttribute(RECORDING_XML_ATTR_FPS,      dtoa(m_dFramesPerSecond, "%.10g").c_str());
  recordingElement.SetAttribute(RECORDING_XML_ATTR_PRIORITY, m_iPriority);
  recordingElement.SetAttribute(RECORDING_XML_ATTR_LIFETIME, m_iLifetime);

  TiXmlNode* recordingNode = root->InsertEndChild(recordingElement);

  if (m_event)
  {
    TiXmlElement epgElement(RECORDING_XML_ELM_EPG_EVENT);
    epgElement.SetAttribute(RECORDING_XML_ATTR_EPG_EVENT_ID,   m_event->EventID());
    epgElement.SetAttribute(RECORDING_XML_ATTR_EPG_START_TIME, m_event->StartTimeAsTime());
    epgElement.SetAttribute(RECORDING_XML_ATTR_EPG_DURATION,   m_event->Duration());
    epgElement.SetAttribute(RECORDING_XML_ATTR_EPG_TABLE,      m_event->TableID());
    epgElement.SetAttribute(RECORDING_XML_ATTR_EPG_VERSION,    m_event->Version());
    TiXmlNode* epgNode = recordingNode->InsertEndChild(epgElement);
  }

  isyslog("saving recording info to '%s'", m_strFilename.c_str());
  if (!xmlDoc.SafeSaveFile(m_strFilename))
  {
    esyslog("failed to save recording info: could not write to '%s'", m_strFilename.c_str());
    return false;
  }
  return true;
}

void cRecordingInfo::SetEvent(const cEvent* event)
{
  m_event = event;
  Write();
}

std::string cRecordingInfo::Title(void) const
{
  return m_event->Title();
}

std::string cRecordingInfo::ShortText(void) const
{
  return m_event->ShortText();
}

std::string cRecordingInfo::Description(void) const
{
  return m_event->Description();
}

const CEpgComponents* cRecordingInfo::Components(void) const
{
  return m_event->Components();
}

void cRecordingInfo::SetTitle(const std::string& strTitle)
{
  if (m_ownEvent)
    m_ownEvent->SetTitle(strTitle);
}

}
