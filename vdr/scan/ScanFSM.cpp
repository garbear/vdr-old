/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "ScanFSM.h"

#include <assert.h>

using namespace VDR::SCAN_FSM;

namespace VDR
{

#define ABSOLUTE_DIFFERENCE(f1, f2)  ((f1) >= (f2) ? (f1) - (f2) : (f2) - (f1))

cScanFsm::cScanFsm(cDevice* device, cChannelManager* channelManager, const ChannelPtr& transponder, cSynchronousAbort* abortableJob /* = NULL */)
 : m_device(device),
   m_channelManager(channelManager),
   m_currentTransponder(transponder),
   m_abortableJob(abortableJob),
   m_swReceiver(transponder)
{
  assert(m_device && channelManager && m_currentTransponder);
  m_transponders.push_back(m_currentTransponder);
}

template <> void cScanFsm::Enter<eStart>()           { ReportState("Start"); }
template <> void cScanFsm::Enter<eTune>()            { ReportState("Tune"); }
template <> void cScanFsm::Enter<eNextTransponder>() { ReportState("NextTransponder"); }
template <> void cScanFsm::Enter<eDetachReceiver>()  { ReportState("DetachReceiver"); }
template <> void cScanFsm::Enter<eScanNit>()         { ReportState("ScanNit"); }
template <> void cScanFsm::Enter<eScanPat>()         { ReportState("ScanPat"); }
template <> void cScanFsm::Enter<eScanSdt>()         { ReportState("ScanSdt"); }
template <> void cScanFsm::Enter<eAddChannels>()     { ReportState("AddChannels"); }
template <> void cScanFsm::Enter<eScanEit>()         { ReportState("ScanEit"); }
template <> void cScanFsm::Enter<eStop>()            { ReportState("Stop");
                                                       m_device->Receiver()->DetachAllReceivers(); }

template <> int cScanFsm::Event<eStart>()
{
  m_device->Channel()->SwitchChannel(m_currentTransponder);

  m_device->Receiver()->AttachReceiver(&m_swReceiver);

  return eTune;
}

template <> int cScanFsm::Event<eTune>()
{
  if (IsKnownInitialTransponder(*m_currentTransponder, false, false))
  {
    dsyslog("Channel is a known initial transponder, skipping");
    return eNextTransponder;
  }

  m_scannedTransponders.push_back(m_currentTransponder->Clone());

  m_device->Channel()->SwitchChannel(m_currentTransponder);

  if (m_device->Channel()->HasLock(LOCK_TIMEOUT_MS))
    return eScanNit;
  else
    return eNextTransponder;
}

template <> int cScanFsm::Event<eNextTransponder>()
{
  // Get the next transponder from cScanFilter::NewTransponders
  ChannelVector::const_iterator it = std::find(m_transponders.begin(), m_transponders.end(), m_currentTransponder);

  if (it == m_transponders.end())
  {
    esyslog("Transponder is not a known transponder");
    return eStop; // Not in the vector
  }

  if (it + 1 == m_transponders.end())
    return eStop; // End of the vector

  m_currentTransponder = *(it + 1);
  return eTune;
}

template <> int cScanFsm::Event<eDetachReceiver>()
{
  m_device->Receiver()->DetachAllReceivers();

  return eNextTransponder;
}

template <> int cScanFsm::Event<eScanNit>()
{
  cNitScanner nitScanner(this, true);
  cNitScanner nitOtherScanner(this, false);

  m_device->SectionFilter()->AttachFilter(&nitScanner);
  m_device->SectionFilter()->AttachFilter(&nitOtherScanner);

  bool aborted = false;
  if (m_abortableJob)
    aborted = m_abortableJob->WaitForAbort(NIT_SCAN_TIMEOUT_MS);
  else
    usleep(NIT_SCAN_TIMEOUT_MS * 1000);

  m_device->SectionFilter()->Detach(&nitScanner);
  m_device->SectionFilter()->Detach(&nitOtherScanner);

  return aborted ? eStop : eScanPat;
}

template <> int cScanFsm::Event<eScanPat>()
{
  cPatScanner patScanner(this);

  m_device->SectionFilter()->AttachFilter(&patScanner);

  bool aborted = false;
  if (m_abortableJob)
    aborted = m_abortableJob->WaitForAbort(PAT_SCAN_TIMEOUT_MS);
  else
    usleep(PAT_SCAN_TIMEOUT_MS * 1000);

  m_device->SectionFilter()->Detach(&patScanner);

  // Clean up PMT scanners created in PatFoundChannel()
  for (std::vector<cPmtScanner*>::iterator it = m_pmtScanners.begin(); it != m_pmtScanners.end(); ++it)
  {
    m_device->SectionFilter()->Detach(*it);
    delete *it;
  }
  m_pmtScanners.clear();

  return aborted ? eStop : eScanSdt;
}

template <> int cScanFsm::Event<eScanSdt>()
{
  cSdtScanner sdtScanner(this);

  m_device->SectionFilter()->AttachFilter(&sdtScanner);

  bool aborted = false;
  if (m_abortableJob)
    aborted = m_abortableJob->WaitForAbort(SDT_SCAN_TIMEOUT_MS);
  else
    usleep(SDT_SCAN_TIMEOUT_MS * 1000);

  m_device->SectionFilter()->Detach(&sdtScanner);

  return aborted ? eStop : eAddChannels;
}

template <> int cScanFsm::Event<eAddChannels>()
{
  for (ChannelVector::iterator channelIt = m_newChannels.begin(); channelIt != m_newChannels.end(); ++channelIt)
  {
    const ChannelPtr& channel = *channelIt;

    if (!channel->Vpid() && ! channel->Apid(0) && ! channel->Dpid(0) &&
        !channel->Tpid() && ! channel->Ca() &&
        channel->Name() == CHANNEL_NAME_UNKNOWN)
    {
      continue;
    }

    if (!m_channelManager->HasUniqueChannelID(channel))
    {
      ChannelPtr existingChannel = m_channelManager->GetByChannelID(channel->GetChannelID(), false, false);
      if (existingChannel)
      {
        int i;
        char ALangs[MAXAPIDS][MAXLANGCODE2] = { "" };
        char DLangs[MAXDPIDS][MAXLANGCODE2] = { "" };
        char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
        int  Atypes[MAXAPIDS + 1]           = {0};
        int  Dtypes[MAXDPIDS + 1]           = {0};

        for (i = 0; i < MAXAPIDS; i++)
        {
          int len = strlen(channel->Alang(i));
          if (len < 1)
            break;
          strncpy(ALangs[i], channel->Alang(i), std::min(len, MAXLANGCODE2));
        }

        for (i = 0; i < MAXAPIDS; i++)
        {
          int len = strlen(channel->Dlang(i));
          if (len < 1)
            break;
          strncpy(DLangs[i], channel->Dlang(i), std::min(len, MAXLANGCODE2));
        }

        for (i = 0; i < MAXAPIDS; i++)
        {
          int len = strlen(channel->Slang(i));
          if (len < 1)
            break;
          strncpy(SLangs[i], channel->Slang(i), std::min(len, MAXLANGCODE2));
        }

        for (i = 0; i < MAXAPIDS; i++)
          Atypes[i] = channel->Atype(i);

        for (i = 0; i < MAXDPIDS; i++)
          Dtypes[i] = channel->Dtype(i);

        if (channel->Vpid() || channel->Apid(0) || channel->Dpid(0))
        {
          // TODO: Remove const_casts
          existingChannel->SetPids(channel->Vpid(), channel->Ppid(), channel->Vtype(),
              const_cast<int*>(channel->Apids()), Atypes, ALangs,
              const_cast<int*>(channel->Dpids()), Dtypes, DLangs,
              const_cast<int*>(channel->Spids()), SLangs,
              channel->Tpid());
        }
        if (channel->Name() != CHANNEL_NAME_UNKNOWN)
          existingChannel->SetName(channel->Name(), channel->ShortName(), channel->Provider());

        // Successfully updated (existing) channel
      }
    }
    else
    {
      // Include all channels for now
      eScanFlags scanFlags = (eScanFlags)(SCAN_TV | SCAN_RADIO | SCAN_FTA | SCAN_SCRAMBLED | SCAN_HD);

      if (channel->Ca() && !(scanFlags & SCAN_SCRAMBLED))
        continue; // Channel is scrambled

      if (!channel->Ca() && !(scanFlags & SCAN_FTA))
        continue; // FTA channel

      if (!channel->Vpid() && (channel->Apid(0) || channel->Dpid(0)) && ! (scanFlags & SCAN_RADIO))
        continue; // Radio channel

      if (channel->Vpid() && !(scanFlags & SCAN_TV))
        continue; // TV channel

      if ((channel->Vtype() > 2) && !(scanFlags & SCAN_HD))
        continue; // HDTV channel

      m_channelManager->AddChannel(channel->Clone()); // TODO: Can we add the channel directly?
    }
  }

  m_channelManager->ReNumber();

  return eScanEit;
}

template <> int cScanFsm::Event<eScanEit>()
{
  // TODO: EIT scanner might be needed
  /*
  cEitScanner eitScanner;

  m_device->SectionFilter()->AttachFilter(&eitScanner);

  bool aborted = false;
  if (m_abortableJob)
    aborted = m_abortableJob->WaitForAbort(EIT_SCAN_TIMEOUT_MS);
  else
    usleep(EIT_SCAN_TIMEOUT_MS * 1000);

  m_device->SectionFilter()->Detach(&eitScanner);

  return aborted ? eStop : eDetachReceiver;
  */
  return eStop;
}

void cScanFsm::NitFoundTransponder(ChannelPtr transponder, bool bOtherTable, int originalNid, int originalTid)
{
  if (!IsKnownInitialTransponder(*transponder, true))
  {
    dsyslog("   Add transponder: NID = %d, TID = %d", transponder->Nid(), transponder->Tid());
    m_transponders.push_back(transponder);
  }
  else
  {
    // we already know this transponder, lets update these channels
    ChannelPtr updateTransponder = GetScannedTransponderByParams(*transponder);
    if (!bOtherTable && updateTransponder)
    {
      // only NIT_actual should update existing channels
      if (IsDifferentTransponderDeepScan(*transponder, *updateTransponder, false))
        updateTransponder->SetTransponderData(transponder->Source(), transponder->FrequencyHz(), transponder->Srate(), transponder->Parameters(), true);

      if ((originalNid != updateTransponder->Nid()) ||
          (originalTid != updateTransponder->Tid()))
      {
        updateTransponder->SetId(originalNid, originalTid, 0, 0);
        dsyslog("   Update transponder: NID = %d, TID = %d", updateTransponder->Nid(), updateTransponder->Tid());
      }
    }
  }
}

void cScanFsm::PatFoundChannel(ChannelPtr channel, int pid)
{
  ChannelPtr scanned = GetScannedTransponderByParams(*channel);
  if (scanned)
    channel->SetId(scanned->Nid(), channel->Tid(), channel->Sid()); // Use scanned channel's NID

  if (!GetByTransponder(m_newChannels, *channel))
  {
    m_newChannels.push_back(channel);
    dsyslog("      Added channel with service ID %d", channel->Sid());

    // TODO: Need a lock for m_pmtScanners
    cPmtScanner* pmtScanner = new cPmtScanner(channel, channel->Sid(), pid);
    m_pmtScanners.push_back(pmtScanner);
    m_device->SectionFilter()->AttachFilter(pmtScanner);
  }
}

ChannelPtr cScanFsm::SdtFoundService(ChannelPtr channel, int nid, int tid, int sid)
{
  assert((bool)channel);
  ChannelPtr newChannel = SdtGetByService(channel->Source(), tid, sid);
  if (newChannel)
  {
    newChannel->SetId(nid, tid, sid); // Update NID
    return newChannel;
  }

  // Before returning an empty channel ptr, add a new channel and transponder to their respective vectors
  newChannel = ChannelPtr(new cChannel);
  newChannel->CopyTransponderData(*channel);

  if (!IsKnownInitialTransponder(*newChannel, true))
  {
    ChannelPtr transponder = newChannel->Clone();
    m_transponders.push_back(transponder);
    dsyslog("   SDT: Add transponder");
  }

  newChannel->SetId(nid, tid, sid);
  dsyslog("   SDT: Add channel (NID = %d, TID = %d, SID = %d)", nid, tid, sid);
  m_newChannels.push_back(newChannel);

  return cChannel::EmptyChannel;
}

ChannelPtr cScanFsm::SdtGetByService(int source, int tid, int sid)
{
  // TODO: Need to lock m_newChannels
  for (ChannelVector::iterator channelIt = m_newChannels.begin(); channelIt != m_newChannels.end(); ++channelIt)
  {
    ChannelPtr& channel = *channelIt;
    if (channel->Source() == source && channel->Tid() == tid && channel->Sid() == sid)
      return channel;
  }
  return cChannel::EmptyChannel;
}

void cScanFsm::SdtFoundChannel(ChannelPtr channel, int nid, int tid, int sid, char* name, char* shortName, char* provider)
{
  // TODO: Need to lock vectors
  if (!IsKnownInitialTransponder(*channel, true))
  {
    ChannelPtr transponder = channel->Clone();
    m_transponders.push_back(transponder);
  }
  channel->SetId(nid, tid, sid, 0);
  channel->SetName(name, shortName, provider);
  m_newChannels.push_back(channel);
}

bool cScanFsm::IsKnownInitialTransponder(const cChannel& newChannel, bool bAutoAllowed, bool bIncludeUnscannedTransponders /* = true */) const
{
  ChannelVector list = m_scannedTransponders;
  if (bIncludeUnscannedTransponders)
    list.insert(list.end(), m_transponders.begin(), m_transponders.end());

  for (ChannelVector::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    const cChannel& channel = **it;
    if (channel.GroupSep())
      continue;

    if (newChannel.IsTerr())
    {
      if ((channel.Source() == newChannel.Source()) && IsNearlySameFrequency(channel, newChannel))
        return true;
    }
    if (newChannel.IsCable())
    {
      if (newChannel.Ca() != 0xA1)
      {
        if ((channel.Source() == newChannel.Source()) && IsNearlySameFrequency(channel, newChannel))
          return true;
      }
      else
      {
        if ((channel.Ca() == newChannel.Ca())         &&
            (channel.Source() == newChannel.Source()) &&
            IsNearlySameFrequency(channel, newChannel, 50))
        {
          return true;
        }
      }
    }
    if (newChannel.IsAtsc())
    {
      cDvbTransponderParams p(channel.Parameters());
      cDvbTransponderParams pn(newChannel.Parameters());

      if ((channel.Source() == newChannel.Source())  &&
          IsNearlySameFrequency(channel, newChannel) &&
          (p.Modulation() == pn.Modulation()))
      {
        return true;
      }
    }
    if (newChannel.IsSat())
    {
      if (!IsDifferentTransponderDeepScan(newChannel, channel, bAutoAllowed))
        return true;
    }
  }
  return false;
}

bool cScanFsm::IsNearlySameFrequency(const cChannel& channel1, const cChannel& channel2, unsigned int maxDeltaHz /* = 2001 */)
{
  uint32_t diff;
  unsigned int f1 = channel1.FrequencyHz();
  unsigned int f2 = channel2.FrequencyHz();

  if (f1 == f2)
    return true;

  diff = ABSOLUTE_DIFFERENCE(f1, f2);

  // FIXME: use symbolrate etc. to estimate bandwidth

  if (diff < maxDeltaHz)
    return true;

  return false;
}

template <typename T>
bool IsDifferent(T paramChannel1, T paramChannel2, bool bAutoAllowed, T autoValue)
{
  if (paramChannel1 == paramChannel2)
    return false;

  if (bAutoAllowed)
    return paramChannel1 != autoValue && paramChannel2 != autoValue;
  else
    return true;
}

bool cScanFsm::IsDifferentTransponderDeepScan(const cChannel& channel1, const cChannel& channel2, bool bAutoAllowed)
{
  int maxdelta = 2001;

  if (channel1.IsSat())
    maxdelta = 2;

  if (channel1.Source() != channel2.Source())
    return true;

  if (!IsNearlySameFrequency(channel1, channel2, maxdelta))
    return true;

  cDvbTransponderParams paramsChannel1(channel1.Parameters());
  cDvbTransponderParams paramsChannel2(channel2.Parameters());

  if (channel1.IsTerr())
  {
    if (IsDifferent(paramsChannel1.Modulation(), paramsChannel2.Modulation(), bAutoAllowed, QAM_AUTO))
      return true;
    if (IsDifferent(paramsChannel1.Bandwidth(), paramsChannel2.Bandwidth(), bAutoAllowed, BANDWIDTH_AUTO))
      return true;
    if (IsDifferent(paramsChannel1.CoderateH(), paramsChannel2.CoderateH(), bAutoAllowed, FEC_AUTO))
      return true;
    if (IsDifferent(paramsChannel1.Hierarchy(), paramsChannel2.Hierarchy(), bAutoAllowed, HIERARCHY_AUTO))
      return true;
    if (IsDifferent(paramsChannel1.CoderateL(), paramsChannel2.CoderateL(), bAutoAllowed, FEC_AUTO))
      return true;
    if (IsDifferent(paramsChannel1.Transmission(), paramsChannel2.Transmission(), bAutoAllowed, TRANSMISSION_MODE_AUTO))
      return true;
    if (IsDifferent(paramsChannel1.Guard(), paramsChannel2.Guard(), bAutoAllowed, GUARD_INTERVAL_AUTO))
      return true;
    return false;
  }

  if (channel1.IsAtsc())
  {
    if (IsDifferent(paramsChannel1.Modulation(), paramsChannel2.Modulation(), bAutoAllowed, QAM_AUTO))
      return true;
    return false;
  }

  if (channel1.IsCable())
  {
    if (IsDifferent(paramsChannel1.Modulation(), paramsChannel2.Modulation(), bAutoAllowed, QAM_AUTO))
      return true;
    if (IsDifferent(channel1.Srate(), channel2.Srate(), false, 6900))
      return true;
    if (IsDifferent(paramsChannel1.CoderateH(), paramsChannel2.CoderateH(), bAutoAllowed, FEC_AUTO))
      return true;
    return false;
  }

  if (channel1.IsSat())
  {
    if (IsDifferent(channel1.Srate(), channel2.Srate(), false, 27500))
      return true;
    if (IsDifferent(paramsChannel1.Polarization(), paramsChannel2.Polarization(), false, POLARIZATION_HORIZONTAL))
      return true;
    if (IsDifferent(paramsChannel1.CoderateH(), paramsChannel2.CoderateH(), bAutoAllowed, FEC_AUTO))
      return true;
    if (IsDifferent(paramsChannel1.System(), paramsChannel2.System(), false, DVB_SYSTEM_1))
      return true;
    if (IsDifferent(paramsChannel1.RollOff(), paramsChannel2.RollOff(), bAutoAllowed, ROLLOFF_AUTO))
      return true;
    if (IsDifferent(paramsChannel1.Modulation(), paramsChannel2.Modulation(), bAutoAllowed, QPSK))
      return true;
    return false;
  }

  return true;
}

ChannelPtr cScanFsm::GetScannedTransponderByParams(const cChannel& newTransponder)
{
  for (ChannelVector::iterator channelIt = m_scannedTransponders.begin(); channelIt != m_scannedTransponders.end(); ++channelIt)
  {
    if (!IsDifferentTransponderDeepScan(**channelIt, newTransponder, true))
      return *channelIt;
  }
  return cChannel::EmptyChannel;
}

ChannelPtr cScanFsm::GetByTransponder(ChannelVector& channels, const cChannel& transponder)
{
  unsigned int maxdelta = 2001;

  if (transponder.IsSat())
    maxdelta = 2;

  for (ChannelVector::iterator channelIt = channels.begin(); channelIt != channels.end(); ++channelIt)
  {
    ChannelPtr& channel = *channelIt;
    if (IsNearlySameFrequency(*channel, transponder, maxdelta) &&
        channel->Source() == transponder.Source()              &&
        channel->Tid()    == transponder.Tid()                 &&
        channel->Sid()    == transponder.Sid())
    {
      return channel;
    }
  }
  return cChannel::EmptyChannel;
}

}
