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

#include "ChannelManager.h"
#include "ChannelDefinitions.h"
#include "ChannelGroup.h"
#include "ChannelFilter.h"
//#include "utils/I18N.h"
#include "filesystem/File.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/UTF8Utils.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>
#include <assert.h>

using namespace PLATFORM;
using namespace VDR;
using namespace std;

cChannelManager::cChannelManager(void)
{
}

cChannelManager::~cChannelManager(void)
{
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
    (*itChannel)->UnregisterObserver(this);
}

cChannelManager &cChannelManager::Get()
{
  static cChannelManager instance;
  return instance;
}

void cChannelManager::AddChannel(const ChannelPtr& channel)
{
  assert(channel.get());

  CLockObject lock(m_mutex);

  // Avoid adding two of the same channel objects to the vector
  if (std::find(m_channels.begin(), m_channels.end(), channel) != m_channels.end())
    return;

  channel->RegisterObserver(this);
  m_channels.push_back(channel);
  SetChanged();
}

void cChannelManager::AddChannels(const ChannelVector& channels)
{
  for (ChannelVector::const_iterator itChannel = channels.begin(); itChannel != channels.end(); ++itChannel)
    AddChannel(*itChannel);
}

void cChannelManager::MergeChannelProps(const ChannelPtr& channel)
{
  const uint16_t tsid = channel->ID().Tsid();
  const uint16_t sid  = channel->ID().Sid();

  if (tsid == 0 || sid == 0)
    return;

  CLockObject lock(m_mutex);

  bool bFound = false;
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if (tsid == (*itChannel)->ID().Tsid() &&
        sid  == (*itChannel)->ID().Sid())
    {
      bFound = true;
      (*itChannel)->SetStreams(channel->GetVideoStream(),
                               channel->GetAudioStreams(),
                               channel->GetDataStreams(),
                               channel->GetSubtitleStreams(),
                               channel->GetTeletextStream());
      (*itChannel)->SetCaDescriptors(channel->GetCaDescriptors());
      (*itChannel)->NotifyObservers();
      break;
    }
  }

  if (!bFound)
  {
    channel->RegisterObserver(this);
    m_channels.push_back(channel);
    SetChanged();
  }
}

void cChannelManager::MergeChannelNamesAndModulation(const ChannelPtr& channel)
{
  const uint16_t tsid = channel->ID().Tsid();
  const uint16_t sid  = channel->ID().Sid();

  if (tsid == 0 || sid == 0)
    return;

  CLockObject lock(m_mutex);

  bool bFound = false;
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if (tsid == (*itChannel)->ID().Tsid() &&
        sid  == (*itChannel)->ID().Sid())
    {
      bFound = true;
      (*itChannel)->SetName(channel->Name(),
                            channel->ShortName(),
                            channel->Provider());

      (*itChannel)->SetNumber(channel->Number(), channel->SubNumber());

      fe_modulation_t modulation = channel->GetTransponder().Modulation();
      if (modulation != (*itChannel)->GetTransponder().Modulation())
      {
        (*itChannel)->GetTransponder().SetModulation(modulation);
        (*itChannel)->SetChanged();
      }

      if (channel->ATSCSourceID() != ATSC_SOURCE_ID_NONE)
        (*itChannel)->SetATSCSourceId(channel->ATSCSourceID());

      (*itChannel)->NotifyObservers();
      break;
    }
  }

  if (!bFound)
  {
    channel->RegisterObserver(this);
    m_channels.push_back(channel);
    SetChanged();
  }
}

ChannelPtr cChannelManager::GetByChannelID(const cChannelID& channelID) const
{
  CLockObject lock(m_mutex);

  // TODO: Search m_channelSids
  //const uint16_t serviceID = channelID.Sid();

  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if ((*itChannel)->ID() == channelID)
      return (*itChannel);
  }

  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByChannelUID(uint32_t channelUid) const
{
  CLockObject lock(m_mutex);

  // maybe we need to use a lookup table

  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if ((*itChannel)->UID() == channelUid)
      return (*itChannel);
  }

  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByTransportAndService(uint16_t network, uint16_t transport, uint16_t service)
{
  CLockObject lock(m_mutex);

  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if ((*itChannel)->ID().Nid()  == network &&
        (*itChannel)->ID().Tsid() == transport &&
        (*itChannel)->ID().Sid()  == service)
      return (*itChannel);
  }

  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByFrequencyAndATSCSourceId(unsigned int frequency, uint32_t sourceId)
{
  CLockObject lock(m_mutex);

  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if ((*itChannel)->GetTransponder().FrequencyHz()  == frequency &&
        (*itChannel)->ID().ATSCSourceId()  == sourceId)
      return (*itChannel);
  }

  return cChannel::EmptyChannel;
}

