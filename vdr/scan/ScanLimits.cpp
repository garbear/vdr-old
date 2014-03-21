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

#include "SatelliteUtils.h"
#include "ScanTask.h"

using namespace PLATFORM;
using namespace VDR::SATELLITE;
using namespace std;

namespace VDR
{

#define DEFAULT_CHANNEL_COUNT  134 // Scan 134 channels by default

cScanLimits::cScanLimits()
 : m_channelCount(DEFAULT_CHANNEL_COUNT)
{
}

void cScanLimits::ForEach(cScanTask* task)
{
  for (vector<fe_modulation>::const_iterator modIt = m_modulations.begin(); modIt != m_modulations.end() && !IsAborting(); ++modIt)
    for (unsigned int iChannel = 0; iChannel < m_channelCount && !IsAborting(); ++iChannel)
      for (vector<eOffsetType>::const_iterator freqIt = m_freqOffsets.begin(); freqIt != m_freqOffsets.end() && !IsAborting(); ++freqIt)
        for (vector<eDvbcSymbolRate>::const_iterator srIt = m_dvbcSymbolRates.begin(); srIt != m_dvbcSymbolRates.end() && !IsAborting(); ++srIt)
          task->DoWork(*modIt, iChannel, *srIt, *freqIt, this);

  Finished();
}

cScanLimitsTerrestrial::cScanLimitsTerrestrial()
{
  // Disable QAM loop
  m_modulations.push_back(QAM_64);

  // Disable symbol rate loop
  m_dvbcSymbolRates.push_back(eSR_6900000);

  m_freqOffsets.push_back(NO_OFFSET);
  m_freqOffsets.push_back(POS_OFFSET);
  m_freqOffsets.push_back(NEG_OFFSET);
}

cScanLimitsCable::cScanLimitsCable(eDvbcSymbolRate symbolRate)
{
  m_modulations.push_back(QAM_64);
  m_modulations.push_back(QAM_256);

  switch (symbolRate)
  {
    case eSR_AUTO:
      m_dvbcSymbolRates.push_back(eSR_6900000);
      m_dvbcSymbolRates.push_back(eSR_6875000);
      break;
    case eSR_ALL:
      for (int sr = eSR_BEGIN; sr <= eSR_END; sr++)
        m_dvbcSymbolRates.push_back((eDvbcSymbolRate)sr);
      break;
    default:
      m_dvbcSymbolRates.push_back(symbolRate);
      break;
    }

  m_freqOffsets.push_back(NO_OFFSET);
  m_freqOffsets.push_back(POS_OFFSET);
  m_freqOffsets.push_back(NEG_OFFSET);
}

cScanLimitsSatellite::cScanLimitsSatellite(eSatellite satelliteId)
{
  // Disable QAM loop
  m_modulations.push_back(QAM_64);

  m_channelCount = SatelliteUtils::GetTransponderCount(satelliteId);

  // Disable symbol rate loop
  m_dvbcSymbolRates.push_back(eSR_6900000);

  // Disable frequency offset loop
  m_freqOffsets.push_back(NO_OFFSET);
}

cScanLimitsATSC::cScanLimitsATSC(fe_modulation atscModulation)
{
  m_modulations.push_back(atscModulation);

  // Disable symbol rate loop
  m_dvbcSymbolRates.push_back(eSR_AUTO);

  m_freqOffsets.push_back(NO_OFFSET);
  m_freqOffsets.push_back(POS_OFFSET);
  m_freqOffsets.push_back(NEG_OFFSET);
}

}
