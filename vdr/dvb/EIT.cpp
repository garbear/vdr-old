/*
 * eit.c: EIT section filter
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version (as used in VDR before 1.3.0) written by
 * Robert Schneider <Robert.Schneider@web.de> and Rolf Hakenes <hakenes@hippomi.de>.
 * Adapted to 'libsi' for VDR 1.3.0 by Marcel Wiesweg <marcel.wiesweg@gmx.de>.
 *
 * $Id: eit.c 2.23.1.1 2013/10/12 11:24:51 kls Exp $
 */

#include "EIT.h"
#include "Config.h"
#include <sys/time.h>
#include "epg/EPG.h"
#include "epg/EPGHandlers.h"
#include "channels/ChannelManager.h"
#include "utils/I18N.h"
#include "libsi/section.h"
#include "libsi/descriptor.h"
#include "libsi/dish.h"

#include "vdr/utils/CalendarUtils.h"
#include "vdr/utils/UTF8Utils.h"

#define VALID_TIME (31536000 * 2) // two years

// --- cEIT ------------------------------------------------------------------

class cEIT : public SI::EIT {
public:
  cEIT(cSchedules *Schedules, int Source, u_char Tid, const u_char *Data, bool OnlyRunningStatus = false);
private:
  bool HandleEitEvent(SI::EIT::Event *EitEvent);
  void ParseSIDescriptor(SI::Descriptor* d);

  bool          m_bHandledExternally;
  bool          m_bOnlyRunningStatus;
  bool          m_bModified;
  time_t        m_SegmentStart;
  time_t        m_SegmentEnd;
  cSchedules*   m_Schedules;
  int           m_iSource;
  u_char        m_Tid;
  const u_char* m_Data;
  SchedulePtr   m_Schedule;
  uchar         m_Version;
  ChannelPtr    m_channel;
  struct tm     m_localTime;
  cEvent*       m_newEvent;
  cEvent*       m_rEvent;
  cEvent*       m_pEvent;
  time_t        m_Now;
  time_t        m_StartTime;
  int           m_iDuration;

  int                           m_iLanguagePreferenceShort;
  int                           m_iLanguagePreferenceExt;
  bool                          m_bUseExtendedEventDescriptor;
  SI::DishDescriptor*           m_DishExtendedEventDescriptor;
  SI::DishDescriptor*           m_DishShortEventDescriptor;
  SI::ExtendedEventDescriptors* m_ExtendedEventDescriptors;
  SI::ShortEventDescriptor*     m_ShortEventDescriptor;
  cLinkChannels                 m_LinkChannels;
  CEpgComponents*                  m_Components;
};

