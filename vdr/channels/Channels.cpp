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

#include "Channels.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "utils/StringUtils.h"
#include "utils/UTF8Utils.h"
#include "dvb/EITScan.h"

#include <algorithm>
#include <assert.h>

using namespace std;

cChannels Channels;

cChannels::cChannels()
 : maxNumber(0),
   maxChannelNameLength(0),
   maxShortChannelNameLength(0),
   modified(CHANNELSMOD_NONE),
   beingEdited(0)
{
}

void cChannels::Clear()
{
  m_fileName.clear();
  for (vector<cChannel*>::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
    delete *itChannel;
  m_channels.clear();
}

void cChannels::Notify(const Observable &obs, const ObservableMessage msg)
{
  //TODO
}

void cChannels::AddChannel(cChannel* channel)
{
  assert(channel);

  // If the channel is a duplicate, delete it instead of adding to the vector
  for (vector<cChannel*>::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if (*itChannel == channel)
    {
      delete channel;
      return;
    }
  }

  channel->RegisterObserver(this);
  m_channels.push_back(channel);
}

void cChannels::RemoveChannel(cChannel* channel)
{
  assert(channel);

  channel->UnregisterObserver(this);
  for (vector<cChannel*>::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    if (channel == *itChannel)
    {
      m_channels.erase(itChannel);
      break;
    }
  }
  delete channel;
}

bool cChannels::Load(const string &fileName, bool bMustExist /* = false */)
{
  Clear();
  bool bAllowComments = false;
  if (!fileName.empty())
  {
    m_fileName = fileName;
    bAllowComments = true;
  }

  bool result = !bMustExist;

  if (!m_fileName.empty() && access(m_fileName.c_str(), F_OK) == 0)
  {
    isyslog("loading %s", m_fileName.c_str());
    FILE *f = fopen(m_fileName.c_str(), "r");
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
          cChannel *l = new cChannel;
          if (l->Deserialise(s))
            AddChannel(l);
          else
          {
            //esyslog("ERROR: error in %s, line %d", m_fileName.c_str(), line);
            delete l;
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

  if (result)
  {
    ReNumber();
    return true;
  }
  return false;
}

bool cChannels::Save(void)
{
  bool result = true;
  //cChannel* l = First();
  cSafeFile f(m_fileName.c_str());
  if (f.Open())
  {
    for (vector<cChannel*>::iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
    {
      if (!(*itChannel)->Save(f))
      {
        result = false;
        break;
      }
    }
    if (!f.Close())
      result = false;
  }
  else
    result = false;

  return result;
}

void cChannels::HashChannel(cChannel *Channel)
{
  channelsHashSid.Add(Channel, Channel->Sid());
}

void cChannels::UnhashChannel(cChannel *Channel)
{
  channelsHashSid.Del(Channel, Channel->Sid());
}

int cChannels::GetNextGroup(unsigned int index)
{
  for (vector<cChannel*>::const_iterator itChannel = m_channels.begin() + index + 1; itChannel != m_channels.end(); ++itChannel)
  {
    if ((*itChannel)->GroupSep() && !(*itChannel)->Name().empty())
    {
      ++itChannel;
      return itChannel != m_channels.end() ? m_channels.begin() - itChannel + 1 : -1;
    }
  }
  return -1;
}

int cChannels::GetPrevGroup(unsigned int index)
{
  if (index == 0)
    return -1;

  for (vector<cChannel*>::const_iterator itChannel = m_channels.begin() + index - 1; itChannel != m_channels.begin(); --itChannel)
  {
    if ((*itChannel)->GroupSep() && !(*itChannel)->Name().empty())
    {
      --itChannel;
      return itChannel != m_channels.begin() ? m_channels.begin() - itChannel - 1 : -1;
    }
  }
  return -1;
}

int cChannels::GetNextNormal(unsigned int index)
{
  for (vector<cChannel*>::const_iterator itChannel = m_channels.begin() + index + 1; itChannel != m_channels.end(); ++itChannel)
  {
    if ((*itChannel)->GroupSep())
    {
      ++itChannel;
      return itChannel != m_channels.end() ? m_channels.begin() - itChannel + 1 : -1;
    }
  }
  return -1;
}

int cChannels::GetPrevNormal(unsigned int index)
{
  if (index == 0)
    return -1;

  for (vector<cChannel*>::const_iterator itChannel = m_channels.begin() + index - 1; itChannel != m_channels.begin(); --itChannel)
  {
    if ((*itChannel)->GroupSep())
    {
      --itChannel;
      return itChannel != m_channels.begin() ? m_channels.begin() - itChannel - 1 : -1;
    }
  }
  return -1;
}

void cChannels::ReNumber()
{
  channelsHashSid.Clear();
  maxNumber = 0;
  int number = 1;
  for (vector<cChannel*>::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    cChannel *channel = *itChannel;
    if (channel->GroupSep())
    {
      if (channel->Number() > number)
        number = channel->Number();
    }
    else
    {
      HashChannel(channel);
      maxNumber = number;
      channel->SetNumber(number++);
    }
  }
}

cChannel *cChannels::GetByNumber(int Number, int SkipGap)
{
  cChannel *previous = NULL;
  for (vector<cChannel*>::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    cChannel *channel = *itChannel;
    if (!channel->GroupSep())
    {
      if (channel->Number() == Number)
        return channel;
      else if (SkipGap && channel->Number() > Number)
        return SkipGap > 0 ? channel : previous;
      previous = channel;
    }
  }
  return NULL;
}

cChannel *cChannels::GetByServiceID(int Source, int Transponder, unsigned short ServiceID)
{
  cList<cHashObject> *list = channelsHashSid.GetList(ServiceID);
  if (list)
  {
    for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj))
    {
      cChannel *channel = (cChannel *)hobj->Object();
      if (channel->Sid() == ServiceID && channel->Source() == Source && ISTRANSPONDER(channel->Transponder(), Transponder))
        return channel;
    }
  }
  return NULL;
}

cChannel *cChannels::GetByChannelID(tChannelID ChannelID, bool TryWithoutRid, bool TryWithoutPolarization)
{
  int sid = ChannelID.Sid();
  cList<cHashObject> *list = channelsHashSid.GetList(sid);
  if (list)
  {
    for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj))
    {
      cChannel *channel = (cChannel *)hobj->Object();
      if (channel->Sid() == sid && channel->GetChannelID() == ChannelID)
        return channel;
    }

    if (TryWithoutRid)
    {
      ChannelID.ClrRid();
      for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj))
      {
        cChannel *channel = (cChannel *)hobj->Object();
        channel->GetChannelID().ClrRid();
        if (channel->Sid() == sid && channel->GetChannelID() == ChannelID)
          return channel;
      }
    }

    if (TryWithoutPolarization)
    {
      ChannelID.ClrPolarization();
      for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj))
      {
        cChannel *channel = (cChannel *)hobj->Object();
        channel->GetChannelID().ClrPolarization();
        if (channel->Sid() == sid && channel->GetChannelID() == ChannelID)
          return channel;
      }
    }
  }
  return NULL;
}

