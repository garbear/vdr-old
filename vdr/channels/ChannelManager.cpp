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
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "dvb/EITScan.h"
//#include "utils/I18N.h"
#include "filesystem/File.h"
#include "utils/CRC32.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/UTF8Utils.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>
#include <assert.h>

using namespace std;
using namespace VDR;
using namespace PLATFORM;

/*
string ChannelString(const ChannelPtr &channel, int number)
{
  string retval;
  if (channel)
    retval = StringUtils::Format("%d%s  %s", channel->Number(), number ? "-" : "", channel->Name().c_str());
  else if (number)
    retval = StringUtils::Format("%d-", number);
  else
    //retval = StringUtils::Format("%s", tr("*** Invalid Channel ***"));
    retval = StringUtils::Format("%s", "*** Invalid Channel ***");
  return retval;
}
*/

cChannelManager::cChannelManager()
 : m_maxNumber(0),
   m_maxChannelNameLength(0),
   m_maxShortChannelNameLength(0)
{
}

cChannelManager &cChannelManager::Get()
{
  static cChannelManager instance;
  return instance;
}

void cChannelManager::Clear(void)
{
  CLockObject lock(m_mutex);
  m_channels.clear();
  m_channelSids.clear();
}

void cChannelManager::Notify(const Observable &obs, const ObservableMessage msg)
{
  {
    CLockObject lock(m_mutex);
    SetChanged();
  }
  NotifyObservers(msg);
}

void cChannelManager::AddChannel(ChannelPtr channel)
{
  assert(channel.get());

  CLockObject lock(m_mutex);

  // Avoid adding two of the same channel objects to the vector
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if (*itChannel == channel)
      return;
  }

  channel->RegisterObserver(this);
  m_channels.push_back(channel);
}

void cChannelManager::AddChannels(const ChannelVector& channels)
{
  if (!channels.empty())
  {
    for (ChannelVector::const_iterator itChannel = channels.begin(); itChannel != channels.end(); ++itChannel)
      AddChannel(*itChannel);

    SetChanged();
    NotifyObservers(ObservableMessageChannelChanged);

    Save();
  }
}

