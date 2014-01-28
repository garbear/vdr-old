/*
 * pat.c: PAT section filter
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: pat.c 2.19 2012/11/25 14:12:21 kls Exp $
 */

#include "PAT.h"
#include "CADescriptorHandler.h"
#include <malloc.h>
#include "channels/ChannelManager.h"
#include "libsi/section.h"
#include "libsi/descriptor.h"
#include "utils/I18N.h"
#include "Config.h"
#include "platform/threads/threads.h"

#define PMT_SCAN_TIMEOUT  10 // seconds

using namespace std;

// --- cPatFilter ------------------------------------------------------------

cPatFilter::cPatFilter(void)
{
  pmtIndex = 0;
  pmtPid = 0;
  pmtSid = 0;
  lastPmtScan = 0;
  numPmtEntries = 0;
  Set(0x00, 0x00);  // PAT
}

void cPatFilter::SetStatus(bool On)
{
  cFilter::SetStatus(On);
  pmtIndex = 0;
  pmtPid = 0;
  pmtSid = 0;
  lastPmtScan = 0;
  numPmtEntries = 0;
}

void cPatFilter::Trigger(void)
{
  numPmtEntries = 0;
}

bool cPatFilter::PmtVersionChanged(int PmtPid, int Sid, int Version)
{
  uint64_t v = Version;
  v <<= 32;
  uint64_t id = (PmtPid | (Sid << 16)) & 0x00000000FFFFFFFFLL;
  for (int i = 0; i < numPmtEntries; i++) {
      if ((pmtVersion[i] & 0x00000000FFFFFFFFLL) == id) {
         bool Changed = (pmtVersion[i] & 0x000000FF00000000LL) != v;
         if (Changed)
            pmtVersion[i] = id | v;
         return Changed;
         }
      }
  if (numPmtEntries < MAXPMTENTRIES)
     pmtVersion[numPmtEntries++] = id | v;
  return true;
}

