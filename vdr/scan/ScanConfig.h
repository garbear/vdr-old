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
#include "SatelliteUtils.h"
#include "channels/ChannelTypes.h"
#include "devices/DeviceTypes.h"
#include "transponders/TransponderTypes.h"
#include "transponders/TransponderFactory.h" // TODO

#include <linux/dvb/frontend.h>

namespace VDR
{

/*
class iScanCallback
{
public:
  virtual ~iScanCallback() { }

  virtual void ScanPercentage(float percentage) { }
  virtual void ScanFrequency(unsigned int frequencyHz) { }
  virtual void ScanSignalStrength(int strength, bool bLocked) { }
  virtual void ScanDeviceInfo(const std::string& strInfo) { }
  virtual void ScanTransponder(const std::string strInfo) { }
  virtual void ScanFoundChannel(ChannelPtr channel) { }
  virtual void ScanFinished(bool bAborted) { }
  virtual void ScanStatus(int status) { }
};
*/

class cScanConfig
{
public:
  cScanConfig();

  /*!
   * \brief Translate one of eDVBCSymbolRate's numeric enums into an integer value
   * \param sr Will assert if sr is eSR_AUTO or eSR_ALL
   * \return The integer value (e.g. 6900000 for eSR_6900000)
   */
  static unsigned int TranslateSymbolRate(eDvbcSymbolRate sr);
  static eDvbcSymbolRate TranslateSymbolRate(unsigned int sr);

  TRANSPONDER_TYPE        dvbType;
  eDvbcSymbolRate         dvbcSymbolRate;
  SATELLITE::eSatellite   satelliteIndex;
  ATSC_MODULATION         atscModulation; // Either VSB over-the-air (VSB_8) or QAM Annex B cable TV (QAM_256)
  DevicePtr               device;
};

}
