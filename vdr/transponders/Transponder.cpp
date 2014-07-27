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

#include "Transponder.h"
#include "Stringifier.h"
#include "TransponderDefinitions.h"
#include "utils/StringUtils.h"

#include <tinyxml.h>

using namespace std;

namespace VDR
{

cTransponder::cTransponder(TRANSPONDER_TYPE type /* = TRANSPONDER_INVALID */)
 : m_deliverySystem(SYS_UNDEFINED),
   m_frequencyHz(0),
   m_modulation(QPSK)
{
  SetType(type);
}

void cTransponder::Reset(TRANSPONDER_TYPE type /* = TRANSPONDER_INVALID */)
{
  cTransponder defaults(type);
  *this = defaults;
}

bool cTransponder::operator==(const cTransponder& rhs) const
{
  if (Type() == rhs.Type())
  {
    if (Type() != TRANSPONDER_INVALID)
    {
      if (m_deliverySystem != rhs.m_deliverySystem ||
          m_frequencyHz    != rhs.m_frequencyHz    ||
          m_modulation     != rhs.m_modulation)
      {
        return false;
      }
    }

    switch (Type())
    {
    case TRANSPONDER_ATSC:        return m_atscParams        == rhs.m_atscParams;
    case TRANSPONDER_CABLE:       return m_cableParams       == rhs.m_cableParams;
    case TRANSPONDER_SATELLITE:   return m_satelliteParams   == rhs.m_satelliteParams;
    case TRANSPONDER_TERRESTRIAL: return m_terrestrialParams == rhs.m_terrestrialParams;
    default:
      return true;
    }
  }
  return false;
}

void cTransponder::SetType(TRANSPONDER_TYPE type)
{
  switch (type)
  {
  case TRANSPONDER_ATSC:
  case TRANSPONDER_CABLE:
  case TRANSPONDER_SATELLITE:
  case TRANSPONDER_TERRESTRIAL:
    m_type = type;
    break;
  default:
    m_type = TRANSPONDER_INVALID;
    break;
  }
}

fe_delivery_system_t cTransponder::DeliverySystem(void) const
{
  switch (m_type)
  {
  case TRANSPONDER_ATSC:
    return SYS_ATSC;
  case TRANSPONDER_CABLE:
    return SYS_DVBC_ANNEX_A;
  case TRANSPONDER_SATELLITE:
    return m_deliverySystem == SYS_DVBS2 ? SYS_DVBS2 : SYS_DVBS;
  case TRANSPONDER_TERRESTRIAL:
    return m_deliverySystem == SYS_DVBT2 ? SYS_DVBT2 : SYS_DVBT;
  default:
    return SYS_UNDEFINED;
  }
}

bool cTransponder::Serialise(TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (m_type != TRANSPONDER_INVALID)
  {
    elem->SetAttribute(TRANSPONDER_XML_ATTR_TYPE, Stringifier::TypeToString(m_type));

    if (m_deliverySystem != SYS_UNDEFINED)
      elem->SetAttribute(TRANSPONDER_XML_ATTR_DELIVERY_SYSTEM, Stringifier::DeliverySystemToString(m_deliverySystem));

    if (m_frequencyHz != 0)
      elem->SetAttribute(TRANSPONDER_XML_ATTR_FREQUENCY, m_frequencyHz);

    elem->SetAttribute(TRANSPONDER_XML_ATTR_INVERSION, Stringifier::InversionToString(m_inversion));

    elem->SetAttribute(TRANSPONDER_XML_ATTR_MODULATION, Stringifier::ModulationToString(m_modulation));

    switch (m_type)
    {
    case TRANSPONDER_ATSC:        m_atscParams.Serialise(elem);        break;
    case TRANSPONDER_CABLE:       m_cableParams.Serialise(elem);       break;
    case TRANSPONDER_SATELLITE:   m_satelliteParams.Serialise(elem);   break;
    case TRANSPONDER_TERRESTRIAL: m_terrestrialParams.Serialise(elem); break;
    }
  }

  return true;
}

bool cTransponder::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  Reset();

  m_type = Stringifier::StringToType(elem->Attribute(TRANSPONDER_XML_ATTR_TYPE));

  if (m_type != TRANSPONDER_INVALID)
  {
    m_deliverySystem = Stringifier::StringToDeliverySystem(elem->Attribute(TRANSPONDER_XML_ATTR_DELIVERY_SYSTEM));

    const char* strFrequencyHz = elem->Attribute(TRANSPONDER_XML_ATTR_FREQUENCY);
    m_frequencyHz = strFrequencyHz ? StringUtils::IntVal(strFrequencyHz) : 0;

    m_inversion = Stringifier::StringToInversion(elem->Attribute(TRANSPONDER_XML_ATTR_INVERSION));

    m_modulation = Stringifier::StringToModulation(elem->Attribute(TRANSPONDER_XML_ATTR_MODULATION));

    switch (m_type)
    {
    case TRANSPONDER_ATSC:        m_atscParams.Deserialise(elem);        break;
    case TRANSPONDER_CABLE:       m_cableParams.Deserialise(elem);       break;
    case TRANSPONDER_SATELLITE:   m_satelliteParams.Deserialise(elem);   break;
    case TRANSPONDER_TERRESTRIAL: m_terrestrialParams.Deserialise(elem); break;
    }
  }

  return true;
}

}
