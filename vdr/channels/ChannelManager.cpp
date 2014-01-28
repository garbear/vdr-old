/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/UTF8Utils.h"
#include "utils/CRC32.h"

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
  {
    if (channel->GroupSep())
      retval = StringUtils::Format("%s", channel->Name().c_str());
    else
      retval = StringUtils::Format("%d%s  %s", channel->Number(), number ? "-" : "", channel->Name().c_str());
  }
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
    if (LoadConf("special://home/system/channels.conf") ||
        LoadConf("special://vdr/system/channels.conf"))
    {
      // convert to xml
      isyslog("converting channels.conf to channels.xml");
      string strFile = m_strFilename;
      SetChanged();
      if (Save("special://home/system/channels.xml"))
        CFile::Delete(strFile);

      return true;
    }

    return false;
  }

  return true;
}

bool cChannelManager::Load(const std::string &file)
{
  CLockObject lock(m_mutex);
  Clear();

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    //CLog::Log(LOGERROR, "CSettings: error loading settings definition from %s, Line %d\n%s", file.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    //cout << "Error loading channels from " << file << ", Line " << xmlDoc.ErrorRow() << endl;
    //cout << xmlDoc.ErrorDesc() << endl;
    return false;
  }

  m_strFilename = file;

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

bool cChannelManager::LoadConf(const string& strFilename)
{
  if (strFilename.empty())
    return false;

  CLockObject lock(m_mutex);
  Clear();

  CFile file;
  if (file.Open(strFilename))
  {
    m_strFilename = strFilename;
    string strLine;
    unsigned int line = 0; // For error logging

    while (file.ReadLine(strLine))
    {
      // Strip comments and spaces
      size_t comment_pos = strLine.find('#');
      strLine = strLine.substr(0, comment_pos);
      StringUtils::Trim(strLine);

      if (strLine.empty())
        continue;

      ChannelPtr l = ChannelPtr(new cChannel);
      if (l->DeserialiseConf(strLine))
      {
        AddChannel(l);
      }
      else
      {
        esyslog("Error loading config file %s, line %d", strFilename.c_str(), line);
      }

      line++;
    }
  }

  if (m_channels.empty())
  {
    esyslog("No channels loaded from %s", strFilename.c_str());
    return false;
  }

  isyslog("Loaded channels from %s, tracking %lu total channels", strFilename.c_str(), m_channels.size());

  ReNumber();
  return true;
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
    if (channel->GroupSep())
    {
      TiXmlElement sepElement(CHANNEL_XML_ELM_SEPARATOR);
      TiXmlNode *sepNode = root->InsertEndChild(sepElement);
      channel->SerialiseSep(sepNode);
    }
    else
    {
      TiXmlElement channelElement(CHANNEL_XML_ELM_CHANNEL);
      TiXmlNode *channelNode = root->InsertEndChild(channelElement);
      channel->SerialiseChannel(channelNode);
    }
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
    if (!channel->GroupSep())
    {
      if (channel->Number() == number)
        return channel;
      else if (skipGap && channel->Number() > number)
        return skipGap > 0 ? channel : previous;
      previous = channel;
    }
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByServiceID(int serviceID, int source, int transponder) const
{
  assert(serviceID);
  assert(source);
  assert(transponder);

  ChannelSidMap::const_iterator it = m_channelSids.find(serviceID);
  if (it != m_channelSids.end())
  {
    const ChannelVector &channelVec = it->second;
    for (ChannelVector::const_iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
    {
      if ((*itChannel)->Sid() == serviceID && (*itChannel)->Source() == source && ISTRANSPONDER((*itChannel)->Transponder(), transponder))
        return (*itChannel);
    }
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByChannelID(const tChannelID &channelID, bool bTryWithoutRid /* = false */, bool bTryWithoutPolarization /* = false */)
{
  int serviceID = channelID.Sid();
  ChannelSidMap::iterator it = m_channelSids.find(serviceID);
  if (it != m_channelSids.end())
  {
    ChannelVector &channelVec = it->second;
    for (ChannelVector::iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
    {
      ChannelPtr &channel = *itChannel;
      if (channel->Sid() == serviceID && channel->GetChannelID() == channelID)
        return channel;
    }

    tChannelID otherChannelID = channelID;
    if (bTryWithoutRid)
    {
      otherChannelID.ClrRid();
      for (ChannelVector::iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
      {
        ChannelPtr &channel = *itChannel;
        tChannelID myChannelID = channel->GetChannelID();
        myChannelID.ClrRid();
        if (channel->Sid() == serviceID && myChannelID == otherChannelID)
          return channel;
      }
    }

    if (bTryWithoutPolarization)
    {
      otherChannelID.ClrPolarization();
      for (ChannelVector::iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
      {
        ChannelPtr &channel = *itChannel;
        tChannelID myChannelID = channel->GetChannelID();
        myChannelID.ClrPolarization();
        if (channel->Sid() == serviceID && myChannelID == otherChannelID)
          return channel;
      }
    }
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByChannelID(int nid, int tid, int sid)
{
  ChannelSidMap::iterator it = m_channelSids.find(sid);
  if (it != m_channelSids.end())
  {
    ChannelVector &channelVec = it->second;
    for (ChannelVector::iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
    {
      ChannelPtr &channel = *itChannel;
      if (channel->Sid() == sid && channel->Tid() == tid && channel->Nid() == nid)
        return channel;
    }
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cChannelManager::GetByTransponderID(tChannelID channelID)
{
  int source = channelID.Source();
  int nid = channelID.Nid();
  int tid = channelID.Tid();
  CLockObject lock(m_mutex);
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    ChannelPtr &channel = *itChannel;
    if (channel->Nid() == nid && channel->Tid() == tid && channel->Source() == source)
      return channel;
  }
  return cChannel::EmptyChannel;
}

ChannelVector cChannelManager::GetByTransponder(int iSource, int iTransponder)
{
  ChannelVector channels;
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    ChannelPtr &channel = *itChannel;
    if (channel->Transponder() == iTransponder && channel->Source() == iSource)
      channels.push_back(channel);
  }
  return channels;
}

int cChannelManager::GetNextGroup(unsigned int index) const
{
  CLockObject lock(m_mutex);
  for (unsigned int i = index + 1; i < m_channels.size(); i++)
  {
    const ChannelPtr &channel = m_channels[i];
    if (channel->GroupSep() && !channel->Name().empty())
      return i;
  }
  return -1;
}

int cChannelManager::GetPrevGroup(unsigned int index) const
{
  if (index == 0)
    return -1;

  CLockObject lock(m_mutex);
  for (int i = index - 1; i >= 0; i--)
  {
    const ChannelPtr &channel = m_channels[i];
    if (channel->GroupSep() && !channel->Name().empty())
      return i;
  }
  return -1;
}

int cChannelManager::GetNextNormal(unsigned int index) const
{
  CLockObject lock(m_mutex);
  for (unsigned int i = index + 1; i < m_channels.size(); i++)
  {
    const ChannelPtr &channel = m_channels[i];
    if (!channel->GroupSep())
      return i;
  }
  return -1;
}

int cChannelManager::GetPrevNormal(unsigned int index) const
{
  if (index == 0)
    return -1;

  CLockObject lock(m_mutex);
  for (int i = index - 1; i >= 0; i--)
  {
    const ChannelPtr &channel = m_channels[i];
    if (!channel->GroupSep())
      return i;
  }
  return -1;
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
    if (channel->GroupSep())
      number = std::max(channel->Number(), number);
    else
    {
      m_channelSids[channel->Sid()].push_back(channel);
      m_maxNumber = number;
      channel->SetNumber(number++);
    }
  }
}

bool cChannelManager::HasUniqueChannelID(const ChannelPtr &newChannel, const ChannelPtr &oldChannel /* = cChannel::EmptyChannel */) const
{
  assert(newChannel.get());

  tChannelID newChannelID = newChannel->GetChannelID();
  CLockObject lock(m_mutex);
  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    const ChannelPtr &channel = *itChannel;
    if (!channel->GroupSep() && channel != oldChannel && channel->GetChannelID() == newChannelID)
      return false;
  }
  return true;
}

unsigned int cChannelManager::MaxChannelNameLength()
{
  CLockObject lock(m_mutex);
  if (!m_maxChannelNameLength)
  {
    for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
    {
      const ChannelPtr &channel = *itChannel;
      if (!channel->GroupSep())
        m_maxChannelNameLength = std::max(cUtf8Utils::Utf8StrLen(channel->Name().c_str()), (unsigned)m_maxChannelNameLength);
    }
  }
  return m_maxChannelNameLength;
}

unsigned int cChannelManager::MaxShortChannelNameLength()
{
  CLockObject lock(m_mutex);
  if (!m_maxShortChannelNameLength)
  {
    for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
    {
      const ChannelPtr &channel = *itChannel;
      if (!channel->GroupSep())
        m_maxShortChannelNameLength = std::max(cUtf8Utils::Utf8StrLen(channel->ShortName(true).c_str()), (unsigned)m_maxShortChannelNameLength);
    }
  }
  return m_maxShortChannelNameLength;
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

ChannelPtr cChannelManager::NewChannel(const cChannel& transponder, const string& name, const string& shortName, const string& provider, int nid, int tid, int sid, int rid /* = 0 */)
{
  dsyslog("creating new channel '%s,%s;%s' on %s transponder %d with id %d-%d-%d-%d", name.c_str(), shortName.c_str(), provider.c_str(), cSource::ToString(transponder.Source()).c_str(), transponder.Transponder(), nid, tid, sid, rid);
  ChannelPtr newChannel = ChannelPtr(new cChannel);
  if (newChannel)
  {
    newChannel->CopyTransponderData(transponder);
    newChannel->SetId(nid, tid, sid, rid);
    newChannel->SetName(name, shortName, provider);
    AddChannel(newChannel);
    ReNumber();
  }
  return newChannel;
}

void cChannelManager::AddTransponders(cScanList* list) const
{
  for (std::vector<ChannelPtr>::const_iterator it = m_channels.begin(); it != m_channels.end(); it++)
    list->AddTransponder(*it);
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
    if(channelUID == channel->Hash()) {
      result = channel;
      break;
    }
  }

  return result;
}

vector<ChannelPtr> cChannelManager::GetCurrent(void) const
{
  PLATFORM::CLockObject lock(m_mutex);
  return m_channels;
}

void cChannelManager::CreateChannelGroups(bool automatic)
{
  std::string groupname;

  CLockObject lock(m_mutex);
  for (std::vector<ChannelPtr>::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it)
  {
    ChannelPtr channel = *it;
    bool isRadio = CChannelFilter::IsRadio(channel);

    if(automatic && !channel->GroupSep())
      groupname = channel->Provider();
    else if(!automatic && channel->GroupSep())
      groupname = channel->Name();

    if(groupname.empty())
      continue;

    if (!CChannelGroups::Get(isRadio).HasGroup(groupname))
    {
      CChannelGroup group(isRadio, groupname, automatic);
      CChannelGroups::Get(isRadio).AddGroup(group);
    }
  }
}
