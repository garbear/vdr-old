/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "DVBSourceParams.h"
#include "utils/StringUtils.h"
//#include "channels.h"
class cChannel { };
//#include "i18n.h"
#define trNOOP(s) (s)
//#include "tools.h" // for logging

#include <ctype.h> // for toupper
#include <sstream>

using namespace std;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))
#endif

struct tDvbParameter
{
  int         userValue;
  int         driverValue;
  const char *userString;
};

const tDvbParameter _InversionValues[] =
{
  {   0, INVERSION_OFF,  trNOOP("off") },
  {   1, INVERSION_ON,   trNOOP("on") },
  { 999, INVERSION_AUTO, trNOOP("auto") },
};

const tDvbParameter _BandwidthValues[] =
{
  {    5,  5000000, "5 MHz" },
  {    6,  6000000, "6 MHz" },
  {    7,  7000000, "7 MHz" },
  {    8,  8000000, "8 MHz" },
  {   10, 10000000, "10 MHz" },
  { 1712,  1712000, "1.712 MHz" },
};

const tDvbParameter _CoderateValues[] =
{
  {   0, FEC_NONE, trNOOP("none") },
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
  { 999, FEC_AUTO, trNOOP("auto") },
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
  { 999, QAM_AUTO, trNOOP("auto") },
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
  { 999, TRANSMISSION_MODE_AUTO, trNOOP("auto") },
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
  {   999, GUARD_INTERVAL_AUTO,   trNOOP("auto") },
};

const tDvbParameter _HierarchyValues[] =
{
  {   0, HIERARCHY_NONE, trNOOP("none") },
  {   1, HIERARCHY_1,    "1" },
  {   2, HIERARCHY_2,    "2" },
  {   4, HIERARCHY_4,    "4" },
  { 999, HIERARCHY_AUTO, trNOOP("auto") },
};

const tDvbParameter _RollOffValues[] =
{
  {   0, ROLLOFF_AUTO, trNOOP("auto") },
  {  20, ROLLOFF_20, "0.20" },
  {  25, ROLLOFF_25, "0.25" },
  {  35, ROLLOFF_35, "0.35" },
};

// --- cDvbTransponderParams ---------------------------------------------

cDvbTransponderParams::cDvbTransponderParams(const string &strParameters)
 : m_polarization(0), // TODO: Default to a valid polarization: H, L, R, V
   m_inversion(INVERSION_AUTO),
   m_bandwidth(8000000),
   m_coderateH(FEC_AUTO),
   m_coderateL(FEC_AUTO),
   m_modulation(QPSK),
   m_system(DVB_SYSTEM_1),
   m_transmission(TRANSMISSION_MODE_AUTO),
   m_guard(GUARD_INTERVAL_AUTO),
   m_hierarchy(HIERARCHY_AUTO),
   m_rollOff(ROLLOFF_AUTO),
   m_streamId(0)
{
  Deserialize(strParameters);
}

/*!
 * \brief Serialization helper function
 * \param name The parameter name (upper case letter)
 * \param paramValue The parameter's value
 * \return Concatenated string of name + parameter's value
 */
string SerializeDriverValue(char name, int paramValue)
{
  return StringUtils::Format("%c%d", name, paramValue);
}

/*!
 * \brief Serialization helper function
 * \param name The parameter name (upper case letter)
 * \param driverValue The parameter's driver value
 * \param dvbParamTable The DVB parameter lookup table (see tables above)
 * \return Concatenated string of name + parameter's user value
 */
template <size_t N>
string SerializeDriverValue(char name, int driverValue, const tDvbParameter (&dvbParamTable)[N])
{
  for (unsigned int i = 0; i < N; i++)
  {
    if (dvbParamTable[i].driverValue == driverValue)
      return StringUtils::Format("%c%d", name, dvbParamTable[i].userValue);
  }
  return "";
}

