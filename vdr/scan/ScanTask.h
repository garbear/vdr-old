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

#include "channels/ChannelTypes.h"
#include "CountryUtils.h"
#include "FrontendCapabilities.h"
#include "ScanLimits.h"
#include "ScanConfig.h"
#include "dvb/filters/EIT.h"
#include "dvb/filters/NIT.h"
#include "dvb/filters/PAT.h"
#include "dvb/filters/PMT.h"
#include "dvb/filters/SDT.h"
#include "dvb/filters/TDT.h"

#include <linux/dvb/frontend.h>
#include <vector>

namespace VDR
{

class cDiseqc;
class cDevice;
class cSynchronousAbort;

class cScanTask
{
public:
  cScanTask(cDevice* device, const cFrontendCapabilities& caps);
  virtual ~cScanTask() { }

  void DoWork(const ChannelPtr& channel, cSynchronousAbort* abortableJob = NULL, iScanCallback* callback = NULL);

  static unsigned int ChannelToFrequency(unsigned int channel, eChannelList channelList);

  /*!
   * \brief Create a channel object with the given parameters. Must be implemented
   *        by subclasses.
   */
  virtual ChannelPtr GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset) = 0;

protected:
  cFrontendCapabilities m_caps;

private:
  cDevice*           m_device;
  cSynchronousAbort* m_abortableJob;
  //std::vector<cPmtScanner*> m_pmtScanners;
};

class cScanTaskTerrestrial : public cScanTask
{
public:
  cScanTaskTerrestrial(cDevice* device, const cFrontendCapabilities& caps, eChannelList channelList);
  virtual ~cScanTaskTerrestrial() { }

protected:
  virtual ChannelPtr GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset);

private:
  eChannelList m_channelList;
};

class cScanTaskCable : public cScanTask
{
public:
  cScanTaskCable(cDevice* device, const cFrontendCapabilities& caps, eChannelList channelList);
  virtual ~cScanTaskCable() { }

protected:
  virtual ChannelPtr GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset);

private:
  eChannelList m_channelList;
};

class cScanTaskSatellite : public cScanTask
{
public:
  cScanTaskSatellite(cDevice* device, const cFrontendCapabilities& caps, SATELLITE::eSatellite satelliteId);
  virtual ~cScanTaskSatellite() { }

protected:
  virtual ChannelPtr GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset);

private:
  static bool ValidSatFrequency(unsigned int frequencyHz, const cChannel& channel);
  static cDiseqc* GetDiseqc(cChannelSource source, unsigned int frequency, fe_polarization_t polarization);

  SATELLITE::eSatellite m_satelliteId;
};

class cScanTaskATSC : public cScanTask
{
public:
  cScanTaskATSC(cDevice* device, const cFrontendCapabilities& caps, eChannelList channelList);
  virtual ~cScanTaskATSC() { }

protected:
  ChannelPtr GetChannel(fe_modulation modulation, unsigned int iChannel, eDvbcSymbolRate symbolRate, eOffsetType freqOffset);

private:
  eChannelList m_channelList;
};

}
