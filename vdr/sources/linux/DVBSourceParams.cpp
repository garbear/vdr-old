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
#include "../../../i18n.h"
#include "../../../menuitems.h"
#include "../../../osdbase.h"

#include <assert.h>
#include <ctype.h> // for toupper
#include <linux/dvb/frontend.h>
#include <sstream>
#include <stdio.h>
#include <string.h>

using namespace std;

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

const DvbParameterVector &cDvbParameters::InversionValues()
{
  static DvbParameterVector values;
  if (values.empty())
  {
    values = ArrayToVector(_InversionValues);
  }
  return values;
}

/*!
 * \brief Break a C array out into a std::vector
 * \param dvbParam the tDvbParameterMap array
 * \return A vector of pointers
 */
template <size_t N>
DvbParameterVector ArrayToVector(const tDvbParameter (&dvbParam)[N])
{
  DvbParameterVector ret;
  for (unsigned int i = 0; i < N; i++)
    ret.push_back(&dvbParam[i]);
  return ret;
}

// --- DVB Parameter Maps ----------------------------------------------------

string cDvbParameters::MapToUserString(int value, const DvbParameterVector &vecParams)
{
  for (DvbParameterVector::const_iterator paramIt = vecParams.begin(); paramIt != vecParams.end(); ++paramIt)
  {
    if ((*paramIt)->driverValue == value)
      return (*paramIt)->userString;
  }
  return "???";
}

int cDvbParameters::MapToUser(int value, const DvbParameterVector &vecParams, string &strString)
{
  for (DvbParameterVector::const_iterator paramIt = vecParams.begin(); paramIt != vecParams.end(); ++paramIt)
  {
    if ((*paramIt)->driverValue == value)
    {
      strString = tr((*paramIt)->userString.c_str());
      return (*paramIt)->userValue;
    }
  }
  return -1;
}

int cDvbParameters::MapToDriver(int value, const DvbParameterVector &vecParams)
{
  for (DvbParameterVector::const_iterator paramIt = vecParams.begin(); paramIt != vecParams.end(); ++paramIt)
  {
    if ((*paramIt)->userValue == value)
      return (*paramIt)->driverValue;
  }
  return -1;
}

const DvbParameterVector &cDvbParameters::InversionValues()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_InversionValues);
  return values;
}

const DvbParameterVector &cDvbParameters::BandwidthValues()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_BandwidthValues);
  return values;
}

const DvbParameterVector &cDvbParameters::CoderateValues()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_CoderateValues);
  return values;
}

const DvbParameterVector &cDvbParameters::ModulationValues()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_ModulationValues);
  return values;
}

const DvbParameterVector &cDvbParameters::SystemValuesSat()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_SystemValuesSat);
  return values;
}

const DvbParameterVector &cDvbParameters::SystemValuesTerr()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_SystemValuesTerr);
  return values;
}

const DvbParameterVector &cDvbParameters::TransmissionValues()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_TransmissionValues);
  return values;
}

const DvbParameterVector &cDvbParameters::GuardValues()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_GuardValues);
  return values;
}

const DvbParameterVector &cDvbParameters::HierarchyValues()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_HierarchyValues);
  return values;
}

const DvbParameterVector &cDvbParameters::RollOffValues()
{
  static DvbParameterVector values;
  if (values.empty())
    values = ArrayToVector(_RollOffValues);
  return values;
}

// --- cDvbTransponderParameters ---------------------------------------------

cDvbTransponderParameters::cDvbTransponderParameters(const string &strParameters)
 : m_polarization(0),
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

string cDvbTransponderParameters::PrintParameter(char name, int value)
{
  char buffer[64] = {0};
  if (0 <= value && value != 999)
    sprintf(buffer, "%c%d", name, value);

  return buffer;
}