string cDvbTransponderParams::Serialize(char type) const
{
  stringstream strBuffer;

  if (type == 'A')
  {
    strBuffer << SerializeDriverValue('I', m_inversion,    _InversionValues);
    strBuffer << SerializeDriverValue('M', m_modulation,   _ModulationValues);
  }
  else if (type == 'C')
  {
    strBuffer << SerializeDriverValue('C', m_coderateH,    _CoderateValues);
    strBuffer << SerializeDriverValue('I', m_inversion,    _InversionValues);
    strBuffer << SerializeDriverValue('M', m_modulation,   _ModulationValues);
  }
  else if (type == 'S')
  {
    // Only record valid polarizations
    if (m_polarization == 'H' ||
        m_polarization == 'L' ||
        m_polarization == 'R' ||
        m_polarization == 'V')
      strBuffer << m_polarization;
    strBuffer << SerializeDriverValue('C', m_coderateH,    _CoderateValues);
    strBuffer << SerializeDriverValue('I', m_inversion,    _InversionValues);
    strBuffer << SerializeDriverValue('M', m_modulation,   _ModulationValues);
    if (m_system == 1)
    {
      strBuffer << SerializeDriverValue('O', m_rollOff,    _RollOffValues);
      strBuffer << SerializeDriverValue('P', m_streamId);
    }
    strBuffer << SerializeDriverValue('S', m_system,       _SystemValuesSat); // we only need the numerical value, so Sat or Terr doesn't matter
  }
  else if (type == 'T')
  {
    strBuffer << SerializeDriverValue('B', m_bandwidth,    _BandwidthValues);
    strBuffer << SerializeDriverValue('C', m_coderateH,    _CoderateValues);
    strBuffer << SerializeDriverValue('D', m_coderateL,    _CoderateValues);
    strBuffer << SerializeDriverValue('G', m_guard,        _GuardValues);
    strBuffer << SerializeDriverValue('I', m_inversion,    _InversionValues);
    strBuffer << SerializeDriverValue('M', m_modulation,   _ModulationValues);
    if (m_system == 1)
      strBuffer << SerializeDriverValue('P', m_streamId);
    strBuffer << SerializeDriverValue('S', m_system,       _SystemValuesSat); // we only need the numerical value, so Sat or Terr doesn't matter
    strBuffer << SerializeDriverValue('T', m_transmission, _TransmissionValues);
    strBuffer << SerializeDriverValue('Y', m_hierarchy,    _HierarchyValues);
  }

  return strBuffer.str();
}

/*!
 * \brief Deserialization helper function
 * \param str The serialized string beginning with the parameter's value
 * \param paramValue The param whose value is set
 *
 * If successful, str will be truncated just past the number (e.g. "99C3..."
 * becomes "C3..."). Otherwise, str will be cleared (stopping deserialization).
 */
void DeserializeDriverValue(string &str, int &paramValue)
{
  long temp;
  if (StringUtils::IntVal(str, temp, str))
    paramValue = temp;
  else
    str = ""; // Don't continue deserializing
}

/*!
 * \brief Deserialization helper function
 * \param str The serialized string beginning with the parameter's user value
 * \param driverValue The param whose value is set
 * \param dvbParamTable The DVB parameter lookup table (see tables above)
 *
 * If str begins with a number, it will be truncated just past the number (e.g.
 * "99C3..." becomes "C3..."). If the user value doesn't exist in the lookup
 * table, the string will still be truncated but driverValue will be untouched.
 * If str doesn't begin with a number, str will be cleared (stopping
 * deserialization).
 */
template <size_t N>
void DeserializeDriverValue(string &str, int &driverValue, const tDvbParameter (&dvbParamTable)[N])
{
  long userValue;
  if (StringUtils::IntVal(str, userValue, str))
  {
    for (unsigned int i = 0; i < N; i++)
    {
      if (dvbParamTable[i].userValue == userValue)
        driverValue = dvbParamTable[i].userValue;
    }
  }
  else
    str = ""; // Don't continue deserializing
}

bool cDvbTransponderParams::Deserialize(const std::string &str)
{
  string str2(str);

  while (str2.size())
  {
    char c = str2[0];
    str2 = str2.substr(1);

    switch (toupper(c))
    {
    case 'B': DeserializeDriverValue(str2, m_bandwidth,    _BandwidthValues);    break;
    case 'C': DeserializeDriverValue(str2, m_coderateH,    _CoderateValues);     break;
    case 'D': DeserializeDriverValue(str2, m_coderateL,    _CoderateValues);     break;
    case 'G': DeserializeDriverValue(str2, m_guard,        _GuardValues);        break;
    case 'I': DeserializeDriverValue(str2, m_inversion,    _InversionValues);    break;
    case 'M': DeserializeDriverValue(str2, m_modulation,   _ModulationValues);   break;
    case 'P': DeserializeDriverValue(str2, m_streamId);                          break;
    case 'O': DeserializeDriverValue(str2, m_rollOff,      _RollOffValues);      break;
    case 'S': DeserializeDriverValue(str2, m_system,       _SystemValuesSat);    break; // we only need the numerical value, so Sat or Terr doesn't matter
    case 'T': DeserializeDriverValue(str2, m_transmission, _TransmissionValues); break;
    case 'Y': DeserializeDriverValue(str2, m_hierarchy,    _HierarchyValues);    break;

    case 'H':
    case 'L':
    case 'R':
    case 'V':
      m_polarization = toupper(c);
      break;

    default:
      //esyslog("ERROR: unknown parameter key '%c' in string '%s'", c, str.c_str());
      return false;
    }
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

// --- cDvbSourceParams -------------------------------------------------------

cDvbSourceParams::cDvbSourceParams(char source, const string &strDescription)
 : iSourceParams(source, strDescription),
   m_srate(0)
{
}

void cDvbSourceParams::SetData(const cChannel &channel)
{
  //m_srate = channel.Srate();
  //m_dtp.Deserialize(channel.Parameters());
}

void cDvbSourceParams::GetData(cChannel &channel) const
{
  //channel.SetTransponderData(channel.Source(), channel.Frequency(), m_srate, m_dtp.Serialize(Source()).c_str(), true);
  // TODO: Overload index operators
  //channel[frequency][source] = TransponderData
}
