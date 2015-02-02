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

#include "Recording.h"
#include "Recorder.h"
#include "RecordingDefinitions.h"
#include "channels/ChannelManager.h"
#include "devices/DeviceManager.h"
#include "settings/Settings.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"

#include <tinyxml.h>

namespace VDR
{

RecordingPtr cRecording::EmptyRecording;

cRecording::cRecording(void)
: m_id(RECORDING_INVALID_ID),
  m_playCount(0),
  m_priority(0),
  m_recorder(NULL)
{
}

cRecording::cRecording(const std::string&   strFoldername,
                       const std::string&   strTitle,
                       const ChannelPtr&    channel,
                       const CDateTime&     startTime,
                       const CDateTime&     endTime,
                       const CDateTime&     expires,
                       const CDateTimeSpan& resumePosition,
                       unsigned int         playCount,
                       unsigned int         priority)
: m_id(RECORDING_INVALID_ID),
  m_strFoldername(strFoldername),
  m_strTitle(strTitle),
  m_channel(channel),
  m_startTime(startTime),
  m_endTime(endTime),
  m_expires(expires),
  m_resumePosition(resumePosition),
  m_playCount(playCount),
  m_priority(priority),
  m_recorder(NULL)
{
}

cRecording::~cRecording(void)
{
  Interrupt();
  delete m_recorder;
}

std::string cRecording::URL(void) const
{
  // TODO
  return cSettings::Get().m_VideoDirectory + "/" + m_strFoldername + "/" + m_strTitle + ".ts";
}

bool cRecording::SetID(unsigned int id)
{
  if (m_id == RECORDING_INVALID_ID)
  {
    m_id = id;
    return true;
  }
  return false;
}

cRecording& cRecording::operator=(const cRecording& rhs)
{
  if (this != &rhs)
  {
    SetFoldername(rhs.m_strFoldername);
    SetTitle(rhs.m_strTitle);
    SetChannel(rhs.m_channel);
    SetTime(rhs.m_startTime, rhs.m_endTime);
    SetExpires(rhs.m_expires);
    SetResumePosition(rhs.m_resumePosition);
    SetPlayCount(rhs.m_playCount);
    SetPriority(rhs.m_priority);
  }
  return *this;
}

void cRecording::SetFoldername(const std::string& strFoldername)
{
  if (m_strFoldername != strFoldername)
  {
    m_strFoldername = strFoldername;
    SetChanged();
  }
}

void cRecording::SetTitle(const std::string& strTitle)
{
  if (m_strTitle != strTitle)
  {
    m_strTitle = strTitle;
    SetChanged();
  }
}

void cRecording::SetChannel(const ChannelPtr& channel)
{
  if (m_channel != channel)
  {
    m_channel = channel;
    SetChanged();
  }
}

void cRecording::SetTime(const CDateTime& startTime, const CDateTime& endTime)
{
  if (m_startTime != startTime || m_endTime != endTime)
  {
    m_startTime = startTime;
    m_endTime = endTime;
    SetChanged();
  }
}

void cRecording::SetExpires(const CDateTime& expires)
{
  if (m_expires != expires)
  {
    m_expires = expires;
    SetChanged();
  }
}

void cRecording::SetResumePosition(const CDateTimeSpan& resumePosition)
{
  if (m_resumePosition != resumePosition)
  {
    m_resumePosition = resumePosition;
    SetChanged();
  }
}

void cRecording::SetPlayCount(unsigned int playCount)
{
  if (m_playCount != playCount)
  {
    m_playCount = playCount;
    SetChanged();
  }
}

void cRecording::SetPriority(unsigned int priority)
{
  if (m_priority != priority)
  {
    m_priority = priority;
    SetChanged();
  }
}

bool cRecording::IsValid(bool bCheckID /* = true */) const
{
  return (bCheckID ? m_id != RECORDING_INVALID_ID : true) && // Condition (1)
         !m_strFoldername.empty()                         && // Condition (2)
         !m_strTitle.empty()                              && // Condition (3)
         m_channel && m_channel->IsValid()                && // Condition (4)
         m_startTime.IsValid()                            && // Condition (5)
         m_endTime.IsValid()                              && // Condition (6)
         m_startTime < m_endTime;                            // Condition (7)
}

void cRecording::LogInvalidProperties(bool bCheckID /* = true */) const
{
  // Check condition (1)
  if (bCheckID && m_id == RECORDING_INVALID_ID)
    esyslog("Invalid recording: ID has not been set");

  // Check conditions (2) and (3)
  if (m_strFoldername.empty() || m_strTitle.empty())
  {
    esyslog("Invalid recording %u: invalid %s", m_id,
        !m_strTitle.empty() ? "folder name" :
            !m_strFoldername.empty() ? "title" :
                "folder name and title");
  }

  // Check condition (4)
  if (!m_channel || !m_channel->IsValid())
    esyslog("Invalid timer %u: invalid channel", m_id);

  // Check conditions (5) and (6)
  if (!m_startTime.IsValid() || !m_endTime.IsValid())
  {
    esyslog("Invalid recording %u: invalid %s", m_id,
        m_endTime.IsValid() ? "start time" :
            m_startTime.IsValid() ? "end time" :
                "start and end time");
  }
  else
  {
    time_t startTime;
    time_t endTime;

    m_startTime.GetAsTime(startTime);
    m_endTime.GetAsTime(endTime);

    // Check condition (7)
    if (m_startTime >= m_endTime)
      esyslog("Invalid recording %u: start time (%d) is >= end time (%d)", m_id, startTime, endTime);
  }
}

void cRecording::Resume(void)
{
  if (!m_tunerHandle)
  {
    if (!m_recorder)
      m_recorder = new cRecorder(this);

    m_tunerHandle = cDeviceManager::Get().OpenVideoInput(m_recorder, TUNING_TYPE_RECORDING, m_channel);
  }
}

void cRecording::Interrupt(void)
{
  if (m_tunerHandle)
  {
    m_tunerHandle->Release();
    m_tunerHandle = cTunerHandle::EmptyHandle;
  }
}

bool cRecording::Serialise(TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement* elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (!IsValid())
  {
    LogInvalidProperties();
    return false;
  }

  time_t startTime;
  time_t endTime;

  m_startTime.GetAsTime(startTime);
  m_endTime.GetAsTime(endTime);

  elem->SetAttribute(RECORDING_XML_ATTR_ID, m_id);
  elem->SetAttribute(RECORDING_XML_ATTR_FOLDER, m_strFoldername);
  elem->SetAttribute(RECORDING_XML_ATTR_TITLE, m_strTitle);
  if (!m_channel->ID().Serialise(node))
    return false;
  elem->SetAttribute(RECORDING_XML_ATTR_START_TIME, startTime);
  elem->SetAttribute(RECORDING_XML_ATTR_END_TIME, endTime);
  if (m_expires.IsValid())
  {
    time_t expires;
    m_expires.GetAsTime(expires);
    elem->SetAttribute(RECORDING_XML_ATTR_EXPIRES, expires);
  }
  if (m_resumePosition.GetSecondsTotal() > 0)
    elem->SetAttribute(RECORDING_XML_ATTR_RESUME_POS, m_resumePosition.GetSecondsTotal());
  if (m_playCount > 0)
    elem->SetAttribute(RECORDING_XML_ATTR_PLAY_COUNT, m_playCount);
  if (m_priority > 0)
    elem->SetAttribute(RECORDING_XML_ATTR_PRIORITY, m_priority);

  return true;
}

bool cRecording::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (m_id != RECORDING_INVALID_ID)
  {
    esyslog("Error loading recording: object already has a valid ID (%u)", m_id);
    return false;
  }

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_ID);
    m_id = attr ? StringUtils::IntVal(attr, RECORDING_INVALID_ID) : RECORDING_INVALID_ID;
  }

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_FOLDER);
    m_strFoldername = attr ? attr : "";
  }

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_TITLE);
    m_strTitle = attr ? attr : "";
  }

  cChannelID chanId;
  chanId.Deserialise(node);
  m_channel = cChannelManager::Get().GetByChannelID(chanId);

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_START_TIME);
    m_startTime = (time_t)(attr ? StringUtils::IntVal(attr) : 0);
  }

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_END_TIME);
    m_endTime = (time_t)(attr ? StringUtils::IntVal(attr) : 0);
  }

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_EXPIRES);
    m_expires = (time_t)(attr ? StringUtils::IntVal(attr) : 0);
  }

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_RESUME_POS);
    m_resumePosition.SetDateTimeSpan(0, 0, 0, (attr ? StringUtils::IntVal(attr) : 0));
  }

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_PLAY_COUNT);
    m_playCount = attr ? StringUtils::IntVal(attr) : 0;
  }

  {
    const char* attr = elem->Attribute(RECORDING_XML_ATTR_PRIORITY);
    m_priority = attr ? StringUtils::IntVal(attr) : 0;
  }

  if (!IsValid())
  {
    LogInvalidProperties();
    return false;
  }

  SetChanged(false);

  return true;
}

}