ChannelVector cChannelManager::GetCurrent(void) const
{
  CLockObject lock(m_mutex);
  return m_channels;
}

size_t cChannelManager::ChannelCount() const
{
  CLockObject lock(m_mutex);
  return m_channels.size();
}

void cChannelManager::RemoveChannel(const ChannelPtr& channel)
{
  CLockObject lock(m_mutex);

  ChannelVector::iterator itChannel = std::find(m_channels.begin(), m_channels.end(), channel);
  if (itChannel != m_channels.end())
  {
    (*itChannel)->UnregisterObserver(this);
    m_channels.erase(itChannel);
    SetChanged();
  }
}

void cChannelManager::Clear(void)
{
  CLockObject lock(m_mutex);
  m_channels.clear();
  SetChanged();
}

void cChannelManager::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageChannelChanged:
    SetChanged();
    break;
  default:
    break;
  }
}

void cChannelManager::NotifyObservers()
{
  if (Changed())
  {
    Observable::NotifyObservers(ObservableMessageChannelChanged);
    Save();
  }
}

bool cChannelManager::Load(void)
{
  if (!Load("special://home/system/channels.xml") &&
      !Load("special://vdr/system/channels.xml"))
  {
    return false;
  }

  return true;
}

bool cChannelManager::Load(const std::string &file)
{
  CLockObject lock(m_mutex);
  Clear();

  m_strFilename = file;

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    esyslog("ChannelManager: Failed to load channels from %s, Line %d", file.c_str(), xmlDoc.ErrorRow());
    esyslog("%s", xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), CHANNEL_XML_ROOT))
  {
    // log
    return false;
  }

  const TiXmlNode *channelNode = root->FirstChild(CHANNEL_XML_ELM_CHANNEL);
  while (channelNode != NULL)
  {
    ChannelPtr channel = ChannelPtr(new cChannel);
    if (channel && channel->Deserialise(channelNode))
    {
      AddChannel(channel);
    }
    else
    {
      // log
      continue;
    }
    channelNode = channelNode->NextSibling(CHANNEL_XML_ELM_CHANNEL);
  }

  SetChanged();

  // Don't continue if we didn't load any channels
  return !m_channels.empty();
}

bool cChannelManager::Save(const string &file /* = ""*/)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(CHANNEL_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  CLockObject lock(m_mutex);
  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    const ChannelPtr &channel = *itChannel;
    TiXmlElement channelElement(CHANNEL_XML_ELM_CHANNEL);
    TiXmlNode *channelNode = root->InsertEndChild(channelElement);
    channel->Serialise(channelNode);
  }

  if (!file.empty())
    m_strFilename = file;

  assert(!m_strFilename.empty());

  isyslog("saving channel configuration to '%s'", m_strFilename.c_str());
  if (!xmlDoc.SafeSaveFile(m_strFilename))
  {
    esyslog("failed to save the channel configuration: could not write to '%s'", m_strFilename.c_str());
    return false;
  }
  return true;
}

void cChannelManager::CreateChannelGroups(bool automatic)
{
  string groupname;

  CLockObject lock(m_mutex);
  for (ChannelVector::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it)
  {
    const ChannelPtr& channel = *it;
    bool bIsRadio = CChannelFilter::IsRadio(channel);

    if (automatic)
      groupname = channel->Provider();

    if (groupname.empty())
      continue;

    if (!CChannelGroups::Get(bIsRadio).HasGroup(groupname))
    {
      CChannelGroup group(bIsRadio, groupname, automatic);
      CChannelGroups::Get(bIsRadio).AddGroup(group);
    }
  }
}
