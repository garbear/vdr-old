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

#include "DVBTransponderParams.h"
#include "channels/ChannelDefinitions.h"
#include "utils/CommonMacros.h" // for ARRAY_SIZE()
#include "utils/log/Log.h"
#include "utils/StringUtils.h"

#include <ctype.h>
#include <tinyxml.h>

using namespace std;

namespace VDR
{

#define INT_INVALID  -1

struct tDvbParameter
{
  int         userValue; // Backwards compatibility
  int         driverValue;
  const char* userString;
};

const tDvbParameter _PolarizationValues[] =
{
  { 0, POLARIZATION_HORIZONTAL,     "horizontal" },
  { 1, POLARIZATION_VERTICAL,       "vertical" },
  { 2, POLARIZATION_CIRCULAR_LEFT,  "left" },
  { 3, POLARIZATION_CIRCULAR_RIGHT, "right" },
};

const tDvbParameter _InversionValues[] =
{
  {   0, INVERSION_OFF,  "off" },
  {   1, INVERSION_ON,   "on" },
  { 999, INVERSION_AUTO, "auto" },
};

const tDvbParameter _BandwidthValues[] =
{
  {    5,  BANDWIDTH_5_MHZ,     "5 MHz" },
  {    6,  BANDWIDTH_6_MHZ,     "6 MHz" },
  {    7,  BANDWIDTH_7_MHZ,     "7 MHz" },
  {    8,  BANDWIDTH_8_MHZ,     "8 MHz" },
  {   10,  BANDWIDTH_10_MHZ,    "10 MHz" },
  { 1712,  BANDWIDTH_1_712_MHZ, "1.712 MHz" },
};

const tDvbParameter _CoderateValues[] =
{
  {   0, FEC_NONE, "none" },
  {  12, FEC_1_2,  "1/2" },
  {  23, FEC_2_3,  "2/3" },
  {  34, FEC_3_4,  "3/4" },
  {  35, FEC_3_5,  "3/5" },
  {  45, FEC_4_5,  "4/5" },
  {  56, FEC_5_6,  "5/6" },
  {  67, FEC_6_7,  "6/7" },
  {  78, FEC_7_8,  "7/8" },
  {  89, FEC_8_9,  "8/9" },
  { 910, FEC_9_10, "9/10" },
  { 999, FEC_AUTO, "auto" },
};

const tDvbParameter _ModulationValues[] =
{
  {  16, QAM_16,   "QAM16" },
  {  32, QAM_32,   "QAM32" },
  {  64, QAM_64,   "QAM64" },
  { 128, QAM_128,  "QAM128" },
  { 256, QAM_256,  "QAM256" },
  {   2, QPSK,     "QPSK" },
  {   5, PSK_8,    "8PSK" },
  {   6, APSK_16,  "16APSK" },
  {   7, APSK_32,  "32APSK" },
  {  10, VSB_8,    "VSB8" },
  {  11, VSB_16,   "VSB16" },
  {  12, DQPSK,    "DQPSK" },
  { 999, QAM_AUTO, "auto" },
};

const tDvbParameter _SystemValuesSat[] =
{
  {   0, DVB_SYSTEM_1, "DVB-S" },
  {   1, DVB_SYSTEM_2, "DVB-S2" },
};

const tDvbParameter _SystemValuesTerr[] =
{
  {   0, DVB_SYSTEM_1, "DVB-T" },
  {   1, DVB_SYSTEM_2, "DVB-T2" },
};

const tDvbParameter _TransmissionValues[] =
{
  {   1, TRANSMISSION_MODE_1K,   "1K" },
  {   2, TRANSMISSION_MODE_2K,   "2K" },
  {   4, TRANSMISSION_MODE_4K,   "4K" },
  {   8, TRANSMISSION_MODE_8K,   "8K" },
  {  16, TRANSMISSION_MODE_16K,  "16K" },
  {  32, TRANSMISSION_MODE_32K,  "32K" },
  { 999, TRANSMISSION_MODE_AUTO, "auto" },
};

const tDvbParameter _GuardValues[] =
{
  {     4, GUARD_INTERVAL_1_4,    "1/4" },
  {     8, GUARD_INTERVAL_1_8,    "1/8" },
  {    16, GUARD_INTERVAL_1_16,   "1/16" },
  {    32, GUARD_INTERVAL_1_32,   "1/32" },
  {   128, GUARD_INTERVAL_1_128,  "1/128" },
  { 19128, GUARD_INTERVAL_19_128, "19/128" },
  { 19256, GUARD_INTERVAL_19_256, "19/256" },
  {   999, GUARD_INTERVAL_AUTO,   "auto" },
};

const tDvbParameter _HierarchyValues[] =
{
  {   0, HIERARCHY_NONE, "none" },
  {   1, HIERARCHY_1,    "1" },
  {   2, HIERARCHY_2,    "2" },
  {   4, HIERARCHY_4,    "4" },
  { 999, HIERARCHY_AUTO, "auto" },
};

const tDvbParameter _RollOffValues[] =
{
  {   0, ROLLOFF_AUTO, "auto" },
  {  20, ROLLOFF_20,   "0.20" },
  {  25, ROLLOFF_25,   "0.25" },
  {  35, ROLLOFF_35,   "0.35" },
};

cDvbTransponderParams::cDvbTransponderParams(
   fe_polarization       polarization /* = POLARIZATION_HORIZONTAL */,
   fe_spectral_inversion inversion    /* = INVERSION_AUTO */,
   fe_bandwidth          bandwidth    /* = BANDWIDTH_8_MHZ */,
   fe_code_rate          coderateH    /* = FEC_AUTO */,
   fe_code_rate          coderateL    /* = FEC_AUTO */,
   fe_modulation         modulation   /* = QPSK */,
   eSystemType           system       /* = DVB_SYSTEM_1 */,
   fe_transmit_mode      transmission /* = TRANSMISSION_MODE_AUTO */,
   fe_guard_interval     guard        /* = GUARD_INTERVAL_AUTO */,
   fe_hierarchy          hierarchy    /* = HIERARCHY_AUTO */,
   fe_rolloff            rollOff      /* = ROLLOFF_AUTO */
)
 : m_polarization(polarization),
   m_inversion(inversion),
   m_bandwidth(bandwidth),
   m_coderateH(coderateH),
   m_coderateL(coderateL),
   m_modulation(modulation),
   m_system(system),
   m_transmission(transmission),
   m_guard(guard),
   m_hierarchy(hierarchy),
   m_rollOff(rollOff),
   m_streamId(0)
{
}

bool cDvbTransponderParams::operator==(const cDvbTransponderParams& rhs) const
{
  return m_polarization == rhs.m_polarization &&
         m_inversion    == rhs.m_inversion &&
         m_bandwidth    == rhs.m_bandwidth &&
         m_coderateH    == rhs.m_coderateH &&
         m_coderateL    == rhs.m_coderateL &&
         m_modulation   == rhs.m_modulation &&
         m_system       == rhs.m_system &&
         m_transmission == rhs.m_transmission &&
         m_guard        == rhs.m_guard &&
         m_hierarchy    == rhs.m_hierarchy &&
         m_rollOff      == rhs.m_rollOff &&
         m_streamId     == rhs.m_streamId;
}

void cDvbTransponderParams::Reset()
{
  cDvbTransponderParams defaults;
  *this = defaults;
}

unsigned int cDvbTransponderParams::BandwidthHz() const
{
  switch (m_bandwidth)
  {
  case BANDWIDTH_8_MHZ:     return 8 * 1000 * 1000;
  case BANDWIDTH_7_MHZ:     return 7 * 1000 * 1000;
  case BANDWIDTH_6_MHZ:     return 6 * 1000 * 1000;
  case BANDWIDTH_5_MHZ:     return 5 * 1000 * 1000;
  case BANDWIDTH_10_MHZ:    return 10 * 1000 * 1000;
  case BANDWIDTH_1_712_MHZ: return 1712 * 1000;
  case BANDWIDTH_AUTO:      return 0; // ???
  default:                  return 0;
  }
}

/*!
 * \brief Serialisation helper function
 */
template <typename T, size_t N>
const char* SerialiseDriverValue(T driverValue, const tDvbParameter (&dvbParamTable)[N])
{
  for (unsigned int i = 0; i < N; i++)
  {
    if (driverValue == (T)dvbParamTable[i].driverValue)
      return dvbParamTable[i].userString;
  }
  return "";
}

bool cDvbTransponderParams::Serialise(eDvbType type, TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  switch (type)
  {
  case DVB_ATSC:
    element->SetAttribute(TRANSPONDER_XML_ATTR_INVERSION, SerialiseDriverValue(m_inversion, _InversionValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_MODULATION, SerialiseDriverValue(m_modulation, _ModulationValues));
    break;

  case DVB_CABLE:
    element->SetAttribute(TRANSPONDER_XML_ATTR_CODERATEH, SerialiseDriverValue(m_coderateH, _CoderateValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_INVERSION, SerialiseDriverValue(m_inversion, _InversionValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_MODULATION, SerialiseDriverValue(m_modulation, _ModulationValues));
    break;

  case DVB_SAT:
    element->SetAttribute(TRANSPONDER_XML_ATTR_POLARIZATION, SerialiseDriverValue(m_polarization, _PolarizationValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_CODERATEH, SerialiseDriverValue(m_coderateH, _CoderateValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_INVERSION, SerialiseDriverValue(m_inversion, _InversionValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_MODULATION, SerialiseDriverValue(m_modulation, _ModulationValues));
    if (m_system == DVB_SYSTEM_2)
    {
      element->SetAttribute(TRANSPONDER_XML_ATTR_ROLLOFF, SerialiseDriverValue(m_rollOff, _RollOffValues));
      element->SetAttribute(TRANSPONDER_XML_ATTR_STREAMID, m_streamId);
    }
    element->SetAttribute(TRANSPONDER_XML_ATTR_SYSTEM, SerialiseDriverValue(m_system, _SystemValuesSat));
    break;

  case DVB_TERR:
    element->SetAttribute(TRANSPONDER_XML_ATTR_BANDWIDTH, SerialiseDriverValue(m_bandwidth, _BandwidthValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_CODERATEH, SerialiseDriverValue(m_coderateH, _CoderateValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_CODERATEL, SerialiseDriverValue(m_coderateL, _CoderateValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_GUARD, SerialiseDriverValue(m_guard, _GuardValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_INVERSION, SerialiseDriverValue(m_inversion, _InversionValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_MODULATION, SerialiseDriverValue(m_modulation, _ModulationValues));
    if (m_system == DVB_SYSTEM_2)
      element->SetAttribute(TRANSPONDER_XML_ATTR_STREAMID, m_streamId);
    element->SetAttribute(TRANSPONDER_XML_ATTR_SYSTEM, SerialiseDriverValue(m_system, _SystemValuesTerr));
    element->SetAttribute(TRANSPONDER_XML_ATTR_TRANSMISSION, SerialiseDriverValue(m_transmission, _TransmissionValues));
    element->SetAttribute(TRANSPONDER_XML_ATTR_HIERARCHY, SerialiseDriverValue(m_hierarchy, _HierarchyValues));
    break;
  }

  return true;
}

/*!
 * \brief Serialisation helper function
 */
template <typename T, size_t N>
void DeserialiseDriverValue(const char* userString, T &driverValue, const tDvbParameter (&dvbParamTable)[N])
{
  if (userString != NULL)
  {
    for (unsigned int i = 0; i < N; i++)
    {
      if (StringUtils::EqualsNoCase(userString, dvbParamTable[i].userString))
      {
        driverValue = (T)dvbParamTable[i].driverValue;
        break;
      }
    }
  }
}

bool cDvbTransponderParams::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  Reset();

  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_POLARIZATION), m_polarization, _PolarizationValues);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_INVERSION), m_inversion, _InversionValues);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_BANDWIDTH), m_bandwidth, _BandwidthValues);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_CODERATEH), m_coderateH, _CoderateValues);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_CODERATEL), m_coderateL, _CoderateValues);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_MODULATION), m_modulation, _ModulationValues);

  // Try to deserialise system against both satellite and terrestrial values
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_SYSTEM), m_system, _SystemValuesSat);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_SYSTEM), m_system, _SystemValuesTerr);

  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_TRANSMISSION), m_transmission, _TransmissionValues);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_GUARD), m_guard, _GuardValues);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_HIERARCHY), m_hierarchy, _HierarchyValues);
  DeserialiseDriverValue(elem->Attribute(TRANSPONDER_XML_ATTR_ROLLOFF), m_rollOff, _RollOffValues);

  const char* streamId = elem->Attribute(TRANSPONDER_XML_ATTR_STREAMID);
  if (streamId != NULL)
    m_streamId = StringUtils::IntVal(streamId);

  return true;
}

