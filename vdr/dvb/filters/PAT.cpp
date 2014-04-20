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

#include "PAT.h"
#include "Config.h"
#include "dvb/CADescriptorHandler.h"
#include "channels/ChannelManager.h"
#include "libsi/section.h"
#include "libsi/descriptor.h"
#include "platform/threads/threads.h"
#include "settings/Settings.h"
#include "utils/I18N.h"

#include <malloc.h>

#define PMT_SCAN_TIMEOUT  10 // seconds

using namespace std;

namespace VDR
{

// --- cPatFilter ------------------------------------------------------------

cPatFilter::cPatFilter(cDevice* device)
 : cFilter(device),
   m_pmtIndex(0),
   m_pmtPid(0),
   m_pmtSid(0)
{
  Set(0x00, 0x00);  // PAT
}

void cPatFilter::Enable(bool bEnabled)
{
  cFilter::Enable(bEnabled);
  m_pmtIndex = 0;
  m_pmtPid = 0;
  m_pmtSid = 0;
  m_lastPmtScan.Reset();
  m_pmtVersions.clear();
}

void cPatFilter::Trigger(void)
{
  m_pmtVersions.clear();
}

bool cPatFilter::PmtVersionChanged(int pmtPid, int sid, int version)
{
  uint64_t v = version;
  v <<= 32;
  uint64_t id = (pmtPid | (sid << 16)) & 0x00000000FFFFFFFFLL;
  for (vector<uint64_t>::iterator it = m_pmtVersions.begin(); it != m_pmtVersions.end(); ++it)
  {
    if ((*it & 0x00000000FFFFFFFFLL) == id)
    {
      bool changed = (*it & 0x000000FF00000000LL) != v;
      if (changed)
        *it = id | v;
      return changed;
    }
  }

  m_pmtVersions.push_back(id | v);
  return true;
}

void cPatFilter::ProcessData(u_short pid, u_char tid, const std::vector<uint8_t>& data)
{
  if (pid == 0x00)
  {
    if (tid == 0x00)
    {
      CDateTime now = CDateTime::GetUTCDateTime();
      if (m_pmtPid && (now - m_lastPmtScan).GetSecondsTotal() > PMT_SCAN_TIMEOUT)
      {
        Del(m_pmtPid, 0x02);
        m_pmtPid = 0;
        m_pmtIndex++;
        m_lastPmtScan = now;
      }

      if (!m_pmtPid)
      {
        SI::PAT pat(data.data(), false);

        if (!pat.CheckCRCAndParse())
          return;

        SI::PAT::Association assoc;
        int Index = 0;
        for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); )
        {
          if (!assoc.isNITPid())
          {
            if (Index++ >= m_pmtIndex && cChannelManager::Get().GetByServiceID(Source(), Transponder(), assoc.getServiceId()))
            {
              m_pmtPid = assoc.getPid();
              m_pmtSid = assoc.getServiceId();
              Set(m_pmtPid, 0x02, 0xFF, false);
              break;
            }
          }
        }

        if (!m_pmtPid)
          m_pmtIndex = 0;
      }
    }
  }
  else if (pid == m_pmtPid && tid == SI::TableIdPMT && Source() && Transponder())
  {
    SI::PMT pmt(data.data(), false);

    if (!pmt.CheckCRCAndParse())
      return;

    if (pmt.getServiceId() != m_pmtSid)
      return; // skip broken PMT records

    if (!PmtVersionChanged(m_pmtPid, pmt.getTableIdExtension(), pmt.getVersionNumber()))
    {
      m_lastPmtScan.Reset(); // this triggers the next scan
      return;
    }

    //XXX
//    if (!Channels.Lock(true, 10))
//    {
//      numPmtEntries = 0; // to make sure we try again
//      return;
//    }

    ChannelPtr Channel = cChannelManager::Get().GetByServiceID(Source(), Transponder(), pmt.getServiceId());
    if (Channel)
    {
      /* TODO
      SI::CaDescriptor *d;
      CaDescriptorsPtr CaDescriptors = CaDescriptorsPtr(new cCaDescriptors(Channel->Source(), Channel->Transponder(), Channel->Get()));
      // Scan the common loop:
      for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)pmt.commonDescriptors.getNext(it, SI::CaDescriptorTag)); )
      {
        CaDescriptors->AddCaDescriptor(d, 0);
        delete d;
      }
      // Scan the stream-specific loop:
      SI::PMT::Stream stream;
      int Vpid = 0;
      int Ppid = 0;
      int Vtype = 0;
      int Apids[MAXAPIDS + 1] = { 0 }; // these lists are zero-terminated
      int Atypes[MAXAPIDS + 1] = { 0 };
      int Dpids[MAXDPIDS + 1] = { 0 };
      int Dtypes[MAXDPIDS + 1] = { 0 };
      int Spids[MAXSPIDS + 1] = { 0 };
      uchar SubtitlingTypes[MAXSPIDS + 1] = { 0 };
      uint16_t CompositionPageIds[MAXSPIDS + 1] = { 0 };
      uint16_t AncillaryPageIds[MAXSPIDS + 1] = { 0 };
      char ALangs[MAXAPIDS][MAXLANGCODE2] = { "" };
      char DLangs[MAXDPIDS][MAXLANGCODE2] = { "" };
      char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
      int Tpid = 0;
      int NumApids = 0;
      int NumDpids = 0;
      int NumSpids = 0;
      for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it); ) {
        bool ProcessCaDescriptors = false;
        int esPid = stream.getPid();
        switch (stream.getStreamType())
        {
        case 1: // STREAMTYPE_11172_VIDEO
        case 2: // STREAMTYPE_13818_VIDEO
        case 0x1B: // H.264
          Vpid = esPid;
          Ppid = pmt.getPCRPid();
          Vtype = stream.getStreamType();
          ProcessCaDescriptors = true;
          break;
        case 3: // STREAMTYPE_11172_AUDIO
        case 4: // STREAMTYPE_13818_AUDIO
        case 0x0F: // ISO/IEC 13818-7 Audio with ADTS transport syntax
        case 0x11: // ISO/IEC 14496-3 Audio with LATM transport syntax
        {
          if (NumApids < MAXAPIDS)
          {
            Apids[NumApids] = esPid;
            Atypes[NumApids] = stream.getStreamType();
            SI::Descriptor *d;
            for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
            {
              switch (d->getDescriptorTag())
              {
              case SI::ISO639LanguageDescriptorTag:
              {
                SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                SI::ISO639LanguageDescriptor::Language l;
                char *s = ALangs[NumApids];
                int n = 0;
                for (SI::Loop::Iterator it; ld->languageLoop.getNext(l, it); )
                {
                  if (*ld->languageCode != '-') // some use "---" to indicate "none"
                  {
                    if (n > 0)
                      *s++ = '+';
                    strn0cpy(s, I18nNormalizeLanguageCode(l.languageCode), MAXLANGCODE1);
                    s += strlen(s);
                    if (n++ > 1)
                      break;
                  }
                }
                break;
              }
              default:
                break;
              }
              delete d;
            }
            NumApids++;
          }
          ProcessCaDescriptors = true;
        }
        break;
        case 5: // STREAMTYPE_13818_PRIVATE
        case 6: // STREAMTYPE_13818_PES_PRIVATE
        //XXX case 8: // STREAMTYPE_13818_DSMCC
        {
          int dpid = 0;
          int dtype = 0;
          char lang[MAXLANGCODE1] = { 0 };
          SI::Descriptor *d;
          for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
          {
            switch (d->getDescriptorTag())
            {
            case SI::AC3DescriptorTag:
            case SI::EnhancedAC3DescriptorTag:
              dpid = esPid;
              dtype = d->getDescriptorTag();
              ProcessCaDescriptors = true;
              break;
            case SI::SubtitlingDescriptorTag:
              if (NumSpids < MAXSPIDS)
              {
                Spids[NumSpids] = esPid;
                SI::SubtitlingDescriptor *sd = (SI::SubtitlingDescriptor *)d;
                SI::SubtitlingDescriptor::Subtitling sub;
                char *s = SLangs[NumSpids];
                int n = 0;
                for (SI::Loop::Iterator it; sd->subtitlingLoop.getNext(sub, it); )
                {
                  if (sub.languageCode[0])
                  {
                    SubtitlingTypes[NumSpids] = sub.getSubtitlingType();
                    CompositionPageIds[NumSpids] = sub.getCompositionPageId();
                    AncillaryPageIds[NumSpids] = sub.getAncillaryPageId();
                    if (n > 0)
                      *s++ = '+';
                    strn0cpy(s, I18nNormalizeLanguageCode(sub.languageCode), MAXLANGCODE1);
                    s += strlen(s);
                    if (n++ > 1)
                      break;
                  }
                }
                NumSpids++;
              }
              break;
            case SI::TeletextDescriptorTag:
              Tpid = esPid;
              break;
            case SI::ISO639LanguageDescriptorTag:
            {
              SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
              strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
              break;
            }
            default:
              break;
            }
            delete d;
          }
          if (dpid)
          {
            if (NumDpids < MAXDPIDS)
            {
              Dpids[NumDpids] = dpid;
              Dtypes[NumDpids] = dtype;
              strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
              NumDpids++;
            }
          }
        }
        break;
        case 0x80: // STREAMTYPE_USER_PRIVATE
          if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE) // DigiCipher II VIDEO (ANSI/SCTE 57)
          {
            Vpid = esPid;
            Ppid = pmt.getPCRPid();
            Vtype = 0x02; // compression based upon MPEG-2
            ProcessCaDescriptors = true;
            break;
          }
          // fall through
        case 0x81: // STREAMTYPE_USER_PRIVATE
          if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE) // ATSC A/53 AUDIO (ANSI/SCTE 57)
          {
            char lang[MAXLANGCODE1] = { 0 };
            SI::Descriptor *d;
            for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
            {
              switch (d->getDescriptorTag())
              {
              case SI::ISO639LanguageDescriptorTag:
              {
                SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                break;
              }
              default:
                break;
              }
              delete d;
            }
            if (NumDpids < MAXDPIDS)
            {
              Dpids[NumDpids] = esPid;
              Dtypes[NumDpids] = SI::AC3DescriptorTag;
              strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
              NumDpids++;
            }
            ProcessCaDescriptors = true;
            break;
          }
          // fall through
        case 0x82: // STREAMTYPE_USER_PRIVATE
          if (cSettings::Get().m_iStandardCompliance == STANDARD_ANSISCTE) // STANDARD SUBTITLE (ANSI/SCTE 27)
          {
            //TODO
            break;
          }
          // fall through
        case 0x83 ... 0xFF: // STREAMTYPE_USER_PRIVATE
        {
          char lang[MAXLANGCODE1] = { 0 };
          bool IsAc3 = false;
          SI::Descriptor *d;
          for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
          {
            switch (d->getDescriptorTag())
            {
            case SI::RegistrationDescriptorTag:
            {
              SI::RegistrationDescriptor *rd = (SI::RegistrationDescriptor *)d;
              // http://www.smpte-ra.org/mpegreg/mpegreg.html
              switch (rd->getFormatIdentifier())
              {
              case 0x41432D33: // 'AC-3'
                IsAc3 = true;
                break;
              default:
                //printf("Format identifier: 0x%08X (pid: %d)\n", rd->getFormatIdentifier(), esPid);
                break;
              }
              break;
            }
            case SI::ISO639LanguageDescriptorTag:
            {
              SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
              strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
              break;
            }
            default:
              break;
            }
            delete d;
          }
          if (IsAc3)
          {
            if (NumDpids < MAXDPIDS)
            {
              Dpids[NumDpids] = esPid;
              Dtypes[NumDpids] = SI::AC3DescriptorTag;
              strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
              NumDpids++;
            }
            ProcessCaDescriptors = true;
          }
          break;
        }
        default:
          //printf("PID: %5d %5d %2d %3d %3d\n", pmt.getServiceId(), stream.getPid(), stream.getStreamType(), pmt.getVersionNumber(), Channel->Number());
          break;
        }
        if (ProcessCaDescriptors)
        {
          for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)stream.streamDescriptors.getNext(it, SI::CaDescriptorTag)); )
          {
            CaDescriptors->AddCaDescriptor(d, esPid);
            delete d;
          }
        }
      }
      if (cSettings::Get().m_iUpdateChannels >= 2)
      {
        Channel->SetPids(Vpid, Ppid, Vtype, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
        Channel->SetCaIds(CaDescriptors->CaIds().data());
        Channel->SetSubtitlingDescriptors(SubtitlingTypes, CompositionPageIds, AncillaryPageIds);
      }
      Channel->SetCaDescriptors(cCaDescriptorHandler::Get().AddCaDescriptors(CaDescriptors));
      Channel->NotifyObservers(ObservableMessageChannelChanged);
      */
    }
    m_lastPmtScan.Reset(); // this triggers the next scan
//    XXX Channels.Unlock();
  }
}

}