cChannel *cChannels::GetByChannelID(int nid, int tid, int sid)
{
  cList<cHashObject> *list = channelsHashSid.GetList(sid);
  if (list)
  {
    for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj))
    {
      cChannel *channel = (cChannel *)hobj->Object();
      if (channel->Sid() == sid && channel->Tid() == tid && channel->Nid() == nid)
        return channel;
    }
  }
  return NULL;
}

cChannel *cChannels::GetByTransponderID(tChannelID ChannelID)
{
  int source = ChannelID.Source();
  int nid = ChannelID.Nid();
  int tid = ChannelID.Tid();
  for (vector<cChannel*>::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    cChannel *channel = *itChannel;
    if (channel->Nid() == nid && channel->Tid() == tid && channel->Source() == source)
      return channel;
  }
  return NULL;
}

bool cChannels::HasUniqueChannelID(cChannel *NewChannel, cChannel *OldChannel)
{
  tChannelID NewChannelID = NewChannel->GetChannelID();
  for (vector<cChannel*>::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
  {
    cChannel *channel = *itChannel;
    if (!channel->GroupSep() && channel != OldChannel && channel->GetChannelID() == NewChannelID)
      return false;
  }
  return true;
}

bool cChannels::SwitchTo(int Number)
{
  cChannel *channel = GetByNumber(Number);
  return channel && cDeviceManager::Get().PrimaryDevice()->Channel()->SwitchChannel(*channel, true);
}

int cChannels::MaxChannelNameLength()
{
  if (!maxChannelNameLength)
  {
    for (vector<cChannel*>::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
    {
      cChannel *channel = *itChannel;
      if (!channel->GroupSep())
        maxChannelNameLength = std::max(cUtf8Utils::Utf8StrLen(channel->Name().c_str()), (unsigned)maxChannelNameLength);
    }
  }
  return maxChannelNameLength;
}

int cChannels::MaxShortChannelNameLength()
{
  if (!maxShortChannelNameLength)
  {
    for (vector<cChannel*>::const_iterator itChannel = m_channels.begin(); itChannel != m_channels.end(); ++itChannel)
    {
      cChannel *channel = *itChannel;
      if (!channel->GroupSep())
        maxShortChannelNameLength = std::max(cUtf8Utils::Utf8StrLen(channel->ShortName(true).c_str()), (unsigned)maxShortChannelNameLength);
    }
  }
  return maxShortChannelNameLength;
}

void cChannels::SetModified(bool ByUser)
{
  modified = ByUser ? CHANNELSMOD_USER : !modified ? CHANNELSMOD_AUTO : modified;
  maxChannelNameLength = maxShortChannelNameLength = 0;
}

int cChannels::Modified()
{
  int Result = modified;
  modified = CHANNELSMOD_NONE;
  return Result;
}

cChannel *cChannels::NewChannel(const cChannel& Transponder, const std::string& Name, const std::string& ShortName, const std::string& Provider, int Nid, int Tid, int Sid, int Rid)
{
  dsyslog("creating new channel '%s,%s;%s' on %s transponder %d with id %d-%d-%d-%d", Name.c_str(), ShortName.c_str(), Provider.c_str(), cSource::ToString(Transponder.Source()).c_str(), Transponder.Transponder(), Nid, Tid, Sid, Rid);
  cChannel *NewChannel = new cChannel;
  if (NewChannel)
  {
    NewChannel->CopyTransponderData(Transponder);
    NewChannel->SetId(Nid, Tid, Sid, Rid);
    NewChannel->SetName(Name, ShortName, Provider);
    AddChannel(NewChannel);
    ReNumber();
  }
  return NewChannel;
}

string ChannelString(const cChannel *Channel, int Number)
{
  string retval;
  if (Channel)
  {
    if (Channel->GroupSep())
      retval = StringUtils::Format("%s", Channel->Name().c_str());
    else
      retval = StringUtils::Format("%d%s  %s", Channel->Number(), Number ? "-" : "", Channel->Name().c_str());
  }
  else if (Number)
    retval = StringUtils::Format("%d-", Number);
  else
    retval = StringUtils::Format("%s", tr("*** Invalid Channel ***"));
  return retval;
}

void cChannels::AddTransponders(cScanList* list) const
{
  for (std::vector<cChannel*>::const_iterator it = m_channels.begin(); it != m_channels.end(); it++)
    list->AddTransponder(*it);
}
