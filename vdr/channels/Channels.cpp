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

cChannels Channels;

// --- cChannelSorter --------------------------------------------------------

class cChannelSorter : public cListObject
{
public:
  cChannel *channel;
  tChannelID channelID;

  cChannelSorter(cChannel *Channel)
  {
    channel = Channel;
    channelID = channel->GetChannelID();
  }

  virtual int Compare(const cListObject &ListObject) const
  {
    cChannelSorter *cs = (cChannelSorter *)&ListObject;
    return memcmp(&channelID, &cs->channelID, sizeof(channelID));
  }
};

cChannels::cChannels()
{
  maxNumber = 0;
  maxChannelNameLength = 0;
  maxShortChannelNameLength = 0;
  modified = CHANNELSMOD_NONE;
}

void cChannels::DeleteDuplicateChannels()
{
  cList<cChannelSorter> ChannelSorter;
  for (cChannel *channel = First(); channel; channel = Next(channel))
  {
    if (!channel->GroupSep())
      ChannelSorter.Add(new cChannelSorter(channel));
  }

  ChannelSorter.Sort();
  cChannelSorter *cs = ChannelSorter.First();

  while (cs)
  {
    cChannelSorter *next = ChannelSorter.Next(cs);
    if (next && cs->channelID == next->channelID)
    {
      dsyslog("deleting duplicate channel %s", *next->channel->ToText());
      Del(next->channel);
    }
    cs = next;
  }
}

bool cChannels::Load(const char *FileName, bool AllowComments, bool MustExist)
{
  if (cConfig<cChannel>::Load(FileName, AllowComments, MustExist))
  {
    DeleteDuplicateChannels();
    ReNumber();
    return true;
  }
  return false;
}

void cChannels::HashChannel(cChannel *Channel)
{
  channelsHashSid.Add(Channel, Channel->Sid());
}

void cChannels::UnhashChannel(cChannel *Channel)
{
  channelsHashSid.Del(Channel, Channel->Sid());
}

int cChannels::GetNextGroup(int Idx)
{
  cChannel *channel = Get(++Idx);
  while (channel && !(channel->GroupSep() && *channel->Name()))
    channel = Get(++Idx);
  return channel ? Idx : -1;
}

int cChannels::GetPrevGroup(int Idx)
{
  cChannel *channel = Get(--Idx);
  while (channel && !(channel->GroupSep() && *channel->Name()))
    channel = Get(--Idx);
  return channel ? Idx : -1;
}

int cChannels::GetNextNormal(int Idx)
{
  cChannel *channel = Get(++Idx);
  while (channel && channel->GroupSep())
    channel = Get(++Idx);
  return channel ? Idx : -1;
}

int cChannels::GetPrevNormal(int Idx)
{
  cChannel *channel = Get(--Idx);
  while (channel && channel->GroupSep())
    channel = Get(--Idx);
  return channel ? Idx : -1;
}

void cChannels::ReNumber()
{
  channelsHashSid.Clear();
  maxNumber = 0;
  int Number = 1;
  for (cChannel *channel = First(); channel; channel = Next(channel))
  {
    if (channel->GroupSep())
    {
      if (channel->Number() > Number)
        Number = channel->Number();
    }
    else
    {
      HashChannel(channel);
      maxNumber = Number;
      channel->SetNumber(Number++);
    }
  }
}

cChannel *cChannels::GetByNumber(int Number, int SkipGap)
{
  cChannel *previous = NULL;
  for (cChannel *channel = First(); channel; channel = Next(channel))
  {
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
        if (channel->Sid() == sid && channel->GetChannelID().ClrRid() == ChannelID)
          return channel;
      }
    }

    if (TryWithoutPolarization)
    {
      ChannelID.ClrPolarization();
      for (cHashObject *hobj = list->First(); hobj; hobj = list->Next(hobj))
      {
        cChannel *channel = (cChannel *)hobj->Object();
        if (channel->Sid() == sid && channel->GetChannelID().ClrPolarization() == ChannelID)
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
  for (cChannel *channel = First(); channel; channel = Next(channel))
  {
    if (channel->Tid() == tid && channel->Nid() == nid && channel->Source() == source)
      return channel;
  }
  return NULL;
}

bool cChannels::HasUniqueChannelID(cChannel *NewChannel, cChannel *OldChannel)
{
  tChannelID NewChannelID = NewChannel->GetChannelID();
  for (cChannel *channel = First(); channel; channel = Next(channel))
  {
    if (!channel->GroupSep() && channel != OldChannel && channel->GetChannelID() == NewChannelID)
      return false;
  }
  return true;
}

bool cChannels::SwitchTo(int Number)
{
  cChannel *channel = GetByNumber(Number);
  return channel && cDeviceManager::PrimaryDevice()->SwitchChannel(channel, true);
}

int cChannels::MaxChannelNameLength()
{
  if (!maxChannelNameLength)
  {
    for (cChannel *channel = First(); channel; channel = Next(channel))
    {
      if (!channel->GroupSep())
        maxChannelNameLength = max(cUtf8Utils::Utf8StrLen(channel->Name()), maxChannelNameLength);
    }
  }
  return maxChannelNameLength;
}

int cChannels::MaxShortChannelNameLength()
{
  if (!maxShortChannelNameLength)
  {
    for (cChannel *channel = First(); channel; channel = Next(channel))
    {
      if (!channel->GroupSep())
        maxShortChannelNameLength = max(cUtf8Utils::Utf8StrLen(channel->ShortName(true)), maxShortChannelNameLength);
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

cChannel *cChannels::NewChannel(const cChannel *Transponder, const char *Name, const char *ShortName, const char *Provider, int Nid, int Tid, int Sid, int Rid)
{
  if (Transponder)
  {
    dsyslog("creating new channel '%s,%s;%s' on %s transponder %d with id %d-%d-%d-%d", Name, ShortName, Provider, *cSource::ToString(Transponder->Source()), Transponder->Transponder(), Nid, Tid, Sid, Rid);
    cChannel *NewChannel = new cChannel;
    NewChannel->CopyTransponderData(Transponder);
    NewChannel->SetId(Nid, Tid, Sid, Rid);
    NewChannel->SetName(Name, ShortName, Provider);
    Add(NewChannel);
    ReNumber();
    return NewChannel;
  }
  return NULL;
}

cString ChannelString(const cChannel *Channel, int Number)
{
  char buffer[256];
  if (Channel)
  {
    if (Channel->GroupSep())
      snprintf(buffer, sizeof(buffer), "%s", Channel->Name());
    else
      snprintf(buffer, sizeof(buffer), "%d%s  %s", Channel->Number(), Number ? "-" : "", Channel->Name());
  }
  else if (Number)
    snprintf(buffer, sizeof(buffer), "%d-", Number);
  else
    snprintf(buffer, sizeof(buffer), "%s", tr("*** Invalid Channel ***"));
  return buffer;
}