string cDvbTransponderParameters::Serialize(char type) const
{
  stringstream strBuffer;
  string dummy;

  if (type == 'A')
  {
    strBuffer << PrintParameter('I', cDvbParameters::MapToUser(m_inversion, cDvbParameters::InversionValues(), dummy));
    strBuffer << PrintParameter('M', cDvbParameters::MapToUser(m_modulation, cDvbParameters::ModulationValues(), dummy));
  }
  else if (type == 'C')
  {
    strBuffer << PrintParameter('C', cDvbParameters::MapToUser(m_coderateH, cDvbParameters::CoderateValues(), dummy));
    strBuffer << PrintParameter('I', cDvbParameters::MapToUser(m_inversion, cDvbParameters::InversionValues(), dummy));
    strBuffer << PrintParameter('M', cDvbParameters::MapToUser(m_modulation, cDvbParameters::ModulationValues(), dummy));
  }
  else if (type == 'S')
  {
    char buffer[64] = {0};
    sprintf(buffer, "%c", m_polarization); // Empty value
    strBuffer << buffer;
    strBuffer << PrintParameter('C', cDvbParameters::MapToUser(m_coderateH, cDvbParameters::CoderateValues(), dummy));
    strBuffer << PrintParameter('I', cDvbParameters::MapToUser(m_inversion, cDvbParameters::InversionValues(), dummy));
    strBuffer << PrintParameter('M', cDvbParameters::MapToUser(m_modulation, cDvbParameters::ModulationValues(), dummy));
    if (m_system == 1)
    {
      strBuffer << PrintParameter('O', cDvbParameters::MapToUser(m_rollOff, cDvbParameters::RollOffValues(), dummy));
      strBuffer << PrintParameter('P', m_streamId);
    }
    strBuffer << PrintParameter('S', cDvbParameters::MapToUser(m_system, cDvbParameters::SystemValuesSat(), dummy)); // we only need the numerical value, so Sat or Terr doesn't matter
  }
  else if (type == 'T')
  {
    strBuffer << PrintParameter('B', cDvbParameters::MapToUser(m_bandwidth, cDvbParameters::BandwidthValues(), dummy));
    strBuffer << PrintParameter('C', cDvbParameters::MapToUser(m_coderateH, cDvbParameters::CoderateValues(), dummy));
    strBuffer << PrintParameter('D', cDvbParameters::MapToUser(m_coderateL, cDvbParameters::CoderateValues(), dummy));
    strBuffer << PrintParameter('G', cDvbParameters::MapToUser(m_guard, cDvbParameters::GuardValues(), dummy));
    strBuffer << PrintParameter('I', cDvbParameters::MapToUser(m_inversion, cDvbParameters::InversionValues(), dummy));
    strBuffer << PrintParameter('M', cDvbParameters::MapToUser(m_modulation, cDvbParameters::ModulationValues(), dummy));
    if (m_system == 1)
      strBuffer << PrintParameter('P', m_streamId);
    strBuffer << PrintParameter('S', cDvbParameters::MapToUser(m_system, cDvbParameters::SystemValuesSat(), dummy)); // we only need the numerical value, so Sat or Terr doesn't matter
    strBuffer << PrintParameter('T', cDvbParameters::MapToUser(m_transmission, cDvbParameters::TransmissionValues(), dummy));
    strBuffer << PrintParameter('Y', cDvbParameters::MapToUser(m_hierarchy, cDvbParameters::HierarchyValues(), dummy));
  }

  return strBuffer.str();
}

string cDvbTransponderParameters::ParseFirstParameter(const string &paramString, int &value)
{
  long n;
  string remainder;
  if (StringUtils::IntVal(paramString.substr(1), n, remainder))
  {
    value = n;
    if (value >= 0)
      return remainder;
  }

  esyslog("ERROR: invalid parameter value: %s", paramString.c_str());
  return "";
}

string cDvbTransponderParameters::ParseFirstParameter(const string &paramString, int &value, const DvbParameterVector &vecParams)
{
  assert(!vecParams.empty());
  long n;
  string remainder;
  // Skip the
  if (StringUtils::IntVal(paramString.substr(1), n, remainder))
  {
    value = cDvbParameters::MapToDriver(n, vecParams);
    if (value >= 0)
      return remainder;
  }

  esyslog("ERROR: invalid parameter value: %s", paramString.c_str());
  return "";
}