/*!
 * \brief Deserialisation helper function
 * \param str The serialized string beginning with the parameter's value
 * \param paramValue The param whose value is set
 */
template <typename T>
bool DeserialiseDriverValueConf(const string &str, T &paramValue)
{
  long temp = StringUtils::IntVal(str, INT_INVALID);
  if (temp != INT_INVALID)
  {
    paramValue = (T)temp;
    return true;
  }
  return false; // Don't continue deserialising
}

/*!
 * \brief Deserialisation helper function
 * \param str The serialized string beginning with the parameter's user value
 * \param driverValue The param whose value is set
 * \param dvbParamTable The DVB parameter lookup table (see tables above)
 */
template <typename T, size_t N>
bool DeserialiseDriverValueConf(const string &str, T &driverValue, const tDvbParameter (&dvbParamTable)[N])
{
  long userValue = StringUtils::IntVal(str, INT_INVALID);
  if (userValue != INT_INVALID)
  {
    for (unsigned int i = 0; i < N; i++)
    {
      if (dvbParamTable[i].userValue == userValue)
      {
        driverValue = (T)dvbParamTable[i].driverValue;
        return true;
      }
    }
  }
  return false; // Don't continue deserialising
}

bool cDvbTransponderParams::Deserialise(const string& strParameters)
{
  for (unsigned int i = 0; i < strParameters.size(); )
  {
    switch (toupper(strParameters[i]))
    {
    case 'B':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_bandwidth, _BandwidthValues))
        return false;
      break;
    case 'C':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_coderateH, _CoderateValues))
        return false;
      break;
    case 'D':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_coderateL, _CoderateValues))
        return false;
      break;
    case 'G':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_guard, _GuardValues))
        return false;
      break;
    case 'I':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_inversion, _InversionValues))
        return false;
      break;
    case 'M':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_modulation, _ModulationValues))
        return false;
      break;
    case 'P':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_streamId))
        return false;
      break;
    case 'O':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_rollOff, _RollOffValues))
        return false;
      break;
    case 'S':
      // we only need the numerical value, so Sat or Terr doesn't matter
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_system, _SystemValuesSat))
        return false;
      break;
    case 'T':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_transmission, _TransmissionValues))
        return false;
      break;
    case 'Y':
      if (!DeserialiseDriverValueConf(strParameters.substr(i + 1), m_hierarchy, _HierarchyValues))
        return false;
      break;

    case 'H': m_polarization = POLARIZATION_HORIZONTAL;     break;
    case 'L': m_polarization = POLARIZATION_CIRCULAR_LEFT;  break;
    case 'R': m_polarization = POLARIZATION_CIRCULAR_RIGHT; break;
    case 'V': m_polarization = POLARIZATION_VERTICAL;       break;

    default:
      //esyslog("ERROR: unknown parameter key '%c' in string '%s'", c, str.c_str());
      return false;
    }

    // Fast-forward past parameter and its number
    i++;
    while (i < strParameters.size() && isdigit(strParameters[i]))
      i++;
  }
  return true;
}

