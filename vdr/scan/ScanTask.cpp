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

#include "ScanTask.h"
#include "CountryUtils.h"
#include "SatelliteUtils.h"
#include "channels/Channel.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "dvb/DiSEqC.h"
#include "dvb/filters/PAT.h"
#include "dvb/filters/SDT.h"
#include "dvb/filters/PSIP_VCT.h"
#include "settings/Settings.h"
#include "sources/linux/DVBTransponderParams.h"
#include "utils/log/Log.h"

#include <assert.h>

using namespace std;

namespace VDR
{

cScanTask::cScanTask(cDevice* device, const cFrontendCapabilities& caps)
 : m_caps(caps),
   m_device(device),
   m_abortableJob(NULL)
{
  assert(m_device);
}

void cScanTask::DoWork(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset, cSynchronousAbort* abortableJob /* = NULL */)
{
  ChannelPtr channel = GetChannel(modulation, iChannel, symbolRate, freqOffset);
  DoWork(channel, abortableJob);
}

void cScanTask::DoWork(const ChannelPtr& channel, cSynchronousAbort* abortableJob /* = NULL */)
{
  m_abortableJob = abortableJob;
  if (channel == cChannel::EmptyChannel)
    return;

  if (abortableJob && abortableJob->IsAborting())
    return;

  m_device->Channel()->SwitchChannel(channel);
  if (m_device->Channel()->HasLock(true))
  {
    cPat pat(m_device);
    ChannelVector patChannels = pat.GetChannels();

    // TODO: Use SDT for non-ATSC tuners
    cPsipVct vct(m_device);
    ChannelVector vctChannels = vct.GetChannels();

    for (ChannelVector::const_iterator it = patChannels.begin(); it != patChannels.end(); ++it)
    {
      const ChannelPtr& patChannel = *it;
      for (ChannelVector::const_iterator it2 = vctChannels.begin(); it2 != vctChannels.end(); ++it2)
      {
        const ChannelPtr& vctChannel = *it2;
        if (patChannel->GetTid() == vctChannel->GetTid() &&
            patChannel->GetSid() == vctChannel->GetSid())
        {
          patChannel->SetName(vctChannel->Name(), vctChannel->ShortName(), vctChannel->Provider());
          // TODO: Copy transponder data
          // TODO: Copy major/minor channel number

          cChannelManager::Get().AddChannel(patChannel);
        }
      }
    }
  }
}

unsigned int cScanTask::ChannelToFrequency(unsigned int channel, eChannelList channelList)
{
  int offset;
  unsigned int frequencyStep;
  if (CountryUtils::GetBaseOffset(channel, channelList, offset) &&
      CountryUtils::GetFrequencyStep(channel, channelList, frequencyStep))
    return offset + channel * frequencyStep;

  return 0;
}

cScanTaskTerrestrial::cScanTaskTerrestrial(cDevice* device, const cFrontendCapabilities& caps, eChannelList channelList)
 : cScanTask(device, caps),
   m_channelList(channelList)
{
}

ChannelPtr cScanTaskTerrestrial::GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset)
{
  ChannelPtr channel = ChannelPtr(new cChannel);

  unsigned int frequency = ChannelToFrequency(iChannel, m_channelList);
  if (frequency == 0)
    return cChannel::EmptyChannel; // Skip unused channels

  int iFrequencyOffset;
  if (!CountryUtils::GetFrequencyOffset(iChannel, m_channelList, freqOffset, iFrequencyOffset))
    return cChannel::EmptyChannel; // Skip this one
  frequency += iFrequencyOffset;

  fe_bandwidth bandwidth;
  if (!CountryUtils::GetBandwidth(iChannel, m_channelList, bandwidth))
    return cChannel::EmptyChannel;

  cDvbTransponderParams params;
  params.SetPolarization(POLARIZATION_HORIZONTAL); // (fe_polarization)0
  params.SetInversion(m_caps.caps_inversion);
  params.SetBandwidth(bandwidth); // Only loop-dependent variable
  params.SetCoderateH(m_caps.caps_fec);
  params.SetCoderateL(m_caps.caps_fec);
  params.SetModulation(m_caps.caps_qam);
  params.SetSystem(DVB_SYSTEM_1);
  params.SetTransmission(m_caps.caps_transmission_mode);
  params.SetGuard(m_caps.caps_guard_interval);
  params.SetHierarchy(m_caps.caps_hierarchy);
  params.SetRollOff(ROLLOFF_35); // (fe_rolloff)0

  channel->SetTransponderData(cSource::stTerr, frequency, 27500, params, true);
  channel->SetId(0, 0, 0, 0);

  return channel;
}