void cEIT::ParseSIDescriptor(SI::Descriptor* d)
{
  switch (d->getDescriptorTag())
  {
  case SI::ExtendedEventDescriptorTag:
    {
      SI::ExtendedEventDescriptor *eed = (SI::ExtendedEventDescriptor *) d;

      if (I18nIsPreferredLanguage(g_setup.EPGLanguages, eed->languageCode, m_iLanguagePreferenceExt) || !m_ExtendedEventDescriptors)
      {
        delete m_ExtendedEventDescriptors;
        m_ExtendedEventDescriptors = new SI::ExtendedEventDescriptors;
        m_bUseExtendedEventDescriptor = true;
      }

      if (m_bUseExtendedEventDescriptor)
      {
        m_ExtendedEventDescriptors->Add(eed);
        d = NULL; // so that it is not deleted
      }

      if (eed->getDescriptorNumber() == eed->getLastDescriptorNumber())
        m_bUseExtendedEventDescriptor = false;
    }
    break;
  case SI::DishExtendedEventDescriptorTag:
    {
      SI::DishDescriptor *deed = (SI::DishDescriptor *) d;
      deed->Decompress(m_Tid);
      if (!m_DishExtendedEventDescriptor)
      {
        m_DishExtendedEventDescriptor = deed;
        d = NULL; // so that it is not deleted
      }
      m_bHandledExternally = true;
    }
    break;
  case SI::DishShortEventDescriptorTag:
    {
      SI::DishDescriptor *dsed = (SI::DishDescriptor *) d;
      dsed->Decompress(m_Tid);
      if (!m_DishShortEventDescriptor)
      {
        m_DishShortEventDescriptor = dsed;
        d = NULL; // so that it is not deleted
      }
      m_bHandledExternally = true;
    }
    break;
  case SI::ShortEventDescriptorTag:
    {
      SI::ShortEventDescriptor *sed = (SI::ShortEventDescriptor *) d;
      if (I18nIsPreferredLanguage(g_setup.EPGLanguages, sed->languageCode, m_iLanguagePreferenceShort) || !m_ShortEventDescriptor)
      {
        delete m_ShortEventDescriptor;
        m_ShortEventDescriptor = sed;
        d = NULL; // so that it is not deleted
      }
    }
    break;
  case SI::ContentDescriptorTag:
    {
      SI::ContentDescriptor *cd = (SI::ContentDescriptor *) d;
      SI::ContentDescriptor::Nibble Nibble;
      int NumContents = 0;
      uchar Contents[MaxEventContents] =
        { 0 };
      for (SI::Loop::Iterator it3; cd->nibbleLoop.getNext(Nibble, it3);)
      {
        if (NumContents < MaxEventContents)
        {
          Contents[NumContents] = ((Nibble.getContentNibbleLevel1() & 0xF)
              << 4) | (Nibble.getContentNibbleLevel2() & 0xF);
          NumContents++;
        }
      }
      cEpgHandlers::Get().SetContents(m_pEvent, Contents);
    }
    break;
    /*
     case SI::ParentalRatingDescriptorTag: {
     int LanguagePreferenceRating = -1;
     SI::ParentalRatingDescriptor *prd = (SI::ParentalRatingDescriptor *)d;
     SI::ParentalRatingDescriptor::Rating Rating;
     for (SI::Loop::Iterator it3; prd->ratingLoop.getNext(Rating, it3); ) {
     if (I18nIsPreferredLanguage(Setup.EPGLanguages, Rating.languageCode, LanguagePreferenceRating)) {
     int ParentalRating = (Rating.getRating() & 0xFF);
     switch (ParentalRating) {
     // values defined by the DVB standard (minimum age = rating + 3 years):
     case 0x01 ... 0x0F: ParentalRating += 3; break;
     // values defined by broadcaster CSAT (now why didn't they just use 0x07, 0x09 and 0x0D?):
     case 0x11:          ParentalRating = 10; break;
     case 0x12:          ParentalRating = 12; break;
     case 0x13:          ParentalRating = 16; break;
     default:            ParentalRating = 0;
     }
     cEpgHandlers::Get().SetParentalRating(pEvent, ParentalRating);
     }
     }
     }
     break;
     */
  case SI::DishRatingDescriptorTag:
    if (d->getLength() == 4)
    {
      uint16_t rating = d->getData().TwoBytes(2);
      uint16_t newRating = (rating >> 10) & 0x07;
      if (newRating == 0)
        newRating = 5;
      if (newRating == 6)
        newRating = 0;
      m_pEvent->SetParentalRating((newRating << 10) | (rating & 0x3FF));
      m_pEvent->SetStarRating((rating >> 13) & 0x07);
    }
    break;
  case SI::PDCDescriptorTag:
    {
      SI::PDCDescriptor *pd = (SI::PDCDescriptor *) d;
      m_localTime.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
      int month = m_localTime.tm_mon;
      m_localTime.tm_mon = pd->getMonth() - 1;
      m_localTime.tm_mday = pd->getDay();
      m_localTime.tm_hour = pd->getHour();
      m_localTime.tm_min = pd->getMinute();
      m_localTime.tm_sec = 0;
      if (month == 11 && m_localTime.tm_mon == 0) // current month is dec, but event is in jan
        m_localTime.tm_year++;
      else if (month == 0 && m_localTime.tm_mon == 11) // current month is jan, but event is in dec
        m_localTime.tm_year--;
      time_t vps = mktime(&m_localTime);
      cEpgHandlers::Get().SetVps(m_pEvent, vps);
    }
    break;
  case SI::TimeShiftedEventDescriptorTag:
    {
      SI::TimeShiftedEventDescriptor *tsed = (SI::TimeShiftedEventDescriptor *) d;
      SchedulePtr rSchedule = m_Schedules->GetSchedule(tChannelID(m_iSource, m_channel->Nid(), m_channel->Tid(), tsed->getReferenceServiceId()));
      if (!rSchedule)
        break;
      m_rEvent = rSchedule->GetEvent(tsed->getReferenceEventId());
      if (!m_rEvent)
        break;
      cEpgHandlers::Get().SetTitle(m_pEvent, m_rEvent->Title());
      cEpgHandlers::Get().SetShortText(m_pEvent, m_rEvent->ShortText());
      cEpgHandlers::Get().SetDescription(m_pEvent, m_rEvent->Description());
    }
    break;
  case SI::LinkageDescriptorTag:
    {
      SI::LinkageDescriptor *ld = (SI::LinkageDescriptor *) d;
      tChannelID linkID(m_iSource, ld->getOriginalNetworkId(),
          ld->getTransportStreamId(), ld->getServiceId());
      if (ld->getLinkageType() == 0xB0)
      { // Premiere World
        bool hit = m_StartTime <= m_Now && m_Now < m_StartTime + m_iDuration;
        if (hit)
        {
          char linkName[ld->privateData.getLength() + 1];
          strn0cpy(linkName, (const char *) ld->privateData.getData(), sizeof(linkName));
          // TODO is there a standard way to determine the character set of this string?
          ChannelPtr link = cChannelManager::Get().GetByChannelID(linkID);
          if (link != m_channel)
          { // only link to other channels, not the same one
            //fprintf(stderr, "Linkage %s %4d %4d %5d %5d %5d %5d  %02X  '%s'\n", hit ? "*" : "", channel->Number(), link ? link->Number() : -1, SiEitEvent.getEventId(), ld->getOriginalNetworkId(), ld->getTransportStreamId(), ld->getServiceId(), ld->getLinkageType(), linkName);//XXX
            if (link)
            {
              if (g_setup.UpdateChannels == 1 || g_setup.UpdateChannels >= 3)
              {
                link->SetName(linkName, "", "");
                link->NotifyObservers(ObservableMessageChannelChanged);
              }
            }
            else if (g_setup.UpdateChannels >= 4)
            {
              ChannelPtr transponder = m_channel;
              if (m_channel->Tid() != ld->getTransportStreamId())
                transponder = cChannelManager::Get().GetByTransponderID(linkID);
              link = cChannelManager::Get().NewChannel(*transponder, linkName, "", "", ld->getOriginalNetworkId(), ld->getTransportStreamId(), ld->getServiceId());
              //patFilter->Trigger();
            }
            if (link)
            {
              m_LinkChannels.push_back(new cLinkChannel(link));
            }
          }
          else
          {
            m_channel->SetPortalName(linkName);
            m_channel->NotifyObservers(ObservableMessageChannelChanged);
          }
        }
      }
    }
    break;
  case SI::ComponentDescriptorTag:
    {
      SI::ComponentDescriptor *cd = (SI::ComponentDescriptor *) d;
      uchar Stream = cd->getStreamContent();
      uchar Type = cd->getComponentType();
      if (1 <= Stream && Stream <= 6 && Type != 0)
      { // 1=MPEG2-video, 2=MPEG1-audio, 3=subtitles, 4=AC3-audio, 5=H.264-video, 6=HEAAC-audio
        if (!m_Components)
          m_Components = new CEpgComponents;
        char buffer[Utf8BufSize(256)];
        m_Components->SetComponent(COMPONENT_ADD_NEW, Stream, Type, I18nNormalizeLanguageCode(cd->languageCode), cd->description.getText(buffer, sizeof(buffer)));
      }
    }
    break;
  default:
    break;
    }
}

