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

#include "TransponderFactory.h"
#include "dvb/DiSEqC.h"
#include "scan/ScanConfig.h"
#include "settings/Settings.h"

#include <assert.h>

using namespace std;

#define MAX_CHANNEL_NUMBER  134 // Scan to channel 134

#define CABLE_INVERSION_FALLBACK  INVERSION_OFF // Try OFF if AUTO is not supported (TODO)
#define TERR_INVERSION_FALLBACK   INVERSION_OFF // Try OFF if AUTO is not supported (TODO)

namespace VDR
{

/*
 * Helper method - generate a vector of channels 0 through 134 (inclusive)
 */
const std::vector<unsigned int>& GetChannelNumbers(void)
{
  static vector<unsigned int> channelNumbers;

  if (channelNumbers.empty())
  {
    for (unsigned int i = 0; i <= MAX_CHANNEL_NUMBER; i++)
      channelNumbers.push_back(i);

    // Check for off-by-1 error
    assert(channelNumbers.size() == MAX_CHANNEL_NUMBER + 1);
  }

  return channelNumbers;
}

cTransponderFactory::cTransponderFactory(fe_caps_t caps)
 : m_bCanCodeRateAuto     (caps & FE_CAN_FEC_AUTO),
   m_bCanGuardIntervalAuto(caps & FE_CAN_GUARD_INTERVAL_AUTO),
   m_bCanHierarchyAuto    (caps & FE_CAN_HIERARCHY_AUTO),
   m_bCanInversionAuto    (caps & FE_CAN_INVERSION_AUTO),
   m_bCanQamAuto          (caps & FE_CAN_QAM_AUTO),
   m_bCanS2               (caps & FE_CAN_2G_MODULATION),
   m_bCanTransmissionAuto (caps & FE_CAN_TRANSMISSION_MODE_AUTO)
{
}

cAtscTransponderFactory::cAtscTransponderFactory(fe_caps_t caps, ATSC_MODULATION atscModulation)
 : cTransponderFactory(caps)
{
  vector<fe_modulation_t> modulations;
  switch (atscModulation)
  {
  case ATSC_MODULATION_VSB_8:
    modulations.push_back(VSB_8);
    break;
  case ATSC_MODULATION_QAM_256:
    modulations.push_back(QAM_256);
    break;
  default: // ATSC_MODULATION_BOTH
    modulations.push_back(VSB_8);
    modulations.push_back(QAM_256);
    break;
  }

  const vector<unsigned int>& channels = GetChannelNumbers();

  vector<eOffsetType> frequencyOffsets;
  frequencyOffsets.push_back(NEG_OFFSET);
  frequencyOffsets.push_back(NO_OFFSET);
  frequencyOffsets.push_back(POS_OFFSET);

  for (vector<fe_modulation_t>::const_iterator itMod = modulations.begin(); itMod != modulations.end(); ++itMod)
  {
    for (vector<unsigned int>::const_iterator itCh = channels.begin(); itCh != channels.end(); ++itCh)
    {
      for (vector<eOffsetType>::const_iterator itFreqOff = frequencyOffsets.begin(); itFreqOff != frequencyOffsets.end(); ++itFreqOff)
      {
        AtscVariables vars = { *itMod, *itCh, *itFreqOff };
        if (IsValidTransponder(vars))
          m_params.push_back(vars);
      }
    }
  }

  m_paramIt = m_params.begin();
}

bool cAtscTransponderFactory::IsValidTransponder(const AtscVariables& vars)
{
  int iFrequencyOffset; // unused

  eChannelList channelList = CountryUtils::GetChannelList(vars.modulation);

  // TODO: Refactor this check into CountryUtils
  if (CountryUtils::HasChannelList(vars.modulation) &&
      CountryUtils::HasBaseOffset(channelList, vars.channel)    &&
      CountryUtils::HasFrequencyStep(channelList, vars.channel) &&
      CountryUtils::HasFrequencyOffset(channelList, vars.channel, vars.frequencyOffset))
  {
    return true;
  }

  return false;
}

cTransponder cAtscTransponderFactory::GetNext(void)
{
  cTransponder transponder;

  if (HasNext())
  {
    const AtscVariables& vars = *m_paramIt++;

    assert(IsValidTransponder(vars));

    // TODO: Refactor this calculation into CountryUtils
    eChannelList channelList   = CountryUtils::GetChannelList(vars.modulation);
    int          offset        = CountryUtils::GetBaseOffset(channelList, vars.channel);
    unsigned int frequencyStep = CountryUtils::GetFrequencyStep(channelList, vars.channel);
    int iFrequencyOffset;
    CountryUtils::GetFrequencyOffset(channelList, vars.channel, vars.frequencyOffset, iFrequencyOffset);

    const unsigned int frequencyHz = offset + vars.channel * frequencyStep + iFrequencyOffset;

    transponder.Reset(TRANSPONDER_ATSC);
    transponder.SetChannelNumber(vars.channel);
    transponder.SetFrequencyHz(frequencyHz);
    transponder.SetInversion(m_bCanInversionAuto ? INVERSION_AUTO : INVERSION_OFF);
    transponder.SetModulation(vars.modulation);
  }

  return transponder;
}

cCableTransponderFactory::cCableTransponderFactory(fe_caps_t caps, eDvbcSymbolRate dvbcSymbolRate)
 : cTransponderFactory(caps)
{
  vector<fe_modulation_t> modulations;
  if (m_bCanQamAuto)
  {
    modulations.push_back(QAM_AUTO);
  }
  else
  {
    modulations.push_back(QAM_64);
    modulations.push_back(QAM_256);
  }

  const vector<unsigned int>& channels = GetChannelNumbers();

  vector<eDvbcSymbolRate> dvbcSymbolRates;
  switch (dvbcSymbolRate)
  {
  case eSR_AUTO:
    dvbcSymbolRates.push_back(eSR_6900000);
    dvbcSymbolRates.push_back(eSR_6875000);
    break;
  case eSR_ALL:
    for (int sr = eSR_BEGIN; sr <= eSR_END; sr++)
      dvbcSymbolRates.push_back((eDvbcSymbolRate)sr);
    break;
  default:
    dvbcSymbolRates.push_back(dvbcSymbolRate);
    break;
  }

  vector<eOffsetType> frequencyOffsets;
  frequencyOffsets.push_back(NEG_OFFSET);
  frequencyOffsets.push_back(NO_OFFSET);
  frequencyOffsets.push_back(POS_OFFSET);

  for (vector<fe_modulation_t>::const_iterator itMod = modulations.begin(); itMod != modulations.end(); ++itMod)
  {
    for (vector<unsigned int>::const_iterator itCh = channels.begin(); itCh != channels.end(); ++itCh)
    {
      for (vector<eDvbcSymbolRate>::const_iterator itSymRate = dvbcSymbolRates.begin(); itSymRate != dvbcSymbolRates.end(); ++itSymRate)
      {
        for (vector<eOffsetType>::const_iterator itFreqOff = frequencyOffsets.begin(); itFreqOff != frequencyOffsets.end(); ++itFreqOff)
      {
          CableVariables vars = { *itMod, *itCh, *itSymRate, *itFreqOff };
          if (IsValidTransponder(vars))
            m_params.push_back(vars);
        }
      }
    }
  }

  m_paramIt = m_params.begin();
}

bool cCableTransponderFactory::IsValidTransponder(const CableVariables& vars)
{
  int iFrequencyOffset; // unused

  // TODO: Choose country
  vector<COUNTRY::eCountry> countries = CountryUtils::GetCountries(TRANSPONDER_CABLE);
  if (countries.empty())
    return false;
  COUNTRY::eCountry country = countries[0];

  eChannelList channelList = CountryUtils::GetChannelList(vars.modulation);

  if (CountryUtils::HasChannelList(TRANSPONDER_CABLE, country) &&
      CountryUtils::HasBaseOffset(channelList, vars.channel)    &&
      CountryUtils::HasFrequencyStep(channelList, vars.channel) &&
      CountryUtils::HasFrequencyOffset(channelList, vars.channel, vars.frequencyOffset))
  {
    return true;
  }

  return false;
}

cTransponder cCableTransponderFactory::GetNext(void)
{
  cTransponder transponder;

  if (HasNext())
  {
    const CableVariables& vars = *m_paramIt++;

    assert(IsValidTransponder(vars));

    // TODO: Refactor this calculation into CountryUtils
    eChannelList channelList   = CountryUtils::GetChannelList(TRANSPONDER_CABLE, CountryUtils::GetCountries(TRANSPONDER_CABLE)[0]); // TODO: country check
    int          offset        = CountryUtils::GetBaseOffset(channelList, vars.channel);
    unsigned int frequencyStep = CountryUtils::GetFrequencyStep(channelList, vars.channel);
    int iFrequencyOffset;
    CountryUtils::GetFrequencyOffset(channelList, vars.channel, vars.frequencyOffset, iFrequencyOffset);

    const unsigned int frequencyHz = offset + vars.channel * frequencyStep + iFrequencyOffset;

    transponder.Reset(TRANSPONDER_CABLE);
    transponder.SetChannelNumber(vars.channel);
    transponder.SetDeliverySystem(SYS_DVBC_ANNEX_AC);
    transponder.SetFrequencyHz(frequencyHz);
    transponder.SetInversion(m_bCanInversionAuto ? INVERSION_AUTO : CABLE_INVERSION_FALLBACK);
    transponder.SetModulation(vars.modulation);
    transponder.CableParams().SetCoderateH(FEC_NONE);
    transponder.CableParams().SetSymbolRateHz(vars.dvbcSymbolRate);
  }

  return transponder;
}

cSatelliteTransponderFactory::cSatelliteTransponderFactory(fe_caps_t caps, SATELLITE::eSatellite satelliteIndex)
 : cTransponderFactory(caps)
{
  const vector<unsigned int>& channels = GetChannelNumbers();

  for (vector<unsigned int>::const_iterator itCh = channels.begin(); itCh != channels.end(); ++itCh)
  {
    SatelliteVariables vars = { *itCh, satelliteIndex };
    if (IsValidTransponder(vars))
      m_params.push_back(vars);
  }

  m_paramIt = m_params.begin();
}

bool cSatelliteTransponderFactory::IsValidTransponder(const SatelliteVariables& vars)
{
  // Must check this first, or GetSatellite() will assert (TODO: Fix this)
  if (!SatelliteUtils::HasTransponder(vars.satelliteIndex, vars.channel))
    return false;

  // TODO: Don't expose __sat_transponder! Refactor into SatelliteUtils
  const __sat_transponder& satTransponder = SatelliteUtils::GetTransponder(vars.satelliteIndex, vars.channel);

  if (SatelliteUtils::HasTransponder(vars.satelliteIndex, vars.channel) &&
      SatelliteUtils::IsValidFrequency(satTransponder.intermediate_frequency, satTransponder.polarization) &&
      (m_bCanS2 || satTransponder.modulation_system != SYS_DVBS2)) // Check for driver support
  {
    return true;
  }

  return false;
}

cTransponder cSatelliteTransponderFactory::GetNext(void)
{
  cTransponder transponder;

  if (HasNext())
  {
    const SatelliteVariables& vars = *m_paramIt++;

    // TODO: Don't expose __sat_transponder! Refactor into SatelliteUtils
    const __sat_transponder& satTransponder = SatelliteUtils::GetTransponder(vars.satelliteIndex, vars.channel);

    transponder.Reset(TRANSPONDER_SATELLITE);
    transponder.SetChannelNumber(vars.channel);
    transponder.SetDeliverySystem(satTransponder.modulation_system);
    transponder.SetFrequencyHz(satTransponder.intermediate_frequency);
    transponder.SetInversion(INVERSION_OFF);
    transponder.SetModulation(satTransponder.modulation_type);
    transponder.SatelliteParams().SetCoderateH(satTransponder.fec_inner);
    transponder.SatelliteParams().SetPolarization(satTransponder.polarization);
    transponder.SatelliteParams().SetRollOff(satTransponder.rolloff);
    transponder.SatelliteParams().SetSymbolRateHz(satTransponder.symbol_rate);

    // TODO: Obtain satellite position
    //cChannelSource source;
    //source.Deserialise(SatelliteUtils::GetSatellite(m_satelliteIndex).source_id);
  }

  return transponder;
}

cTerrestrialTransponderFactory::cTerrestrialTransponderFactory(fe_caps_t caps)
 : cTransponderFactory(caps)
{
  const vector<unsigned int>& channels = GetChannelNumbers();

  vector<eOffsetType> frequencyOffsets;
  frequencyOffsets.push_back(NEG_OFFSET);
  frequencyOffsets.push_back(NO_OFFSET);
  frequencyOffsets.push_back(POS_OFFSET);

  for (vector<unsigned int>::const_iterator itCh = channels.begin(); itCh != channels.end(); ++itCh)
  {
    for (vector<eOffsetType>::const_iterator itFreqOff = frequencyOffsets.begin(); itFreqOff != frequencyOffsets.end(); ++itFreqOff)
    {
      TerrestrialVariables vars = { *itCh, *itFreqOff };
      if (IsValidTransponder(vars))
        m_params.push_back(vars);
    }
  }

  m_paramIt = m_params.begin();
}

bool cTerrestrialTransponderFactory::IsValidTransponder(const TerrestrialVariables& vars)
{
  // TODO: Choose country
  vector<COUNTRY::eCountry> countries = CountryUtils::GetCountries(TRANSPONDER_TERRESTRIAL);
  if (countries.empty())
    return false;
  COUNTRY::eCountry country = countries[0];

  eChannelList channelList = CountryUtils::GetChannelList(TRANSPONDER_TERRESTRIAL, country);

  if (CountryUtils::HasChannelList(TRANSPONDER_TERRESTRIAL, country) &&
      CountryUtils::HasBaseOffset(channelList, vars.channel)    &&
      CountryUtils::HasFrequencyStep(channelList, vars.channel) &&
      CountryUtils::HasFrequencyOffset(channelList, vars.channel, vars.frequencyOffset) &&
      CountryUtils::HasBandwidth(vars.channel, channelList))
  {
    return true;
  }

  return false;
}

cTransponder cTerrestrialTransponderFactory::GetNext(void)
{
  cTransponder transponder;

  if (HasNext())
  {
    const TerrestrialVariables& vars = *m_paramIt++;

    assert(IsValidTransponder(vars));

    // TODO: Refactor this calculation into CountryUtils
    eChannelList channelList   = CountryUtils::GetChannelList(TRANSPONDER_TERRESTRIAL, CountryUtils::GetCountries(TRANSPONDER_TERRESTRIAL)[0]); // TODO: country check
    int          offset        = CountryUtils::GetBaseOffset(channelList, vars.channel);
    unsigned int frequencyStep = CountryUtils::GetFrequencyStep(channelList, vars.channel);
    int iFrequencyOffset;
    CountryUtils::GetFrequencyOffset(channelList, vars.channel, vars.frequencyOffset, iFrequencyOffset);
    fe_bandwidth_t bandwidth;
    CountryUtils::GetBandwidth(vars.channel, channelList, bandwidth);

    const unsigned int frequencyHz = offset + vars.channel * frequencyStep + iFrequencyOffset;

    transponder.Reset(TRANSPONDER_TERRESTRIAL);
    transponder.SetChannelNumber(vars.channel);
    transponder.SetDeliverySystem(SYS_DVBT);
    transponder.SetFrequencyHz(frequencyHz);
    transponder.SetInversion(m_bCanInversionAuto ? INVERSION_AUTO : TERR_INVERSION_FALLBACK);
    transponder.SetModulation(m_bCanQamAuto ? QAM_AUTO : QAM_64);
    transponder.TerrestrialParams().SetBandwidth(bandwidth);
    transponder.TerrestrialParams().SetCoderateH(m_bCanCodeRateAuto ? FEC_AUTO : FEC_NONE); // "FEC_AUTO not supported, trying FEC_NONE"
    transponder.TerrestrialParams().SetCoderateL(m_bCanCodeRateAuto ? FEC_AUTO : FEC_NONE);
    transponder.TerrestrialParams().SetGuard(m_bCanGuardIntervalAuto? GUARD_INTERVAL_AUTO : GUARD_INTERVAL_1_8); // "GUARD_INTERVAL_AUTO not supported, trying GUARD_INTERVAL_1_8"
    transponder.TerrestrialParams().SetHierarchy(m_bCanHierarchyAuto ? HIERARCHY_AUTO : HIERARCHY_NONE); // "HIERARCHY_AUTO not supported, trying HIERARCHY_NONE"
    transponder.TerrestrialParams().SetTransmission(m_bCanTransmissionAuto ? TRANSMISSION_MODE_AUTO : TRANSMISSION_MODE_8K); // GB seems to use 8k since 12/2009
  }

  return transponder;
}

}
