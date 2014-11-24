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

#include "RecordingManager.h"
#include "Recording.h"
#include "RecordingDefinitions.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <utility>

namespace VDR
{

cRecordingManager::cRecordingManager(void)
 : m_maxID(RECORDING_INVALID_ID)
{
}

cRecordingManager& cRecordingManager::Get(void)
{
  static cRecordingManager instance;
  return instance;
}

cRecordingManager::~cRecordingManager(void)
{
  for (RecordingMap::const_iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
    it->second->UnregisterObserver(this);
}

RecordingPtr cRecordingManager::GetByID(unsigned int id) const
{
  PLATFORM::CLockObject lock(m_mutex);

  RecordingMap::const_iterator it = m_recordings.find(id);
  if (it != m_recordings.end())
    return it->second;

  return cRecording::EmptyRecording;
}

RecordingPtr cRecordingManager::GetByChannel(const ChannelPtr& channel, const CDateTime& time) const
{
  PLATFORM::CLockObject lock(m_mutex);

  for (RecordingMap::const_iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
    if (it->second->Channel() == channel && it->second->StartTime() <= time && time < it->second->EndTime())
      return it->second;

  return cRecording::EmptyRecording;
}

RecordingVector cRecordingManager::GetRecordings(void) const
{
  RecordingVector recordings;

  PLATFORM::CLockObject lock(m_mutex);

  for (RecordingMap::const_iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
    recordings.push_back(it->second);

  return recordings;
}

size_t cRecordingManager::RecordingCount(void) const
{
  PLATFORM::CLockObject lock(m_mutex);
  return m_recordings.size();
}

bool cRecordingManager::AddRecording(const RecordingPtr& newRecording)
{
  if (newRecording && newRecording->IsValid(false))
  {
    PLATFORM::CLockObject lock(m_mutex);

    newRecording->SetID(++m_maxID);
    newRecording->RegisterObserver(this);
    m_recordings.insert(std::make_pair(newRecording->ID(), newRecording));

    SetChanged();

    const CDateTime now = CDateTime::GetUTCDateTime();
    if (newRecording->StartTime() <= now && now < newRecording->EndTime())
      newRecording->Resume();

    return true;
  }

  return false;
}

bool cRecordingManager::UpdateRecording(unsigned int id, const cRecording& updatedRecording)
{
  PLATFORM::CLockObject lock(m_mutex);

  RecordingPtr recording = GetByID(id);
  if (recording)
  {
    *recording = updatedRecording;
    recording->NotifyObservers();
    return true;
  }

  return false;
}

bool cRecordingManager::RemoveRecording(unsigned int id)
{
  PLATFORM::CLockObject lock(m_mutex);

  RecordingMap::iterator it = m_recordings.find(id);
  if (it != m_recordings.end())
  {
    it->second->Interrupt();
    it->second->UnregisterObserver(this);
    m_recordings.erase(it);

    SetChanged();

    return true;
  }

  return false;
}

void cRecordingManager::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageRecordingChanged:
    SetChanged();
    break;
  default:
    break;
  }
}

void cRecordingManager::NotifyObservers()
{
  if (Observable::NotifyObservers(ObservableMessageRecordingChanged))
    Save();
}

bool cRecordingManager::Load(void)
{
  if (!Load("special://home/system/recordings.xml") &&
      !Load("special://vdr/system/recordings.xml"))
  {
    // create a new empty file
    Save("special://home/system/recordings.xml");
    return false;
  }

  return true;
}

bool cRecordingManager::Load(const std::string& file)
{
  isyslog("Loading recordings from '%s'", file.c_str());

  PLATFORM::CLockObject lock(m_mutex);

  m_recordings.clear();

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
    return false;

  m_strFilename = file;

  TiXmlElement* root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), RECORDING_XML_ROOT))
  {
    esyslog("Failed to find root element '%s' in file '%s'", RECORDING_XML_ROOT, file.c_str());
    return false;
  }

  const TiXmlNode* node = root->FirstChild(RECORDING_XML_ELM_RECORDING);
  while (node != NULL)
  {
    RecordingPtr recording = RecordingPtr(new cRecording);

    if (!recording->Deserialise(node))
    {
      esyslog("Failed to find load recording from file '%s'", file.c_str());
      continue;
    }

    if (GetByID(recording->ID()))
    {
      esyslog("Recording with ID %u already exists, skipping", recording->ID());
      continue;
    }

    if (recording->ID() > m_maxID)
      m_maxID = recording->ID();

    m_recordings.insert(std::make_pair(recording->ID(), recording));

    node = node->NextSibling(RECORDING_XML_ELM_RECORDING);
  }

  return true;
}

bool cRecordingManager::Save(const std::string& file /* = "" */)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(RECORDING_XML_ROOT);
  TiXmlNode* root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  PLATFORM::CLockObject lock(m_mutex);

  for (RecordingMap::const_iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    TiXmlElement recordingElement(RECORDING_XML_ELM_RECORDING);
    TiXmlNode* node = root->InsertEndChild(recordingElement);
    it->second->Serialise(node);
  }

  if (!file.empty())
    m_strFilename = file;

  assert(!m_strFilename.empty());

  isyslog("Saving recordings to '%s'", m_strFilename.c_str());
  if (!xmlDoc.SafeSaveFile(m_strFilename))
  {
    esyslog("Failed to save recordings: could not write to '%s'", m_strFilename.c_str());
    return false;
  }

  return true;
}

}