bool cEIT::HandleEitEvent(SI::EIT::Event *EitEvent)
{
  if (cEpgHandlers::Get().HandleEitEvent(m_Schedule, EitEvent, m_Tid, m_Version))
    return true; // an EPG handler has done all of the processing

  m_StartTime = EitEvent->getStartTime();
  m_iDuration = EitEvent->getDuration();

  // Drop bogus events - but keep NVOD reference events, where all bits of the start time field are set to 1, resulting in a negative number.
  if (m_StartTime == 0 || (m_StartTime > 0 && m_iDuration == 0))
    return false;

  if (!m_SegmentStart)
    m_SegmentStart = m_StartTime;
  m_SegmentEnd = m_StartTime + m_iDuration;

  m_newEvent = NULL;
  m_rEvent   = NULL;
  m_pEvent   = m_Schedule->GetEvent(EitEvent->getEventId(), m_StartTime);

  if (!m_pEvent || m_bHandledExternally)
  {
    if (m_bOnlyRunningStatus)
      return true;
    if (m_bHandledExternally && !cEpgHandlers::Get().IsUpdate(EitEvent->getEventId(), m_StartTime, m_Tid, getVersionNumber()))
      return true;

    // If we don't have that event yet, we create a new one.
    // Otherwise we copy the information into the existing event anyway, because the data might have changed.
    m_pEvent = m_newEvent = new cEvent(EitEvent->getEventId());
    m_newEvent->SetStartTime(m_StartTime);
    m_newEvent->SetDuration(m_iDuration);
    if (!m_bHandledExternally)
      m_Schedule->AddEvent(m_newEvent);
  }
  else
  {
    // We have found an existing event, either through its event ID or its start time.
    m_pEvent->SetSeen();
    uchar TableID = max(m_pEvent->TableID(), uchar(0x4E)); // for backwards compatibility, table ids less than 0x4E are treated as if they were "present"
    // If the new event has a higher table ID, let's skip it.
    // The lower the table ID, the more "current" the information.
    if (m_Tid > TableID)
      return true;
    // If the new event comes from the same table and has the same version number
    // as the existing one, let's skip it to avoid unnecessary work.
    // Unfortunately some stations (like, e.g. "Premiere") broadcast their EPG data on several transponders (like
    // the actual Premiere transponder and the Sat.1/Pro7 transponder), but use different version numbers on
    // each of them :-( So if one DVB card is tuned to the Premiere transponder, while an other one is tuned
    // to the Sat.1/Pro7 transponder, events will keep toggling because of the bogus version numbers.
    else if (m_Tid == TableID && m_pEvent->Version() == getVersionNumber())
      return true;

    cEpgHandlers::Get().SetEventID(m_pEvent, EitEvent->getEventId()); // unfortunately some stations use different event ids for the same event in different tables :-(
    cEpgHandlers::Get().SetStartTime(m_pEvent, m_StartTime);
    cEpgHandlers::Get().SetDuration(m_pEvent, m_iDuration);
  }

  if (m_pEvent->TableID() > 0x4E) // for backwards compatibility, table ids less than 0x4E are never overwritten
    m_pEvent->SetTableID(m_Tid);

  if (m_Tid == 0x4E)
  { // we trust only the present/following info on the actual TS
    if (EitEvent->getRunningStatus() >= SI::RunningStatusNotRunning)
      m_Schedule->SetRunningStatus(m_pEvent, EitEvent->getRunningStatus(), m_channel.get());
  }

  if (m_bOnlyRunningStatus)
  {
    m_pEvent->SetVersion(0xFF); // we have already changed the table id above, so set the version to an invalid value to make sure the next full run will be executed
    return true; // do this before setting the version, so that the full update can be done later
  }
  m_pEvent->SetVersion(getVersionNumber());

  SI::Descriptor* d;
  m_iLanguagePreferenceShort = -1;
  m_iLanguagePreferenceExt = -1;
  m_bUseExtendedEventDescriptor = false;
  m_DishExtendedEventDescriptor = NULL;
  m_DishShortEventDescriptor = NULL;
  m_ExtendedEventDescriptors = NULL;
  m_ShortEventDescriptor = NULL;
  m_Components = NULL;

  for (SI::Loop::Iterator it; (d = EitEvent->eventDescriptors.getNext(it));)
  {
    ParseSIDescriptor(d);

    if (!m_rEvent)
    {
      if (m_DishShortEventDescriptor)
      {
        m_pEvent->SetTitle(m_DishShortEventDescriptor->getText());
      }
      if (m_DishExtendedEventDescriptor)
      {
        m_pEvent->SetDescription(m_DishExtendedEventDescriptor->getText());
        m_pEvent->SetShortText(m_DishExtendedEventDescriptor->getShortText());
      }
      if (m_ShortEventDescriptor)
      {
        char buffer[Utf8BufSize(256)];
        cEpgHandlers::Get().SetTitle(m_pEvent, m_ShortEventDescriptor->name.getText(buffer, sizeof(buffer)));
        cEpgHandlers::Get().SetShortText(m_pEvent, m_ShortEventDescriptor->text.getText(buffer, sizeof(buffer)));
      }
      else
      {
        cEpgHandlers::Get().SetTitle(m_pEvent, NULL);
        cEpgHandlers::Get().SetShortText(m_pEvent, NULL);
      }
      if (m_ExtendedEventDescriptors)
      {
        char buffer[Utf8BufSize(m_ExtendedEventDescriptors->getMaximumTextLength(": ")) + 1];
        cEpgHandlers::Get().SetDescription(m_pEvent, m_ExtendedEventDescriptors->getText(buffer, sizeof(buffer), ": "));
      }
      else
        cEpgHandlers::Get().SetDescription(m_pEvent, NULL);
    }
    SAFE_DELETE(m_DishExtendedEventDescriptor);
    SAFE_DELETE(m_DishShortEventDescriptor);
    SAFE_DELETE(m_ExtendedEventDescriptors);
    SAFE_DELETE(m_ShortEventDescriptor);

    cEpgHandlers::Get().SetComponents(m_pEvent, m_Components);

    cEpgHandlers::Get().FixEpgBugs(m_pEvent);
    if (!m_LinkChannels.empty())
      m_channel->SetLinkChannels(m_LinkChannels);
    m_bModified = true;
    cEpgHandlers::Get().HandleEvent(m_pEvent);
    if (m_bHandledExternally)
      SAFE_DELETE(m_pEvent);
  }

  return true;
}