cScanTaskCable::cScanTaskCable(cDevice* device, const cFrontendCapabilities& caps, eChannelList channelList)
 : cScanTask(device, caps),
   m_channelList(channelList)
{
}

ChannelPtr cScanTaskCable::GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset)
{
  ChannelPtr channel = ChannelPtr(new cChannel);

  unsigned int frequency = ChannelToFrequency(iChannel, m_channelList);
  if (frequency == 0)
    return cChannel::EmptyChannel; // Skip unused channels

  int iFrequencyOffset;
  if (!CountryUtils::GetFrequencyOffset(iChannel, m_channelList, freqOffset, iFrequencyOffset))
    return cChannel::EmptyChannel; // Skip this one
  frequency += iFrequencyOffset;

  fe_modulation this_qam;

  // If caps_qam is not QAM_AUTO, then all modulations in dvbModulations will be tried
  // TODO: If caps_qam *is* QAM_AUTO, then the same code will be run multiple times (once for each dvbModulations item)

  if (m_caps.caps_qam == QAM_AUTO)
    this_qam = m_caps.caps_qam;
  else
  {
    // QAM_AUTO not supported, use value from modulation loop
    this_qam = modulation;
    string strModulation;
    switch (modulation)
    {
      case QPSK:          strModulation = "QAM0";     break;
      case QAM_16:        strModulation = "QAM16";    break;
      case QAM_32:        strModulation = "QAM32";    break;
      case QAM_64:        strModulation = "QAM64";    break;
      case QAM_128:       strModulation = "QAM128";   break;
      case QAM_256:       strModulation = "QAM256";   break;
      case QAM_AUTO:
      default:            strModulation = "QAM Auto"; break;
    }

    dsyslog("Searching %s...", strModulation.c_str());
  }

  cDvbTransponderParams params;
  params.SetPolarization(POLARIZATION_VERTICAL);
  params.SetInversion(INVERSION_OFF);
  params.SetBandwidth(BANDWIDTH_8_MHZ);
  params.SetCoderateH(FEC_NONE);
  params.SetCoderateL(FEC_NONE);
  params.SetModulation(this_qam); // Only loop-dependent variable (loop-independent if caps_qam == QAM_AUTO)
  params.SetSystem((eSystemType)SYS_DVBC_ANNEX_AC); // TODO: This should probably be DVB_SYSTEM_2!!!
  params.SetTransmission(TRANSMISSION_MODE_2K); // (fe_transmit_mode)0
  params.SetGuard(GUARD_INTERVAL_1_32); // (fe_guard_interval)0
  params.SetHierarchy(HIERARCHY_NONE); // (fe_hierarchy)0
  params.SetRollOff(ROLLOFF_35); // (fe_rolloff)0

  channel->SetTransponderData(cSource::stCable, 410000, 6900, params, true);
  channel->SetId(0, 0, 0, 0);

  return channel;
}

cScanTaskSatellite::cScanTaskSatellite(cDevice* device, const cFrontendCapabilities& caps, SATELLITE::eSatellite satelliteId)
 : cScanTask(device, caps),
   m_satelliteId(satelliteId)
{
}

bool cScanTaskSatellite::ValidSatFrequency(unsigned int frequencyHz, const cChannel& channel)
{
  bool result = true;

  unsigned int frequencyMHz = frequencyHz / 1000 / 1000;

  if (cSettings::Get().m_bDiSEqC)
  {
    cDvbTransponderParams params(channel.Parameters());
    cDiseqc *diseqc = GetDiseqc(channel.Source(), channel.FrequencyHz(), params.Polarization());

    if (diseqc)
      frequencyMHz -= diseqc->Lof();
    else
      return false;
  }
  else
  {
    if (frequencyMHz < cSettings::Get().m_iLnbSLOF)
      frequencyMHz -= cSettings::Get().m_iLnbFreqLow;
    else
      frequencyMHz -= cSettings::Get().m_iLnbFreqHigh;
  }

  return frequencyMHz >= 950 && frequencyMHz <= 2150;
}

