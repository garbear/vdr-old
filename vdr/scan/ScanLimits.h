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
#pragma once

#include "CountryUtils.h"
#include "Scanner.h"
#include "utils/SynchronousAbort.h"

#include <linux/dvb/frontend.h>
#include <vector>

namespace VDR
{

class cScanConfig;
class cScanTask;

class cScanLimits : public cSynchronousAbort
{
public:
  virtual ~cScanLimits() { }

  void ForEach(cScanTask* task, const cScanConfig& config, iScanCallback* callback);

protected:
  cScanLimits();

  std::vector<fe_modulation>   m_modulations;
  unsigned int                 m_channelCount;    // Number of channels to scan
  std::vector<eDvbcSymbolRate> m_dvbcSymbolRates; // Must not contain eSR_AUTO or eSR_ALL
  std::vector<eOffsetType>     m_freqOffsets;
};

class cScanLimitsTerrestrial : public cScanLimits
{
public:
  cScanLimitsTerrestrial();
};

class cScanLimitsCable : public cScanLimits
{
public:
  cScanLimitsCable(eDvbcSymbolRate symbolRate);
};

class cScanLimitsSatellite : public cScanLimits
{
public:
  cScanLimitsSatellite(SATELLITE::eSatellite satelliteId);
};

class cScanLimitsATSC : public cScanLimits
{
public:
  // atscModulation: either VSB_8 (VSB over-the-air) or QAM_256 (QAM Annex B cable TV)
  cScanLimitsATSC(fe_modulation_t atscModulation);
};

}
