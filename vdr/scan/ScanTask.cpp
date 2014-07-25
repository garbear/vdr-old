/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

void cScanTask::DoWork(const ChannelPtr& channel, cSynchronousAbort* abortableJob /* = NULL */, iScanCallback* callback /* = NULL */)
{
  m_abortableJob = abortableJob;
  if (channel == cChannel::EmptyChannel)
    return;

  if (abortableJob && abortableJob->IsAborting())
    return;

  if (callback)
    callback->ScanFrequency(channel->GetTransponder().FrequencyHz());

  if (m_device->Channel()->SwitchChannel(channel))
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
        if (patChannel->Tsid() == vctChannel->Tsid() &&
            patChannel->Sid()  == vctChannel->Sid())
        {
          patChannel->SetName(vctChannel->Name(), vctChannel->ShortName(), vctChannel->Provider());
          // TODO: Copy transponder data
          // TODO: Copy major/minor channel number
        }
      }
    }

    if (!patChannels.empty())
      cChannelManager::Get().AddChannels(patChannels);
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

  unsigned int frequencyHz = ChannelToFrequency(iChannel, m_channelList);
  if (frequencyHz == 0)
    return cChannel::EmptyChannel; // Skip unused channels

  int iFrequencyOffset;
  if (!CountryUtils::GetFrequencyOffset(iChannel, m_channelList, freqOffset, iFrequencyOffset))
    return cChannel::EmptyChannel; // Skip this one
  frequencyHz += iFrequencyOffset;

  fe_bandwidth bandwidth;
  if (!CountryUtils::GetBandwidth(iChannel, m_channelList, bandwidth))
    return cChannel::EmptyChannel;

  cTransponder transponder(TRANSPONDER_TERRESTRIAL);
  transponder.SetFrequencyHz(frequencyHz);
  transponder.SetInversion(m_caps.caps_inversion);
  transponder.TerrestrialParams().SetBandwidth(bandwidth); // Only loop-dependent variable
  transponder.TerrestrialParams().SetCoderateH(m_caps.caps_fec);
  transponder.TerrestrialParams().SetCoderateL(m_caps.caps_fec);
  transponder.SetModulation(m_caps.caps_qam);
  transponder.SetDeliverySystem(SYS_DVBT);
  transponder.TerrestrialParams().SetTransmission(m_caps.caps_transmission_mode);
  transponder.TerrestrialParams().SetGuard(m_caps.caps_guard_interval);
  transponder.TerrestrialParams().SetHierarchy(m_caps.caps_hierarchy);

  channel->SetTransponder(transponder);

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

  cTransponder transponder(TRANSPONDER_CABLE);
  transponder.SetFrequencyMHz(410); // Find a dvb-c capable device using *some* channel
  transponder.CableParams().SetSymbolRateHz(6900); // TODO: Should this be 6900 * 1000???
  transponder.SetInversion(INVERSION_OFF);
  transponder.CableParams().SetCoderateH(FEC_NONE);
  transponder.SetModulation(this_qam); // Only loop-dependent variable (loop-independent if caps_qam == QAM_AUTO)
  transponder.SetDeliverySystem(SYS_DVBC_ANNEX_A);

  channel->SetTransponder(transponder);

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
    const cTransponder& transponder = channel.GetTransponder();
    cDiseqc *diseqc = GetDiseqc(transponder.Type(), transponder.FrequencyHz(), transponder.SatelliteParams().Polarization());

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

cDiseqc* cScanTaskSatellite::GetDiseqc(TRANSPONDER_TYPE source, unsigned frequency, fe_polarization_t polarization)
{
  for (cDiseqc *p = Diseqcs.First(); p; p = Diseqcs.Next(p))
  {
    if (p->Source() == source && p->Slof() > frequency && p->Polarization() == polarization)
      return p;
  }

  return NULL;
}

ChannelPtr cScanTaskSatellite::GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset)
{
  ChannelPtr channel = ChannelPtr(new cChannel);

  cTransponder transponder(TRANSPONDER_SATELLITE);
  transponder.SetFrequencyHz(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).intermediate_frequency);
  transponder.SatelliteParams().SetSymbolRateHz(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).symbol_rate); // TODO: Should this be symbol rate * 1000???
  transponder.SatelliteParams().SetPolarization(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).polarization);
  transponder.SetInversion(INVERSION_OFF);
  transponder.SatelliteParams().SetCoderateH(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).fec_inner);
  transponder.SetModulation(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).modulation_type);
  transponder.SetDeliverySystem(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).modulation_system == SYS_DVBS ? SYS_DVBS : SYS_DVBS2);
  transponder.SatelliteParams().SetRollOff(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).rolloff);

  // TODO: Something with SatelliteUtils::GetSatellite(m_satelliteId).source_id

  channel->SetTransponder(transponder);

  if (!ValidSatFrequency(SatelliteUtils::GetTransponder(m_satelliteId, iChannel).intermediate_frequency, *channel))
    return cChannel::EmptyChannel;

  channel->SetId(0, 0, 0);

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

  cTransponder transponder(TRANSPONDER_ATSC);
  transponder.SetFrequencyHz(frequency);
  transponder.SetInversion(m_caps.caps_inversion);
  transponder.SetModulation(modulation); // Only loop-dependent variable

  ChannelPtr channel = ChannelPtr(new cChannel);
  channel->SetTransponder(transponder);

  return channel;
}

}
