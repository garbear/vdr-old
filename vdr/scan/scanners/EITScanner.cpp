/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "EITScanner.h"
#include "channels/Channel.h"
#include "channels/ChannelManager.h"
#include "utils/DateTime.h"

#include <libsi/si.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>
#include <libsi/descriptor.h>

using namespace SI_EXT;

namespace VDR
{

// --- cEitParser ------------------------------------------------------------------

class cEitParser : public SI::EIT
{
public:
  cEitParser(cChannelManager& channelManager, int Source, u_char Tid, const u_char *Data);
};

cEitParser::cEitParser(cChannelManager& channelManager, int Source, u_char Tid, const u_char *Data)
 : SI::EIT(Data, false)
{
  if (!CheckCRCAndParse())
    return;

  tChannelID channelID(Source, getOriginalNetworkId(), getTransportStreamId(), getServiceId());
  ChannelPtr channel = channelManager.GetByChannelID(channelID, true);
  if (!channel)
    return; // only collect data for known channels

  SI::EIT::Event SiEitEvent;
  for (SI::Loop::Iterator it; eventLoop.getNext(SiEitEvent, it); )
  {
    bool ExternalData = false;
    // Drop bogus events - but keep NVOD reference events, where all bits of the start time field are set to 1, resulting in a negative number.
    if ((SiEitEvent.getStartTime() == 0) || (SiEitEvent.getStartTime() > 0 && SiEitEvent.getDuration() == 0))
      continue;

    SI::Descriptor *d;
    cLinkChannels *LinkChannels = NULL;

    for (SI::Loop::Iterator it2; (d = SiEitEvent.eventDescriptors.getNext(it2)); )
    {
      if (ExternalData && d->getDescriptorTag() != SI::ComponentDescriptorTag)
      {
        DELETENULL(d);
        continue;
      }
      switch (d->getDescriptorTag())
      {
        case SI::LinkageDescriptorTag:
        {
          dsyslog("LinkageDescriptorTag");
          SI::LinkageDescriptor *ld = (SI::LinkageDescriptor *)d;
          tChannelID linkID(Source, ld->getOriginalNetworkId(), ld->getTransportStreamId(), ld->getServiceId());

          // Premiere World
          if (ld->getLinkageType() == 0xB0)
          {
            CDateTime now = CDateTime::GetUTCDateTime();
            CDateTime eitStart = CDateTime(SiEitEvent.getStartTime()).GetAsUTCDateTime();
            bool hit = eitStart <= now && now < eitStart + CDateTimeSpan(0, 0, 0, SiEitEvent.getDuration());
            if (hit)
            {
              char linkName[ld->privateData.getLength() + 1];
              strn0cpy(linkName, (const char *)ld->privateData.getData(), sizeof(linkName));
              ChannelPtr link = channelManager.GetByChannelID(linkID);

              // Only link to other channels, not the same one
              if (link != channel)
              {
                if (link)
                {
                  link->SetName(linkName, "", "");
                }
                else
                {
                  link = channelManager.NewChannel(*channel, linkName, "", "", ld->getOriginalNetworkId(), ld->getTransportStreamId(), ld->getServiceId());
                }
                if (link)
                {
                  if (!LinkChannels)
                    LinkChannels = new cLinkChannels;
                  LinkChannels->push_back(new cLinkChannel(link));
                }
              }
              else
                channel->SetPortalName(linkName);
            }
          }
          break;
        }
        default:
          break;
      }
      DELETENULL(d);
    }

    if (LinkChannels)
      channel->SetLinkChannels(*LinkChannels);
  }
}

cEitScanner::cEitScanner(cChannelManager channelManager)
 : m_channelManager(channelManager)
{
  Set(PID_EIT, TABLE_ID_EIT_ACTUAL_PRESENT,        0xFE);  // actual(0x4E)/other(0x4F) TS, present/following
  Set(PID_EIT, TABLE_ID_EIT_ACTUAL_SCHEDULE_START, 0xF0);  // actual TS, schedule(0x50)/schedule for future days(0x5X)
  Set(PID_EIT, TABLE_ID_EIT_OTHER_SCHEDULE_START,  0xF0);  // other  TS, schedule(0x60)/schedule for future days(0x6X)
}

cEitScanner::~cEitScanner()
{
  Del(PID_EIT, TABLE_ID_EIT_ACTUAL_PRESENT);
  Del(PID_EIT, TABLE_ID_EIT_ACTUAL_SCHEDULE_START);
  Del(PID_EIT, TABLE_ID_EIT_OTHER_SCHEDULE_START);
}

void cEitScanner::ProcessData(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  cEitParser EitParser(m_channelManager, Source(), Tid, Data);
}

}