cEIT::cEIT(cSchedules *Schedules, int Source, u_char Tid, const u_char *Data, bool OnlyRunningStatus) :
    SI::EIT(Data, false)
{
  if (!CheckCRCAndParse())
    return;

  m_channel = cChannelManager::Get().GetByChannelID(getOriginalNetworkId(), getTransportStreamId(), getServiceId());
  if (!m_channel)
    return;

  if (cEpgHandlers::Get().IgnoreChannel(*m_channel))
    return;

  m_Now = time(NULL);
  if (m_Now < VALID_TIME)
    return; // we need the current time for handling PDC descriptors

  struct tm tm_r;
  m_localTime          = *localtime_r(&m_Now, &tm_r); // this initializes the time zone in 't'
  m_bHandledExternally = cEpgHandlers::Get().HandledExternally(m_channel.get());
  m_Schedules          = Schedules;
  m_Schedule           = Schedules->GetSchedule(m_channel, true);
  m_iSource            = Source;
  m_Tid                = Tid;
  m_Data               = Data;
  m_bOnlyRunningStatus = OnlyRunningStatus;
  m_bModified          = false;
  m_SegmentStart       = 0;
  m_SegmentEnd         = 0;
  m_Version            = getVersionNumber();

  SI::EIT::Event SiEitEvent;
  bool bHasEvents(false);
  for (SI::Loop::Iterator it; eventLoop.getNext(SiEitEvent, it);)
    bHasEvents |= HandleEitEvent(&SiEitEvent);

  if (Tid == 0x4E)
  {
    if (!bHasEvents && getSectionNumber() == 0)
      // ETR 211: an empty entry in section 0 of table 0x4E means there is currently no event running
      m_Schedule->ClrRunningStatus(m_channel.get());
    m_Schedule->SetPresentSeen();
  }

  if (m_bModified && !OnlyRunningStatus)
  {
    cEpgHandlers::Get().SortSchedule(m_Schedule);
    cEpgHandlers::Get().DropOutdated(m_Schedule, m_SegmentStart, m_SegmentEnd, Tid, m_Version);
    Schedules->SetModified(m_Schedule);
  }
}

