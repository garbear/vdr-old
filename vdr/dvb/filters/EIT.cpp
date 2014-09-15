/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#include "EIT.h"
#include "channels/Channel.h"
#include "channels/ChannelID.h"
#include "channels/ChannelManager.h"
#include "epg/Component.h"
#include "epg/Event.h"
#include "epg/ScheduleManager.h"
#include "settings/Settings.h"
#include "utils/CommonMacros.h"
#include "utils/DateTime.h"
#include "utils/I18N.h"

#include <assert.h>
#include <libsi/dish.h>
#include <libsi/si_ext.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

#define DESCRIPTION_SEPARATOR  ": "

// From UTF8Utils.h
#ifndef Utf8BufSize
#define Utf8BufSize(s)  ((s) * 4)
#endif

namespace VDR
{

cEit::cEit(cDevice* device, cChannelManager& channelManager)
 : cFilter(device),
   m_channelManager(channelManager),
   m_extendedEventDescriptors(NULL)
{
  OpenResource(PID_EIT, TableIdEIT_presentFollowing,     0xFE); // actual(0x4E) / other(0x4F) TS, present / following
  OpenResource(PID_EIT, TableIdEIT_schedule_first,       0xF0); // actual TS, schedule(0x50) / schedule for future days (0x5X)
  OpenResource(PID_EIT, TableIdEIT_schedule_Other_first, 0xF0); // other  TS, schedule(0x60) / schedule for future days (0x6X)
  OpenResource(0x0300,  TableIdPAT,                      0x00); // Dish Network EEPG
  OpenResource(0x0441,  TableIdPAT,                      0x00); // Bell ExpressVU EEPG
}

EventVector cEit::GetEvents()
{
  EventVector events;

  uint16_t        pid;  // Packet ID
  vector<uint8_t> data; // Section data
  if (GetSection(pid, data) && (pid == PID_EIT ||
                                pid == 0x0300  ||
                                pid == 0x0441))
  {
    SI::EIT tsEIT(data.data());
    if (tsEIT.CheckCRCAndParse() /* && tsEIT.getTableId() >= SI::TableIdEIT_presentFollowing */) // TODO: Do we need this check?
    {
      // We need the current time for handling PDC descriptors
      CDateTime now = CDateTime::GetUTCDateTime();
      /*
      ChannelPtr channel = m_channelManager.GetByChannelID(tsEIT.getOriginalNetworkId(),
                                                           tsEIT.getTransportStreamId(),
                                                           tsEIT.getServiceId());
      */
      ChannelPtr channel = ChannelPtr(new cChannel);
      channel->SetId(tsEIT.getOriginalNetworkId(), tsEIT.getTransportStreamId(), tsEIT.getServiceId());

      if (now.IsValid())
      {
        SI::EIT::Event eitEvent;
        for (SI::Loop::Iterator it; tsEIT.eventLoop.getNext(eitEvent, it); )
        {
          // Drop bogus events - but keep NVOD reference events, where all bits of
          // the start time field are set to 1, resulting in a negative number
          CDateTime startTime(eitEvent.getStartTime());
          int iDuration = eitEvent.getDuration();
          if (!startTime.IsValid() || (startTime.IsValid() && iDuration == 0))
            continue;

          /*
          if (!segmentStart.IsValid())
            segmentStart = startTime;

          // Keep advancing segmentEnd
          segmentEnd = startTime + CDateTimeSpan(0, 0, 0, iDuration);
          */


          /*
          newEvent = NULL;
          rEvent = NULL;
          pEvent = schedule->GetEvent(eitEvent.getEventId(), startTime);

          if (!pEvent)
          {
            // If we don't have that event yet, we create a new one
            pEvent = newEvent = new cEvent(eitEvent.getEventId());
            newEvent->SetStartTime(startTime);
            newEvent->SetDuration(iDuration);
            //schedule->AddEvent(newEvent);
          }
          else
          {
            // We have found an existing event, either through its event ID or its start time
            pEvent->SetSeen();

            // If the new event has a higher table ID, let's skip it. The lower
            // the table ID, the more "current" the information.
            // For backwards compatibility, table IDs less than 0x4E are treated
            // as if they were "present"
            uint8_t tableID = std::max(pEvent->TableID(), (uint8_t)0x4E);
            if (tid > tableID)
            {
              bHasEvents |= true;
              continue;
            }
            // If the new event comes from the same table and has the same version number
            // as the existing one, let's skip it to avoid unnecessary work.
            // Unfortunately some stations (like, e.g. "Premiere") broadcast their EPG data on several transponders (like
            // the actual Premiere transponder and the Sat.1/Pro7 transponder), but use different version numbers on
            // each of them :-( So if one DVB card is tuned to the Premiere transponder, while an other one is tuned
            // to the Sat.1/Pro7 transponder, events will keep toggling because of the bogus version numbers.
            else if (tid == tableID && pEvent->Version() == tsEIT.getVersionNumber())
            {
              bHasEvents |= true;
              continue;
            }

            pEvent->SetEventID(eitEvent.getEventId());
            pEvent->SetStartTime(startTime);
            pEvent->SetDuration(iDuration);
          }
          */

          /*
          // For backwards compatibility, table IDs less than 0x4E are never overwritten
          if (pEvent->TableID() > 0x4E)
            pEvent->SetTableID(tid);
          */

          EventPtr thisEvent = EventPtr(new cEvent(eitEvent.getEventId()));
          thisEvent->SetStartTime(startTime);
          thisEvent->SetEndTime(startTime + CDateTimeSpan(0, 0, 0, iDuration));
          thisEvent->SetTableID(tsEIT.getTableId());
          thisEvent->SetVersion(tsEIT.getVersionNumber());

          // We trust only the present/following info on the actual TS
          /* TODO
          if (tsEIT.getTableId() == 0x4E && eitEvent.getRunningStatus() >= SI::RunningStatusNotRunning)
            thisEvent->SetRunningStatus(eitEvent.getRunningStatus());
          */

          string                strTitle;
          string                strShortText;
          string                strDescription;
          vector<CEpgComponent> components;
          vector<uint8_t>       contents;
          cLinkChannels         linkChannels;

          SI::Descriptor* d;
          for (SI::Loop::Iterator it2; (d = eitEvent.eventDescriptors.getNext(it2)); )
          {
            switch (d->getDescriptorTag())
            {
            case SI::ExtendedEventDescriptorTag:
            case SI::DishExtendedEventDescriptorTag:
            case SI::DishShortEventDescriptorTag:
            case SI::ShortEventDescriptorTag:
            case SI::TimeShiftedEventDescriptorTag:
              GetText(d, tsEIT.getTableId(), tsEIT.getOriginalNetworkId(), tsEIT.getTransportStreamId(),
                  strTitle, strShortText, strDescription);
              break;
            case SI::ContentDescriptorTag:
              GetContents((SI::ContentDescriptor*)d, contents);
              break;
            /*
            case SI::ParentalRatingDescriptorTag:
            {
              SI::ParentalRatingDescriptor* prd = (SI::ParentalRatingDescriptor*)d;
              int languagePreferenceRating = -1;

              SI::ParentalRatingDescriptor::Rating rating;
              for (SI::Loop::Iterator it3; prd->ratingLoop.getNext(rating, it3); )
              {
                if (I18nIsPreferredLanguage(cSettings::Get().m_EPGLanguages, rating.languageCode, languagePreferenceRating))
                {
                  int parentalRating = (rating.getRating() & 0xFF);

                  switch (parentalRating)
                  {
                  // Values defined by the DVB standard (minimum age = rating + 3 years):
                  case 0x01 ... 0x0F: parentalRating += 3; break;
                  // Values defined by broadcaster CSAT (now why didn't they just use 0x07, 0x09 and 0x0D?):
                  case 0x11:          parentalRating = 10; break;
                  case 0x12:          parentalRating = 12; break;
                  case 0x13:          parentalRating = 16; break;
                  default:            parentalRating = 0;
                  }

                  thisEvent->SetParentalRating(parentalRating);
                }
              }
              break;
            }
            */
            case SI::DishRatingDescriptorTag:
            {
              if (d->getLength() == 4)
              {
                uint16_t rating = d->getData().TwoBytes(2);
                uint16_t newRating = (rating >> 10) & 0x07;

                if (newRating == 0)
                  newRating = 5;
                if (newRating == 6)
                  newRating = 0;

                thisEvent->SetParentalRating((newRating << 10) | (rating & 0x3FF));
                thisEvent->SetStarRating((rating >> 13) & 0x07);
              }
              break;
            }
            case SI::PDCDescriptorTag:
            {
              SI::PDCDescriptor* pd = (SI::PDCDescriptor*)d;

              CDateTime vps = now;
              vps.SetDateTime(vps.GetYear(), pd->getMonth(), pd->getDay(), pd->getHour(), pd->getMinute(), 0);

              if (now.GetMonth() == 12 && vps.GetMonth() == 1) // current month is dec, but event is in jan
                vps += CDateTimeSpan(1, 0, 0, 0);
              else if (now.GetMonth() == 1 && vps.GetMonth() == 12) // current month is jan, but event is in dec
                vps -= CDateTimeSpan(1, 0, 0, 0);

              thisEvent->SetVps(vps);

              break;
            }
            case SI::LinkageDescriptorTag:
            {
              SI::LinkageDescriptor* ld = (SI::LinkageDescriptor*)d;

              cChannelID linkID(ld->getOriginalNetworkId(), ld->getTransportStreamId(), ld->getServiceId());

              // Premiere World
              if (ld->getLinkageType() == 0xB0)
              {
                CDateTime eitStart = CDateTime(eitEvent.getStartTime()).GetAsUTCDateTime();

                bool bHit = (eitStart <= now) && (now < (eitStart + CDateTimeSpan(0, 0, 0, eitEvent.getDuration())));
                if (bHit)
                {
                  string linkName = (const char*)ld->privateData.getData();

                  // TODO is there a standard way to determine the character set of this string?

                  //ChannelPtr link = m_channelManager.GetByChannelID(linkID); // TODO
                  ChannelPtr link = channel; // TODO

                  // Only link to other channels, not the same one
                  if (link != channel)
                  {
                    // TODO
                  }
                  else
                  {
                    channel->SetPortalName(linkName);
                  }
                }
              }
              break;
            }
            case SI::ComponentDescriptorTag:
            {
              SI::ComponentDescriptor* cd = (SI::ComponentDescriptor*)d;

              const uint8_t stream = cd->getStreamContent();
              const uint8_t type = cd->getComponentType();

              // 1=MPEG2-video, 2=MPEG1-audio, 3=subtitles, 4=AC3-audio, 5=H.264-video, 6=HEAAC-audio
              if (1 <= stream && stream <= 6 && type != 0)
              {
                char buffer[Utf8BufSize(256)] = { };
                cd->description.getText(buffer, sizeof(buffer));
                components.push_back(CEpgComponent(stream, type, I18nNormalizeLanguageCode(cd->languageCode), buffer));
              }
              break;
            }
            default:
              break;
            }

            DELETENULL(d);
          }

          thisEvent->SetTitle(strTitle);
          thisEvent->SetPlotOutline(strShortText);
          thisEvent->SetPlot(strDescription);

          thisEvent->SetContents(contents);

          thisEvent->SetComponents(components);

          thisEvent->FixEpgBugs();

          /* TODO
          if (!linkChannels.empty())
            channel->SetLinkChannels(linkChannels);
          */

          // Report pEvent
          events.push_back(thisEvent);

          // m_extendedEventDescriptors is allocated in GetText()
          SAFE_DELETE(m_extendedEventDescriptors);

          //bHasEvents |= true;
        }

        /*
        if (tid == 0x4E)
        {
          if (!bHasEvents && tsEIT.getSectionNumber() == 0)
          {
            // ETR 211: an empty entry in section 0 of table 0x4E means there is
            // currently no event running
            schedule->ClrRunningStatus(channel.get());
          }
          schedule->SetPresentSeen();
        }
        */

        /*
        if (bModified)
        {
          cEpgHandlers::Get().SortSchedule(m_Schedule);
          cEpgHandlers::Get().DropOutdated(m_Schedule, m_SegmentStart, m_SegmentEnd, Tid, m_Version);
          Schedules->SetModified(m_Schedule);
        }
        */
      }
    }
  }

  return events;
}

void cEit::GetText(SI::Descriptor* d, uint8_t tid, uint16_t nid, uint16_t tsid,
    std::string& strTitle, std::string& strShortText, std::string& strDescription)
{
  switch (d->getDescriptorTag())
  {
  case SI::ShortEventDescriptorTag:
  {
    SI::ShortEventDescriptor* sed = (SI::ShortEventDescriptor*)d;

    int iLanguagePreferenceShort = -1;
    bool bPreferred = I18nIsPreferredLanguage(cSettings::Get().m_EPGLanguages, sed->languageCode, iLanguagePreferenceShort);

    if (strTitle.empty() || bPreferred)
    {
      char buffer[Utf8BufSize(256)] = { };
      sed->name.getText(buffer, sizeof(buffer));
      strTitle = buffer;
    }

    if (strShortText.empty() || bPreferred)
    {
      char bufferShortText[Utf8BufSize(256)] = { };
      sed->text.getText(bufferShortText, sizeof(bufferShortText));
      strShortText = bufferShortText;
    }

    break;
  }
  case SI::ExtendedEventDescriptorTag:
  {
    SI::ExtendedEventDescriptor* eed = (SI::ExtendedEventDescriptor*)d;

    int iLanguagePreferenceExt = -1;
    bool bPreferred = I18nIsPreferredLanguage(cSettings::Get().m_EPGLanguages, eed->languageCode, iLanguagePreferenceExt);

    if (!m_extendedEventDescriptors || bPreferred)
    {
      delete m_extendedEventDescriptors;
      m_extendedEventDescriptors = new SI::ExtendedEventDescriptors;
    }

    m_extendedEventDescriptors->Add(eed);

    if (eed->getDescriptorNumber() == eed->getLastDescriptorNumber())
    {
      const size_t length = Utf8BufSize(m_extendedEventDescriptors->getMaximumTextLength(DESCRIPTION_SEPARATOR)) + 1;
      char* buffer = new char[length];

      m_extendedEventDescriptors->getText(buffer, length, DESCRIPTION_SEPARATOR);
      strDescription = buffer;
      delete[] buffer;

      // m_extendedEventDescriptors could be deleted here, but then we would
      // leak if the last descriptor number isn't present
    }

    break;
  }
  case SI::TimeShiftedEventDescriptorTag:
  {
    SI::TimeShiftedEventDescriptor* tsed = (SI::TimeShiftedEventDescriptor*)d;

    cChannelID channelId(nid,
                         tsid,
                         tsed->getReferenceServiceId());

    // TODO: Shouldn't need to query cScheduleManager, it should know how to
    // merge partial event data
    EventPtr rEvent = cScheduleManager::Get().GetEvent(channelId, tsed->getReferenceEventId());
    if (rEvent)
    {
      strTitle = rEvent->Title();
      strShortText = rEvent->PlotOutline();
      strDescription = rEvent->Plot();
    }

    break;
  }
  case SI::DishShortEventDescriptorTag:
  {
    SI::DishDescriptor* dsed = (SI::DishDescriptor*)d;
    dsed->Decompress(tid);

    strTitle = dsed->getText();
    break;
  }
  case SI::DishExtendedEventDescriptorTag:
  {
    SI::DishDescriptor* deed = (SI::DishDescriptor*)d;
    deed->Decompress(tid);

    strDescription = deed->getText();
    strShortText = deed->getShortText();
    break;
  }
  default:
    break;
  }
}

void cEit::GetContents(SI::ContentDescriptor* cd, vector<uint8_t>& contents)
{
  if (!cd)
    return;

  SI::ContentDescriptor::Nibble nibble;
  for (SI::Loop::Iterator it3; cd->nibbleLoop.getNext(nibble, it3); )
  {
    contents.push_back(((nibble.getContentNibbleLevel1() & 0xF) << 4) |
                        (nibble.getContentNibbleLevel2() & 0xF));
  }
}

}
