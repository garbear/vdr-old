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
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "dvb/EITScan.h"
//#include "utils/I18N.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/UTF8Utils.h"
#include "utils/CRC32.h"

#include <algorithm>
#include <assert.h>

using namespace std;
using namespace VDR;

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
   m_maxShortChannelNameLength(0),
   m_modified(CHANNELSMOD_NONE),
   m_beingEdited(0)
{
}

cChannelManager &cChannelManager::Get()
{
  static cChannelManager instance;
  return instance;
}

void cChannelManager::Clear(void)
{
  m_channels.clear();
  m_channelSids.clear();
}

void cChannelManager::Notify(const Observable &obs, const ObservableMessage msg)
{
  //TODO
}

void cChannelManager::AddChannel(ChannelPtr channel)
{
  assert(channel.get());

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

bool cChannelManager::Load(const std::string &file)
{
  Clear();

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    //CLog::Log(LOGERROR, "CSettings: error loading settings definition from %s, Line %d\n%s", file.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    //cout << "Error loading channels from " << file << ", Line " << xmlDoc.ErrorRow() << endl;
    //cout << xmlDoc.ErrorDesc() << endl;
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

bool cChannelManager::LoadConf(const string &file)
{
  Clear();

  bool bAllowComments = false;
  if (!file.empty())
    bAllowComments = true;

  bool result = true;

  if (!file.empty() && access(file.c_str(), F_OK) == 0)
  {
    isyslog("loading %s", file.c_str());
    FILE *f = fopen(file.c_str(), "r");
    if (f)
    {
      char *s;
      int line = 0;
      cReadLine ReadLine;
      result = true;
      while ((s = ReadLine.Read(f)) != NULL)
      {
        line++;
        if (bAllowComments)
        {
          char *p = strchr(s, '#');
          if (p)
            *p = 0;
        }
        stripspace(s);
        if (!isempty(s))
        {
          ChannelPtr l = ChannelPtr(new cChannel);
          if (l->DeserialiseConf(s))
            AddChannel(l);
          else
          {
            //esyslog("ERROR: error in %s, line %d", m_fileName.c_str(), line);
            result = false;
          }
        }
      }
      fclose(f);
    }
    else
    {
      //LOG_ERROR_STR(m_fileName.c_str());
      result = false;
    }
  }

  /*
  if (!result)
    fprintf(stderr, "vdr: error while reading '%s'\n", m_fileName.c_str());
  */

  if (!result)
    return false;

  ReNumber();
  return true;
}

bool cChannelManager::Save(const string &file)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(CHANNEL_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

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

  return xmlDoc.SaveFile(file);
}

bool cChannelManager::SaveConf(const string &file)
{
  bool result = true;
  //cChannel* l = First();
  cSafeFile f(file.c_str());
  if (f.Open())
  {
    for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
    {
      const ChannelPtr &channel = *itChannel;
      if (!channel->SaveConf(f))
      {
        result = false;
        break;
      }
    }
    f.Close();
  }
  else
    result = false;

  return result;
}

ChannelPtr cChannelManager::GetByNumber(int number, int skipGap /* = 0 */)
{
  ChannelPtr previous;
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
  return ChannelPtr();
}

ChannelPtr cChannelManager::GetByServiceID(unsigned short serviceID, int source, int transponder)
{
  ChannelSidMap::iterator it = m_channelSids.find(serviceID);
  if (it != m_channelSids.end())
  {
    ChannelVector &channelVec = it->second;
    for (ChannelVector::iterator itChannel = channelVec.begin(); itChannel != channelVec.end(); ++itChannel)
    {
      ChannelPtr &channel = *itChannel;
      if (channel->Sid() == serviceID && channel->Source() == source && ISTRANSPONDER(channel->Transponder(), transponder))
        return channel;
    }
  }
  return ChannelPtr();
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
  return ChannelPtr();
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
  return ChannelPtr();
}

ChannelPtr cChannelManager::GetByTransponderID(tChannelID channelID)
{
  int source = channelID.Source();
  int nid = channelID.Nid();
  int tid = channelID.Tid();
  for (ChannelVector::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    ChannelPtr &channel = *itChannel;
    if (channel->Nid() == nid && channel->Tid() == tid && channel->Source() == source)
      return channel;
  }
  return ChannelPtr();
}

int cChannelManager::GetNextGroup(unsigned int index) const
{
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

bool cChannelManager::HasUniqueChannelID(const ChannelPtr &newChannel, const ChannelPtr &oldChannel /* = ChannelPtr() */) const
{
  assert(newChannel.get());

  tChannelID newChannelID = newChannel->GetChannelID();
  for (ChannelVector::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    const ChannelPtr &channel = *itChannel;
    if (!channel->GroupSep() && channel != oldChannel && channel->GetChannelID() == newChannelID)
      return false;
  }
  return true;
}

bool cChannelManager::SwitchTo(int number)
{
  ChannelPtr channel = GetByNumber(number);
  return channel.get() ;//&& cDeviceManager::Get().PrimaryDevice()->Channel()->SwitchChannel(*channel, true);
}

unsigned int cChannelManager::MaxChannelNameLength()
{
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

eLastModifiedType cChannelManager::Modified()
{
  eLastModifiedType result = m_modified;
  m_modified = CHANNELSMOD_NONE;
  return result;
}

void cChannelManager::SetModified(bool bByUser /* = false */)
{
  m_maxChannelNameLength = m_maxShortChannelNameLength = 0;

  if (bByUser)
    m_modified = CHANNELSMOD_USER;
  else if (m_modified == CHANNELSMOD_NONE)
    m_modified = CHANNELSMOD_AUTO;
}

ChannelPtr cChannelManager::NewChannel(const cChannel& transponder, const string& name, const string& shortName, const string& provider, int nid, int tid, int sid, int rid /* = 0 */)
{
  //dsyslog("creating new channel '%s,%s;%s' on %s transponder %d with id %d-%d-%d-%d", name.c_str(), shortName.c_str(), provider.c_str(), cSource::ToString(transponder.Source()).c_str(), transponder.Transponder(), nid, tid, sid, rid);
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
  /*
  for (std::vector<ChannelPtr>::const_iterator it = m_channels.begin(); it != m_channels.end(); it++)
    list->AddTransponder(it->get());
  */
}

ChannelPtr cChannelManager::GetByChannelUID(uint32_t channelUID) const
{
  ChannelPtr result;
  ChannelPtr channel;

  // maybe we need to use a lookup table
  for (ChannelVector::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it)
  {
    channel = (*it);
    if(channelUID == CCRC32::CRC32(channel->GetChannelID().Serialize())) {
      result = channel;
      break;
    }
  }

  return result;
}