void cPatFilter::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  if (Pid == 0x00) {
     if (Tid == 0x00) {
        if (pmtPid && time(NULL) - lastPmtScan > PMT_SCAN_TIMEOUT) {
           Del(pmtPid, 0x02);
           pmtPid = 0;
           pmtIndex++;
           lastPmtScan = time(NULL);
           }
        if (!pmtPid) {
           SI::PAT pat(Data, false);
           if (!pat.CheckCRCAndParse())
              return;
           SI::PAT::Association assoc;
           int Index = 0;
           for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); ) {
               if (!assoc.isNITPid()) {
                  if (Index++ >= pmtIndex && cChannelManager::Get().GetByServiceID(Source(), Transponder(), assoc.getServiceId())) {
                     pmtPid = assoc.getPid();
                     pmtSid = assoc.getServiceId();
                     Add(pmtPid, 0x02);
                     break;
                     }
                  }
               }
           if (!pmtPid)
              pmtIndex = 0;
           }
        }
     }
  else if (Pid == pmtPid && Tid == SI::TableIdPMT && Source() && Transponder()) {
     SI::PMT pmt(Data, false);
     if (!pmt.CheckCRCAndParse())
        return;
     if (pmt.getServiceId() != pmtSid)
        return; // skip broken PMT records
     if (!PmtVersionChanged(pmtPid, pmt.getTableIdExtension(), pmt.getVersionNumber())) {
        lastPmtScan = 0; // this triggers the next scan
        return;
        }
     //XXX
//     if (!Channels.Lock(true, 10)) {
//        numPmtEntries = 0; // to make sure we try again
//        return;
//        }
     ChannelPtr Channel = cChannelManager::Get().GetByServiceID(Source(), Transponder(), pmt.getServiceId());
     if (Channel) {
        SI::CaDescriptor *d;
        CaDescriptorsPtr CaDescriptors = CaDescriptorsPtr(new cCaDescriptors(Channel->Source(), Channel->Transponder(), Channel->Sid()));
        // Scan the common loop:
        for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)pmt.commonDescriptors.getNext(it, SI::CaDescriptorTag)); ) {
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
            switch (stream.getStreamType()) {
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
                      if (NumApids < MAXAPIDS) {
                         Apids[NumApids] = esPid;
                         Atypes[NumApids] = stream.getStreamType();
                         SI::Descriptor *d;
                         for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                             switch (d->getDescriptorTag()) {
                               case SI::ISO639LanguageDescriptorTag: {
                                    SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                    SI::ISO639LanguageDescriptor::Language l;
                                    char *s = ALangs[NumApids];
                                    int n = 0;
                                    for (SI::Loop::Iterator it; ld->languageLoop.getNext(l, it); ) {
                                        if (*ld->languageCode != '-') { // some use "---" to indicate "none"
                                           if (n > 0)
                                              *s++ = '+';
                                           strn0cpy(s, I18nNormalizeLanguageCode(l.languageCode), MAXLANGCODE1);
                                           s += strlen(s);
                                           if (n++ > 1)
                                              break;
                                           }
                                        }
                                    }
                                    break;
                               default: ;
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
                      for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                          switch (d->getDescriptorTag()) {
                            case SI::AC3DescriptorTag:
                            case SI::EnhancedAC3DescriptorTag:
                                 dpid = esPid;
                                 dtype = d->getDescriptorTag();
                                 ProcessCaDescriptors = true;
                                 break;
                            case SI::SubtitlingDescriptorTag:
                                 if (NumSpids < MAXSPIDS) {
                                    Spids[NumSpids] = esPid;
                                    SI::SubtitlingDescriptor *sd = (SI::SubtitlingDescriptor *)d;
                                    SI::SubtitlingDescriptor::Subtitling sub;
                                    char *s = SLangs[NumSpids];
                                    int n = 0;
                                    for (SI::Loop::Iterator it; sd->subtitlingLoop.getNext(sub, it); ) {
                                        if (sub.languageCode[0]) {
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
                            case SI::ISO639LanguageDescriptorTag: {
                                 SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                 strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                                 }
                                 break;
                            default: ;
                            }
                          delete d;
                          }
                      if (dpid) {
                         if (NumDpids < MAXDPIDS) {
                            Dpids[NumDpids] = dpid;
                            Dtypes[NumDpids] = dtype;
                            strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
                            NumDpids++;
                            }
                         }
                      }
                      break;
              case 0x80: // STREAMTYPE_USER_PRIVATE
                      if (g_setup.StandardCompliance == STANDARD_ANSISCTE) { // DigiCipher II VIDEO (ANSI/SCTE 57)
                         Vpid = esPid;
                         Ppid = pmt.getPCRPid();
                         Vtype = 0x02; // compression based upon MPEG-2
                         ProcessCaDescriptors = true;
                         break;
                         }
                      // fall through
              case 0x81: // STREAMTYPE_USER_PRIVATE
                      if (g_setup.StandardCompliance == STANDARD_ANSISCTE) { // ATSC A/53 AUDIO (ANSI/SCTE 57)
                         char lang[MAXLANGCODE1] = { 0 };
                         SI::Descriptor *d;
                         for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                             switch (d->getDescriptorTag()) {
                               case SI::ISO639LanguageDescriptorTag: {
                                    SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                    strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                                    }
                                    break;
                               default: ;
                               }
                            delete d;
                            }
                         if (NumDpids < MAXDPIDS) {
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
                      if (g_setup.StandardCompliance == STANDARD_ANSISCTE) { // STANDARD SUBTITLE (ANSI/SCTE 27)
                         //TODO
                         break;
                         }
                      // fall through
              case 0x83 ... 0xFF: // STREAMTYPE_USER_PRIVATE
                      {
                      char lang[MAXLANGCODE1] = { 0 };
                      bool IsAc3 = false;
                      SI::Descriptor *d;
                      for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                          switch (d->getDescriptorTag()) {
                            case SI::RegistrationDescriptorTag: {
                                 SI::RegistrationDescriptor *rd = (SI::RegistrationDescriptor *)d;
                                 // http://www.smpte-ra.org/mpegreg/mpegreg.html
                                 switch (rd->getFormatIdentifier()) {
                                   case 0x41432D33: // 'AC-3'
                                        IsAc3 = true;
                                        break;
                                   default:
                                        //printf("Format identifier: 0x%08X (pid: %d)\n", rd->getFormatIdentifier(), esPid);
                                        break;
                                   }
                                 }
                                 break;
                            case SI::ISO639LanguageDescriptorTag: {
                                 SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                 strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                                 }
                                 break;
                            default: ;
                            }
                         delete d;
                         }
                      if (IsAc3) {
                         if (NumDpids < MAXDPIDS) {
                            Dpids[NumDpids] = esPid;
                            Dtypes[NumDpids] = SI::AC3DescriptorTag;
                            strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
                            NumDpids++;
                            }
                         ProcessCaDescriptors = true;
                         }
                      }
                      break;
              default: ;//printf("PID: %5d %5d %2d %3d %3d\n", pmt.getServiceId(), stream.getPid(), stream.getStreamType(), pmt.getVersionNumber(), Channel->Number());
              }
            if (ProcessCaDescriptors) {
               for (SI::Loop::Iterator it; (d = (SI::CaDescriptor*)stream.streamDescriptors.getNext(it, SI::CaDescriptorTag)); ) {
                   CaDescriptors->AddCaDescriptor(d, esPid);
                   delete d;
                   }
               }
            }
        if (g_setup.UpdateChannels >= 2) {
           Channel->SetPids(Vpid, Ppid, Vtype, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
           Channel->SetCaIds(CaDescriptors->CaIds().data());
           Channel->SetSubtitlingDescriptors(SubtitlingTypes, CompositionPageIds, AncillaryPageIds);
           }
        Channel->SetCaDescriptors(cCaDescriptorHandler::Get().AddCaDescriptors(CaDescriptors));
        Channel->NotifyObservers(ObservableMessageChannelChanged);
        }
     lastPmtScan = 0; // this triggers the next scan
//     XXX Channels.Unlock();
     }
}
