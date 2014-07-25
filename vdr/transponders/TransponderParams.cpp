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

#include "TransponderParams.h"
#include "Stringifier.h"
#include "TransponderDefinitions.h"
#include "utils/StringUtils.h"

#include <tinyxml.h>

namespace VDR
{

/****************
   cCableParams
 ****************/

cCableParams::cCableParams(void)
 : m_coderateH(FEC_AUTO),
   m_symbolRateHz(0)
{
}

bool cCableParams::operator==(const cCableParams& rhs) const
{
  return m_coderateH    == rhs.m_coderateH    &&
         m_symbolRateHz == rhs.m_symbolRateHz;
}

void cCableParams::Serialise(TiXmlElement* elem) const
{
  if (m_coderateH != FEC_AUTO)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_CODERATEH, Stringifier::CoderateToString(m_coderateH));

  if (m_symbolRateHz != 0)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_SYMBOL_RATE, m_symbolRateHz);
}

void cCableParams::Deserialise(const TiXmlElement* elem)
{
  m_coderateH = Stringifier::StringToCoderate(elem->Attribute(TRANSPONDER_XML_ATTR_CODERATEH));

  const char* strAttr = elem->Attribute(TRANSPONDER_XML_ATTR_SYMBOL_RATE);
  m_symbolRateHz = strAttr ? StringUtils::IntVal(strAttr) : 0;
}

/********************
   cSatelliteParams
 ********************/

cSatelliteParams::cSatelliteParams(void)
 : m_coderateH(FEC_AUTO),
   m_polarization(POLARIZATION_HORIZONTAL),
   m_rollOff(ROLLOFF_AUTO),
   m_streamId(0),
   m_symbolRateHz(0)
{
}

bool cSatelliteParams::operator==(const cSatelliteParams& rhs) const
{
  return m_coderateH    == rhs.m_coderateH    &&
         m_polarization == rhs.m_polarization &&
         m_rollOff      == rhs.m_rollOff      &&
         m_streamId     == rhs.m_streamId     &&
         m_symbolRateHz == rhs.m_symbolRateHz;
}

void cSatelliteParams::Serialise(TiXmlElement* elem) const
{
  if (m_coderateH != FEC_AUTO)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_CODERATEH, Stringifier::CoderateToString(m_coderateH));

  // Polarization has no "auto" default, so always serialize this param
  elem->SetAttribute(TRANSPONDER_XML_ATTR_POLARIZATION, Stringifier::PolarizationToString(m_polarization));

  if (m_rollOff != ROLLOFF_AUTO)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_ROLLOFF, Stringifier::RollOffToString(m_rollOff));

  if (m_streamId != 0)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_STREAMID, m_streamId);

  if (m_symbolRateHz != 0)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_SYMBOL_RATE, m_symbolRateHz);
}

void cSatelliteParams::Deserialise(const TiXmlElement* elem)
{
  m_coderateH = Stringifier::StringToCoderate(elem->Attribute(TRANSPONDER_XML_ATTR_CODERATEH));

  m_polarization = Stringifier::StringToPolarization(elem->Attribute(TRANSPONDER_XML_ATTR_POLARIZATION));

  m_rollOff = Stringifier::StringToRollOff(elem->Attribute(TRANSPONDER_XML_ATTR_ROLLOFF));

  const char* strStreamId = elem->Attribute(TRANSPONDER_XML_ATTR_STREAMID);
  m_streamId = strStreamId ? StringUtils::IntVal(strStreamId) : 0;

  const char* strSymbolRate = elem->Attribute(TRANSPONDER_XML_ATTR_SYMBOL_RATE);
  m_symbolRateHz = strSymbolRate ? StringUtils::IntVal(strSymbolRate) : 0;
}

/********************
   cTerrestrialParams
 ********************/

cTerrestrialParams::cTerrestrialParams(void)
 : m_bandwidth(BANDWIDTH_8_MHZ), // TODO: Maybe BANDWIDTH_AUTO?
   m_coderateH(FEC_AUTO),
   m_coderateL(FEC_AUTO),
   m_guard(GUARD_INTERVAL_AUTO),
   m_hierarchy(HIERARCHY_AUTO),
   m_streamId(0),
   m_transmission(TRANSMISSION_MODE_AUTO)
{
}

bool cTerrestrialParams::operator==(const cTerrestrialParams& rhs) const
{
  return m_bandwidth    == m_bandwidth        &&
         m_coderateH    == rhs.m_coderateH    &&
         m_coderateL    == rhs.m_coderateL    &&
         m_guard        == rhs.m_guard        &&
         m_hierarchy    == rhs.m_hierarchy    &&
         m_streamId     == rhs.m_streamId     &&
         m_transmission == rhs.m_transmission;
}

unsigned int cTerrestrialParams::BandwidthHz(void) const
{
  switch (m_bandwidth)
  {
  case BANDWIDTH_8_MHZ:     return  8 * 1000 * 1000;
  case BANDWIDTH_7_MHZ:     return  7 * 1000 * 1000;
  case BANDWIDTH_6_MHZ:     return  6 * 1000 * 1000;
  case BANDWIDTH_AUTO:      return                0;
  case BANDWIDTH_5_MHZ:     return  5 * 1000 * 1000;
  case BANDWIDTH_10_MHZ:    return 10 * 1000 * 1000;
  case BANDWIDTH_1_712_MHZ: return      1712 * 1000;
  default:                  return                0;
  }
}

void cTerrestrialParams::Serialise(TiXmlElement* elem) const
{
  elem->SetAttribute(TRANSPONDER_XML_ATTR_BANDWIDTH, Stringifier::BandwidthToString(m_bandwidth));

  if (m_coderateH != FEC_AUTO)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_CODERATEH, Stringifier::CoderateToString(m_coderateH));

  if (m_coderateL != FEC_AUTO)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_CODERATEL, Stringifier::CoderateToString(m_coderateL));

  if (m_guard != GUARD_INTERVAL_AUTO)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_GUARD, Stringifier::GuardToString(m_guard));

  if (m_hierarchy != HIERARCHY_AUTO)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_HIERARCHY, Stringifier::HierarchyToString(m_hierarchy));

  if (m_streamId != 0)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_STREAMID, m_streamId);

  if (m_transmission != TRANSMISSION_MODE_AUTO)
    elem->SetAttribute(TRANSPONDER_XML_ATTR_TRANSMISSION, Stringifier::TransmissionToString(m_transmission));
}

void cTerrestrialParams::Deserialise(const TiXmlElement* elem)
{
  m_bandwidth = Stringifier::StringToBandwidth(elem->Attribute(TRANSPONDER_XML_ATTR_BANDWIDTH));

  m_coderateH = Stringifier::StringToCoderate(elem->Attribute(TRANSPONDER_XML_ATTR_CODERATEH));

  m_coderateL = Stringifier::StringToCoderate(elem->Attribute(TRANSPONDER_XML_ATTR_CODERATEL));

  m_guard = Stringifier::StringToGuard(elem->Attribute(TRANSPONDER_XML_ATTR_GUARD));

  m_hierarchy = Stringifier::StringToHierarchy(elem->Attribute(TRANSPONDER_XML_ATTR_HIERARCHY));

  const char* strStreamId = elem->Attribute(TRANSPONDER_XML_ATTR_STREAMID);
  m_streamId = strStreamId ? StringUtils::IntVal(strStreamId) : 0;

  m_transmission = Stringifier::StringToTransmission(elem->Attribute(TRANSPONDER_XML_ATTR_TRANSMISSION));
}

}