void cChannelManager::RemoveChannel(ChannelPtr channel)
{
  assert(channel.get());

  channel->UnregisterObserver(this);
  CLockObject lock(m_mutex);
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if (channel == *itChannel)
    {
      m_channels.erase(itChannel);

      // TODO: Also erase from sid map
      // TODO TODO: Get rid of sid map

      break;
    }
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

  ReNumber();

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

ChannelPtr cChannelManager::GetByNumber(int number, int skipGap /* = 0 */)
{
  ChannelPtr previous;
  CLockObject lock(m_mutex);
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    ChannelPtr &channel = *itChannel;

    if (channel->Number() == number)
      return channel;
    else if (skipGap && channel->Number() > number)
      return skipGap > 0 ? channel : previous;

    previous = channel;
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByServiceID(int serviceID, TRANSPONDER_TYPE source, int transponder) const
{
  assert(serviceID);
  assert(transponder);

  ChannelSidMap::const_iterator it = m_channelSids.find(serviceID);
  if (it != m_channelSids.end())
  {
    const ChannelVector &channelVec = it->second;
    for (ChannelVector::const_iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
    {
      if ((*itChannel)->ID().Sid() == serviceID && (*itChannel)->GetTransponder().Type() == source && ISTRANSPONDER((*itChannel)->FrequencyMHzWithPolarization(), transponder))
        return (*itChannel);
    }
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByServiceID(const ChannelVector& channels, int serviceID, TRANSPONDER_TYPE source, int transponder)
{
  assert(serviceID);
  assert(transponder);

  for (ChannelVector::const_iterator itChannel = channels.begin(); itChannel != channels.end(); ++itChannel)
  {
    if ((*itChannel)->ID().Sid() == serviceID && (*itChannel)->GetTransponder().Type() == source && ISTRANSPONDER((*itChannel)->FrequencyMHzWithPolarization(), transponder))
      return (*itChannel);
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByChannelID(const cChannelID& channelID)
{
  CLockObject lock(m_mutex);

  int serviceID = channelID.Sid();
  ChannelSidMap::iterator it = m_channelSids.find(serviceID);
  if (it != m_channelSids.end())
  {
    ChannelVector &channelVec = it->second;
    for (ChannelVector::iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
    {
      ChannelPtr &channel = *itChannel;
      if (channel->ID().Sid() == serviceID && channel->ID() == channelID)
        return channel;
    }
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByChannelID(uint16_t nid, uint16_t tsid, uint16_t sid)
{
  CLockObject lock(m_mutex);

  ChannelSidMap::iterator it = m_channelSids.find(sid);
  if (it != m_channelSids.end())
  {
    ChannelVector &channelVec = it->second;
    for (ChannelVector::iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
    {
      ChannelPtr &channel = *itChannel;
      if (channel->ID().Sid() == sid && channel->ID().Tsid() == tsid && channel->ID().Nid() == nid)
        return channel;
    }
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByTransponderID(const cChannelID& channelID)
{
  int nid = channelID.Nid();
  int tid = channelID.Tsid();
  CLockObject lock(m_mutex);
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    ChannelPtr &channel = *itChannel;
    if (channel->ID().Nid() == nid && channel->ID().Tsid() == tid)
      return channel;
  }
  return cChannel::EmptyChannel;
}

void cChannelManager::ReNumber()
{
  CLockObject lock(m_mutex);
  m_channelSids.clear();
  m_maxNumber = 0;

  unsigned int number = 1;
  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    const ChannelPtr &channel = *itChannel;
    m_channelSids[channel->ID().Sid()].push_back(channel);
    m_maxNumber = number;
    channel->SetNumber(number++);
  }
}

bool cChannelManager::HasUniqueChannelID(const ChannelPtr &newChannel, const ChannelPtr &oldChannel /* = cChannel::EmptyChannel */) const
{
  assert(newChannel.get());

  const cChannelID& newChannelID = newChannel->ID();
  CLockObject lock(m_mutex);
  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    const ChannelPtr &channel = *itChannel;
    if (channel != oldChannel && channel->ID() == newChannelID)
      return false;
  }
  return true;
}

void cChannelManager::SetModified(void)
{
  {
    PLATFORM::CLockObject lock(m_mutex);
    m_maxChannelNameLength = m_maxShortChannelNameLength = 0;
    SetChanged();
  }
  NotifyObservers();
}

ChannelPtr cChannelManager::GetByChannelUID(uint32_t channelUID) const
{
  ChannelPtr result;
  ChannelPtr channel;

  // maybe we need to use a lookup table
  CLockObject lock(m_mutex);
  for (ChannelVector::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it)
  {
    channel = (*it);
    if(channelUID == channel->UID()) {
      result = channel;
      break;
    }
  }

  return result;
}

ChannelVector cChannelManager::GetCurrent(void) const
{
  PLATFORM::CLockObject lock(m_mutex);
  return m_channels;
}

void cChannelManager::CreateChannelGroups(bool automatic)
{
  std::string groupname;

  CLockObject lock(m_mutex);
  for (ChannelVector::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it)
  {
    ChannelPtr channel = *it;
    bool isRadio = CChannelFilter::IsRadio(channel);

    if (automatic)
      groupname = channel->Provider();

    if (groupname.empty())
      continue;

    if (!CChannelGroups::Get(isRadio).HasGroup(groupname))
    {
      CChannelGroup group(isRadio, groupname, automatic);
      CChannelGroups::Get(isRadio).AddGroup(group);
    }
  }
}

unsigned int cChannelManager::ChannelCount() const
{
  CLockObject lock(m_mutex);
  return m_channels.size();
}