bool cDvbTransponderParameters::Deserialize(const std::string &str)
{
  while (str.size())
  {
    char c = str[0];
    str = str.substr(1);
    switch (toupper(c))
    {
    case 'B': str = ParseFirstParameter(str, m_bandwidth,    cDvbParameters::BandwidthValues());    break;
    case 'C': str = ParseFirstParameter(str, m_coderateH,    cDvbParameters::CoderateValues());     break;
    case 'D': str = ParseFirstParameter(str, m_coderateL,    cDvbParameters::CoderateValues());     break;
    case 'G': str = ParseFirstParameter(str, m_guard,        cDvbParameters::GuardValues());        break;
    case 'I': str = ParseFirstParameter(str, m_inversion,    cDvbParameters::InversionValues());    break;
    case 'M': str = ParseFirstParameter(str, m_modulation,   cDvbParameters::ModulationValues());   break;
    case 'O': str = ParseFirstParameter(str, m_rollOff,      cDvbParameters::RollOffValues());      break;
    case 'P': str = ParseFirstParameter(str, m_streamId);                                           break;
    case 'S': str = ParseFirstParameter(str, m_system,       cDvbParameters::SystemValuesSat());    break; // we only need the numerical value, so Sat or Terr doesn't matter
    case 'T': str = ParseFirstParameter(str, m_transmission, cDvbParameters::TransmissionValues()); break;
    case 'Y': str = ParseFirstParameter(str, m_hierarchy,    cDvbParameters::HierarchyValues());    break;

    case 'H': m_polarization = 'H'; break;
    case 'L': m_polarization = 'L'; break;
    case 'R': m_polarization = 'R'; break;
    case 'V': m_polarization = 'V'; break;

    default:
      esyslog("ERROR: unknown parameter key '%c'", c);
      return false;
    }
  }
  return true;
}

// --- cDvbSourceParams -------------------------------------------------------

cDvbSourceParams::cDvbSourceParams(char source, const string &strDescription)
 : iSourceParams(source, strDescription),
   m_param(0),
   m_srate(0)
{
}

void cDvbSourceParams::SetData(const cChannel &channel)
{
  m_srate = channel.Srate();
  m_dtp.Deserialize(channel.Parameters());
  m_param = 0;
}

void cDvbSourceParams::GetData(cChannel &channel) const
{
  channel.SetTransponderData(channel.Source(), channel.Frequency(), m_srate, m_dtp.Serialize(Source()).c_str(), true);
  // TODO: Overload index operators
  //channel[frequency][source] = TransponderData
}

/*
cOsdItem *cDvbSourceParams::GetOsdItem()
{
  char type = Source();
  const tDvbParameterMap *SystemValues = (type == 'S' ? SystemValuesSat : SystemValuesTerr);
  switch (m_param++)
  {
    case  0: return strchr("  S ", type) ? new cMenuEditChrItem(tr("Polarization"), &m_dtp.m_polarization, "HVLR")             : GetOsdItem();
    case  1: return strchr("  ST", type) ? new cMenuEditMapItem(tr("System"),       &m_dtp.m_system,       SystemValues)       : GetOsdItem();
    case  2: return strchr(" CS ", type) ? new cMenuEditIntItem(tr("Srate"),        &m_srate)                                  : GetOsdItem();
    case  3: return strchr("ACST", type) ? new cMenuEditMapItem(tr("Inversion"),    &m_dtp.m_inversion,    InversionValues)    : GetOsdItem();
    case  4: return strchr(" CST", type) ? new cMenuEditMapItem(tr("CoderateH"),    &m_dtp.m_coderateH,    CoderateValues)     : GetOsdItem();
    case  5: return strchr("   T", type) ? new cMenuEditMapItem(tr("CoderateL"),    &m_dtp.m_coderateL,    CoderateValues)     : GetOsdItem();
    case  6: return strchr("ACST", type) ? new cMenuEditMapItem(tr("Modulation"),   &m_dtp.m_modulation,   ModulationValues)   : GetOsdItem();
    case  7: return strchr("   T", type) ? new cMenuEditMapItem(tr("Bandwidth"),    &m_dtp.m_bandwidth,    BandwidthValues)    : GetOsdItem();
    case  8: return strchr("   T", type) ? new cMenuEditMapItem(tr("Transmission"), &m_dtp.m_transmission, TransmissionValues) : GetOsdItem();
    case  9: return strchr("   T", type) ? new cMenuEditMapItem(tr("Guard"),        &m_dtp.m_guard,        GuardValues)        : GetOsdItem();
    case 10: return strchr("   T", type) ? new cMenuEditMapItem(tr("Hierarchy"),    &m_dtp.m_hierarchy,    HierarchyValues)    : GetOsdItem();
    case 11: return strchr("  S ", type) ? new cMenuEditMapItem(tr("Rolloff"),      &m_dtp.m_rollOff,      RollOffValues)      : GetOsdItem();
    case 12: return strchr("  ST", type) ? new cMenuEditIntItem(tr("StreamId"),     &m_dtp.m_streamId,     0, 255)             : GetOsdItem();
    default: return NULL;
  }
  return NULL;
}
*/