const char *cDvbTransponderParams::TranslateModulation(fe_modulation modulation)
{
  for (unsigned int i = 0; i < ARRAY_SIZE(_ModulationValues); i++)
  {
    if (_ModulationValues[i].driverValue == modulation)
      return _ModulationValues[i].userString;
  }
  return "";
}

vector<string> cDvbTransponderParams::GetModulationsFromCaps(fe_caps_t caps)
{
  vector<string> modulations;

  if (caps & FE_CAN_QPSK)      { modulations.push_back(cDvbTransponderParams::TranslateModulation(QPSK)); }
  if (caps & FE_CAN_QAM_16)    { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_16)); }
  if (caps & FE_CAN_QAM_32)    { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_32)); }
  if (caps & FE_CAN_QAM_64)    { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_64)); }
  if (caps & FE_CAN_QAM_128)   { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_128)); }
  if (caps & FE_CAN_QAM_256)   { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_256)); }
  if (caps & FE_CAN_8VSB)      { modulations.push_back(cDvbTransponderParams::TranslateModulation(VSB_8)); }
  if (caps & FE_CAN_16VSB)     { modulations.push_back(cDvbTransponderParams::TranslateModulation(VSB_16)); }
  if (caps & FE_CAN_TURBO_FEC) { modulations.push_back("TURBO_FEC"); }

  return modulations;
}

}