// --- cTDT ------------------------------------------------------------------

#define MAX_TIME_DIFF   1 // number of seconds the local time may differ from dvb time before making any corrections
#define MAX_ADJ_DIFF   10 // number of seconds the local time may differ from dvb time to allow smooth adjustment
#define ADJ_DELTA     300 // number of seconds between calls for smooth time adjustment

class cTDT : public SI::TDT {
private:
  static PLATFORM::CMutex mutex;
  static time_t lastAdj;
public:
  cTDT(const u_char *Data);
  };

PLATFORM::CMutex cTDT::mutex;
time_t cTDT::lastAdj = 0;

cTDT::cTDT(const u_char *Data)
:SI::TDT(Data, false)
{
  CheckParse();

#ifndef ANDROID
  time_t dvbtim = getTime();
  time_t loctim = time(NULL);

  int diff = dvbtim - loctim;
  if (abs(diff) > MAX_TIME_DIFF) {
     mutex.Lock();
     if (abs(diff) > MAX_ADJ_DIFF) {
        if (stime(&dvbtim) == 0)
           isyslog("system time changed from %s (%ld) to %s (%ld)", CalendarUtils::TimeToString(loctim).c_str(), loctim, CalendarUtils::TimeToString(dvbtim).c_str(), dvbtim);
        else
           esyslog("ERROR while setting system time: %m");
        }
     else if (time(NULL) - lastAdj > ADJ_DELTA) {
        lastAdj = time(NULL);
        timeval delta;
        delta.tv_sec = diff;
        delta.tv_usec = 0;
        if (adjtime(&delta, NULL) == 0)
           isyslog("system time adjustment initiated from %s (%ld) to %s (%ld)", CalendarUtils::TimeToString(loctim).c_str(), loctim, CalendarUtils::TimeToString(dvbtim).c_str(), dvbtim);
        else
           esyslog("ERROR while adjusting system time: %m");
        }
     mutex.Unlock();
     }
#endif
}