cDiseqc* cScanTaskSatellite::GetDiseqc(int source, int frequency, char polarization)
{
  for (cDiseqc *p = Diseqcs.First(); p; p = Diseqcs.Next(p))
  {
    if (p->Source() == source && p->Slof() > frequency && p->Polarization() == toupper(polarization))
      return p;
  }

  return NULL;
}

ChannelPtr cScanTaskSatellite::GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset)
{
  ChannelPtr channel = ChannelPtr(new cChannel);

  cDvbTransponderParams params;
  params.SetPolarization(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).polarization);
  params.SetInversion(INVERSION_OFF);
  params.SetBandwidth(BANDWIDTH_8_MHZ); // (fe_bandwidth)0
  params.SetCoderateH(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).fec_inner);
  params.SetCoderateL(FEC_NONE);
  params.SetModulation(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).modulation_type);
  params.SetSystem(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).modulation_system == SYS_DVBS ? DVB_SYSTEM_1 : DVB_SYSTEM_2);
  params.SetTransmission(TRANSMISSION_MODE_2K); // (fe_transmit_mode)0
  params.SetGuard(GUARD_INTERVAL_1_32); // (fe_guard_interval)0
  params.SetHierarchy(HIERARCHY_NONE); // (fe_hierarchy)0
  params.SetRollOff(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).rolloff);

  channel->SetTransponderData(cSource::FromString(SatelliteUtils::GetSatellite(m_satelliteId).source_id),
                             SatelliteUtils::GetTransponder(m_satelliteId, iChannel).intermediate_frequency,
                             SatelliteUtils::GetTransponder(m_satelliteId, iChannel).symbol_rate,
                             params,
                             true);

  if (!ValidSatFrequency(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).intermediate_frequency, *channel))
    return cChannel::EmptyChannel;

  channel->SetId(0, 0, 0, 0);

  if (SatelliteUtils::GetTransponder(m_satelliteId, iChannel).modulation_system == SYS_DVBS2)
  {
    if (!m_caps.caps_s2)
    {
      dsyslog("%d: skipped (%s)", SatelliteUtils::GetTransponder(m_satelliteId, iChannel).intermediate_frequency, "no driver support");
      return cChannel::EmptyChannel;
    }
  }

  return channel;
}

cScanTaskATSC::cScanTaskATSC(cDevice* device, const cFrontendCapabilities& caps, eChannelList channelList)
 : cScanTask(device, caps),
   m_channelList(channelList)
{
}

ChannelPtr cScanTaskATSC::GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset)
{
  unsigned int frequency = ChannelToFrequency(iChannel, m_channelList);
  if (frequency == 0)
    return cChannel::EmptyChannel; // Skip unused channels

  int iFrequencyOffset;
  if (!CountryUtils::GetFrequencyOffset(iChannel, m_channelList, freqOffset, iFrequencyOffset))
    return cChannel::EmptyChannel; // Skip this one
  frequency += iFrequencyOffset;

  // wirbelscan comment: "FIXME: VSB vs QAM here"

  cDvbTransponderParams params;
  params.SetPolarization(POLARIZATION_VERTICAL);
  params.SetInversion(m_caps.caps_inversion);
  params.SetBandwidth(BANDWIDTH_8_MHZ); // Should probably be 8000000 (8MHz) or BANDWIDTH_8_MHZ
  params.SetCoderateH(m_caps.caps_fec);
  params.SetCoderateL(FEC_NONE);
  params.SetModulation(modulation); // Only loop-dependent variable
  params.SetSystem((eSystemType)SYS_DVBC_ANNEX_AC); // TODO: This should probably be DVB_SYSTEM_2!!!
  params.SetTransmission(TRANSMISSION_MODE_2K); // (fe_transmit_mode)0
  params.SetGuard(GUARD_INTERVAL_1_32); // (fe_guard_interval)0
  params.SetHierarchy(HIERARCHY_NONE); // (fe_hierarchy)0
  params.SetRollOff(ROLLOFF_35); // (fe_rolloff)0

  ChannelPtr channel = ChannelPtr(new cChannel);
  channel->SetTransponderData(cSource::stAtsc, frequency, cScanConfig::TranslateSymbolRate(symbolRate), params, true);
  channel->SetId(0, 0, 0, 0);

  return channel;
}

}
