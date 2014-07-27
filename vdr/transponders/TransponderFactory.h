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
#pragma once

#include "Transponder.h"
#include "TransponderTypes.h"
#include "scan/CountryUtils.h"
#include "scan/SatelliteUtils.h"

#include <vector>

namespace VDR
{

enum ATSC_MODULATION
{
  ATSC_MODULATION_VSB_8,
  ATSC_MODULATION_QAM_256,
  ATSC_MODULATION_BOTH
};

enum eDvbcSymbolRate
{
  eSR_AUTO,
  eSR_ALL,
  eSR_BEGIN,
  eSR_6900000 = eSR_BEGIN,
  eSR_6875000,
  eSR_6111000,
  eSR_6250000,
  eSR_6790000,
  eSR_6811000,
  eSR_5900000,
  eSR_5000000,
  eSR_3450000,
  eSR_4000000,
  eSR_6950000,
  eSR_7000000,
  eSR_6952000,
  eSR_5156000,
  eSR_5483000,
  eSR_END = eSR_5483000
};

class cTransponderFactory
{
public:
  cTransponderFactory(fe_caps_t caps);
  virtual ~cTransponderFactory(void) { }

  virtual bool HasNext(void) const = 0;
  virtual cTransponder GetNext(void) = 0;

protected:
  bool m_bCanCodeRateAuto;
  bool m_bCanGuardIntervalAuto;
  bool m_bCanHierarchyAuto;
  bool m_bCanInversionAuto;
  bool m_bCanQamAuto;
  bool m_bCanS2;
  bool m_bCanTransmissionAuto;
};

class cAtscTransponderFactory : public cTransponderFactory
{
public:
  cAtscTransponderFactory(fe_caps_t caps, ATSC_MODULATION atscModuation);
  virtual ~cAtscTransponderFactory(void) { }

  virtual bool HasNext(void) const { return m_paramIt != m_params.end(); }
  virtual cTransponder GetNext(void);

private:
  struct AtscVariables
  {
    fe_modulation_t modulation;
    unsigned int    channel;
    eOffsetType     frequencyOffset;
  };

  bool IsValidTransponder(const AtscVariables& vars);

  std::vector<AtscVariables>                 m_params;
  std::vector<AtscVariables>::const_iterator m_paramIt;
};

class cCableTransponderFactory : public cTransponderFactory
{
public:
  cCableTransponderFactory(fe_caps_t caps, eDvbcSymbolRate dvbcSymbolRate);
  virtual ~cCableTransponderFactory(void) { }

  virtual bool HasNext(void) const { return m_paramIt != m_params.end(); }
  virtual cTransponder GetNext(void);

private:
  struct CableVariables
  {
    fe_modulation_t modulation;
    unsigned int    channel;
    eDvbcSymbolRate dvbcSymbolRate;
    eOffsetType     frequencyOffset;
  };

  bool IsValidTransponder(const CableVariables& vars);

  std::vector<CableVariables>                 m_params;
  std::vector<CableVariables>::const_iterator m_paramIt;
};

class cSatelliteTransponderFactory : public cTransponderFactory
{
public:
  cSatelliteTransponderFactory(fe_caps_t caps, SATELLITE::eSatellite satelliteIndex);
  virtual ~cSatelliteTransponderFactory(void) { }

  virtual bool HasNext(void) const { return m_paramIt != m_params.end(); }
  virtual cTransponder GetNext(void);

private:
  struct SatelliteVariables
  {
    unsigned int          channel;
    SATELLITE::eSatellite satelliteIndex;
  };

  bool IsValidTransponder(const SatelliteVariables& vars);

  std::vector<SatelliteVariables>                 m_params;
  std::vector<SatelliteVariables>::const_iterator m_paramIt;
};

class cTerrestrialTransponderFactory : public cTransponderFactory
{
public:
  cTerrestrialTransponderFactory(fe_caps_t caps);
  virtual ~cTerrestrialTransponderFactory(void) { }

  virtual bool HasNext(void) const { return m_paramIt != m_params.end(); }
  virtual cTransponder GetNext(void);

private:
  struct TerrestrialVariables
  {
    unsigned int channel;
    eOffsetType  frequencyOffset;
  };

  bool IsValidTransponder(const TerrestrialVariables& vars);

  std::vector<TerrestrialVariables>                 m_params;
  std::vector<TerrestrialVariables>::const_iterator m_paramIt;
};

}