// --- cEitFilter ------------------------------------------------------------

time_t cEitFilter::disableUntil = 0;

cEitFilter::cEitFilter(void)
{
  Set(0x12, 0x00, 0x00);
  Set(0x0300, 0x00, 0x00); // Dish Network EEPG
  Set(0x0441, 0x00, 0x00); // Bell ExpressVU EEPG 
  Set(0x14, 0x70);        // TDT
}

void cEitFilter::SetDisableUntil(time_t Time)
{
  disableUntil = Time;
}

void cEitFilter::ProcessData(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  if (disableUntil) {
     if (time(NULL) > disableUntil)
        disableUntil = 0;
     else
        return;
     }
  switch (Pid) {
    case 0x0300:
    case 0x0441:
    case 0x12: {
         if (Tid >= 0x4E) {
            cSchedulesLock SchedulesLock(true, 10);
            cSchedules *Schedules = SchedulesLock.Get();
            if (Schedules)
               cEIT EIT(Schedules, Source(), Tid, Data);
            else {
               // If we don't get a write lock, let's at least get a read lock, so
               // that we can set the running status and 'seen' timestamp (well, actually
               // with a read lock we shouldn't be doing that, but it's only integers that
               // get changed, so it should be ok)
               cSchedulesLock SchedulesLock;
               cSchedules *Schedules = SchedulesLock.Get();
               if (Schedules)
                  cEIT EIT(Schedules, Source(), Tid, Data, true);
               }
            }
         }
         break;
    case 0x14: {
         if (g_setup.SetSystemTime && g_setup.TimeSource == Source() && g_setup.TimeTransponder && ISTRANSPONDER(Transponder(), g_setup.TimeTransponder))
            cTDT TDT(Data);
         }
         break;
    default: ;
    }
}
