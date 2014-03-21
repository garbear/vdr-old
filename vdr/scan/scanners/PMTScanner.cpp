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

#include "PMTScanner.h"
#include "channels/Channel.h"
#include "dvb/CADescriptorHandler.h"
#include "dvb/CADescriptors.h"
#include "utils/I18N.h"

#include <libsi/si_ext.h>
#include <libsi/descriptor.h>
#include <libsi/section.h>

using namespace SI;
using namespace SI_EXT;
using namespace std;

namespace VDR
{

cPmtScanner::cPmtScanner(cDevice* device, ChannelPtr channel, u_short Sid, u_short PmtPid)
 : cFilter(device),
   pmtPid(PmtPid),
   pmtSid(Sid),
   Channel(channel),
   numPmtEntries(0)
{
  Set(pmtPid, TableIdPMT);     // PMT
}

void cPmtScanner::ProcessData(u_short pid, u_char tid, const vector<uint8_t>& data)
{
  SI::PMT pmt(data.data(), false);
  if (!pmt.CheckCRCAndParse() || (pmt.getServiceId() != pmtSid))
    return;

  //HEXDUMP(data.data(), Length);

  if (Channel->Sid() != pmtSid)
  {
    dsyslog("ERROR: Channel->Sid(%d) != pmtSid(%d)", Channel->Sid(), pmtSid);
    return;
  }

  SI::CaDescriptor * d;
  CaDescriptorsPtr CaDescriptors = CaDescriptorsPtr(new cCaDescriptors(Channel->Source(), Channel->Transponder(), Channel->Sid()));

  // Scan the common loop:
  for (SI::Loop::Iterator it; (d = (SI::CaDescriptor *) pmt.commonDescriptors.getNext(it, SI::CaDescriptorTag));)
  {
    CaDescriptors->AddCaDescriptor(d, false);
    DELETENULL(d);
  }

  // Scan the stream-specific loop:
  SI::PMT::Stream stream;
  int             Vpid                           = 0;
  int             Vtype                          = 0;
  int             Ppid                           = 0;
  int             StreamType                     = STREAMTYPE_UNDEFINED;
  int             Apids[MAXAPIDS + 1]            = {0}; // these lists are zero-terminated
  int             Atypes[MAXAPIDS + 1]           = {0};
  int             Dpids[MAXDPIDS + 1]            = {0};
  int             Dtypes[MAXDPIDS + 1]           = {0};
  int             Spids[MAXSPIDS + 1]            = {0};
  char            ALangs[MAXAPIDS][MAXLANGCODE2] = {""};
  char            DLangs[MAXDPIDS][MAXLANGCODE2] = {""};
  char            SLangs[MAXSPIDS][MAXLANGCODE2] = {""};
  int             Tpid                           = 0;
  int             NumApids                       = 0;
  int             NumDpids                       = 0;
  int             NumSpids                       = 0;

  for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it);)
  {
    StreamType = stream.getStreamType();
    switch (StreamType)
    {
      case STREAMTYPE_11172_VIDEO:
      case STREAMTYPE_13818_VIDEO:
      case STREAMTYPE_14496_H264_VIDEO:
        Vpid  = stream.getPid();
        Vtype = StreamType;
        Ppid  = pmt.getPCRPid();
        break;
      case STREAMTYPE_11172_AUDIO:
      case STREAMTYPE_13818_AUDIO:
      case STREAMTYPE_13818_AUDIO_ADTS:
      case STREAMTYPE_14496_AUDIO_LATM:
      {
        if (NumApids < MAXAPIDS)
        {
          Apids[NumApids] = stream.getPid();
          Atypes[NumApids] = stream.getStreamType();
          SI::Descriptor * d;
          for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it));)
          {
            switch (d->getDescriptorTag())
            {
              case SI::ISO639LanguageDescriptorTag:
              {
                SI::ISO639LanguageDescriptor *         ld = (SI::ISO639LanguageDescriptor *) d;
                SI::ISO639LanguageDescriptor::Language l;
                char * s = ALangs[NumApids];
                int    n = 0;
                for (SI::Loop::Iterator it; ld->languageLoop.getNext(l, it);)
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
            DELETENULL(d);
          }
          NumApids++;
        }
        break;
      }
      case STREAMTYPE_13818_PRIVATE:
      case STREAMTYPE_13818_PES_PRIVATE:
      {
        int              dpid = 0;
        int              dtype = 0;
        char             lang[MAXLANGCODE1] = {0};
        SI::Descriptor * d;
        for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it));)
        {
          switch (d->getDescriptorTag())
          {
            case SI::AC3DescriptorTag:
            case EnhancedAC3DescriptorTag:
            case DTSDescriptorTag:
            case AACDescriptorTag:
              dpid = stream.getPid();
              dtype = d->getDescriptorTag();
              break;
            case SI::SubtitlingDescriptorTag:
              if (NumSpids < MAXSPIDS)
              {
                Spids[NumSpids] = stream.getPid();
                SI::SubtitlingDescriptor * sd = (SI::SubtitlingDescriptor *) d;
                SI::SubtitlingDescriptor::Subtitling sub;
                char * s = SLangs[NumSpids];
                int    n = 0;
                for (SI::Loop::Iterator it; sd->subtitlingLoop.getNext(sub, it);)
                {
                  if (sub.languageCode[0])
                  {
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
              Tpid = stream.getPid();
              break;
            case SI::ISO639LanguageDescriptorTag:
            {
              SI::ISO639LanguageDescriptor * ld = (SI::ISO639LanguageDescriptor *) d;
              strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
            }
            break;
            default:
              break;
          }
          DELETENULL(d);
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
      case STREAMTYPE_13818_USR_PRIVATE_81:
      {
        // ATSC AC-3; kls && Alex Lasnier
        if (Channel->IsAtsc())
        {
          char dlang[MAXLANGCODE1] = { 0 };
          SI::Descriptor *d;
          for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
          {
            switch (d->getDescriptorTag())
            {
              case SI::ISO639LanguageDescriptorTag:
              {
                SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                strn0cpy(dlang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
              }
              break;
              default:
                break;
            }
            DELETENULL(d);
          }
          if (NumDpids < MAXDPIDS)
          {
            Dpids[NumDpids] = stream.getPid();
            Dtypes[NumDpids] = SI::AC3DescriptorTag;
            strn0cpy(DLangs[NumDpids], dlang, MAXLANGCODE1);
            NumDpids++;
          }
        }
      }
      break;
      default:
        break;
    }
    for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)stream.streamDescriptors.getNext(it, SI::CaDescriptorTag));)
    {
      CaDescriptors->AddCaDescriptor(d, true);
      DELETENULL(d);
    }
  }

  Del(pmtPid, TableIdPMT);

  if (Vpid || Apids[0] || Dpids[0] || Tpid)
  {
    Channel->SetPids(Vpid, Vpid ? Ppid : 0, Vpid ? Vtype : 0, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
    Channel->SetCaIds(CaDescriptors->CaIds().data()); // TODO: Make Channel->SetCaIds accept a vector
    Channel->SetCaDescriptors(cCaDescriptorHandler::Get().AddCaDescriptors(CaDescriptors));
  }
  else
  {
    dsyslog("   PMT: PmtPid=%.5d Sid=%d is invalid (no audio/video)", pid, pmt.getServiceId());
    Channel->SetId(Channel->Nid(), Channel->Tid(), Channel->Sid(), INVALID_CHANNEL);
  }

  return;
}

}
