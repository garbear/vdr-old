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

#include "DVBParameters.h"
#include "../../../i18n.h"
#include "../../../menuitems.h"
#include "../../../osdbase.h"

#include <linux/dvb/frontend.h>
#include <stdio.h>
#include <string.h>

using namespace std;

// TODO: ?!?!?
inline bool ST(const char *s, char type, int system) { return strchr(s, type) && (strchr(s, '0' + system + 1) || strchr(s, '*')); }

// --- DVB Parameter Maps ----------------------------------------------------

const tDvbParameterMap InversionValues[] =
{
  {   0, INVERSION_OFF,  trNOOP("off") },
  {   1, INVERSION_ON,   trNOOP("on") },
  { 999, INVERSION_AUTO, trNOOP("auto") },
  {  -1, 0, NULL }
};

const tDvbParameterMap BandwidthValues[] =
{
  {    5,  5000000, "5 MHz" },
  {    6,  6000000, "6 MHz" },
  {    7,  7000000, "7 MHz" },
  {    8,  8000000, "8 MHz" },
  {   10, 10000000, "10 MHz" },
  { 1712,  1712000, "1.712 MHz" },
  {  -1, 0, NULL }
};

const tDvbParameterMap CoderateValues[] =
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
  {  -1, 0, NULL }
};

const tDvbParameterMap ModulationValues[] =
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
  {  -1, 0, NULL }
};

const tDvbParameterMap SystemValuesSat[] =
{
  {   0, DVB_SYSTEM_1, "DVB-S" },
  {   1, DVB_SYSTEM_2, "DVB-S2" },
  {  -1, 0, NULL }
};

const tDvbParameterMap SystemValuesTerr[] =
{
  {   0, DVB_SYSTEM_1, "DVB-T" },
  {   1, DVB_SYSTEM_2, "DVB-T2" },
  {  -1, 0, NULL }
};

const tDvbParameterMap TransmissionValues[] =
{
  {   1, TRANSMISSION_MODE_1K,   "1K" },
  {   2, TRANSMISSION_MODE_2K,   "2K" },
  {   4, TRANSMISSION_MODE_4K,   "4K" },
  {   8, TRANSMISSION_MODE_8K,   "8K" },
  {  16, TRANSMISSION_MODE_16K,  "16K" },
  {  32, TRANSMISSION_MODE_32K,  "32K" },
  { 999, TRANSMISSION_MODE_AUTO, trNOOP("auto") },
  {  -1, 0, NULL }
};

const tDvbParameterMap GuardValues[] =
{
  {     4, GUARD_INTERVAL_1_4,    "1/4" },
  {     8, GUARD_INTERVAL_1_8,    "1/8" },
  {    16, GUARD_INTERVAL_1_16,   "1/16" },
  {    32, GUARD_INTERVAL_1_32,   "1/32" },
  {   128, GUARD_INTERVAL_1_128,  "1/128" },
  { 19128, GUARD_INTERVAL_19_128, "19/128" },
  { 19256, GUARD_INTERVAL_19_256, "19/256" },
  {   999, GUARD_INTERVAL_AUTO,   trNOOP("auto") },
  {  -1, 0, NULL }
};

const tDvbParameterMap HierarchyValues[] =
{
  {   0, HIERARCHY_NONE, trNOOP("none") },
  {   1, HIERARCHY_1,    "1" },
  {   2, HIERARCHY_2,    "2" },
  {   4, HIERARCHY_4,    "4" },
  { 999, HIERARCHY_AUTO, trNOOP("auto") },
  {  -1, 0, NULL }
};

const tDvbParameterMap RollOffValues[] =
{
  {   0, ROLLOFF_AUTO, trNOOP("auto") },
  {  20, ROLLOFF_20, "0.20" },
  {  25, ROLLOFF_25, "0.25" },
  {  35, ROLLOFF_35, "0.35" },
  {  -1, 0, NULL }
};

string DvbParameters::MapToUserString(int value, const tDvbParameterMap *map)
{
  int n = DriverIndex(value, map);
  if (n >= 0)
    return map[n].userString;
  return "???";
}

int DvbParameters::MapToUser(int value, const tDvbParameterMap *map, string &strString)
{
  int n = DriverIndex(value, map);
  if (n >= 0)
  {
    strString = tr(map[n].userString.c_str());
    return map[n].userValue;
  }
  return -1;
}

int DvbParameters::MapToDriver(int value, const tDvbParameterMap *map)
{
  int n = UserIndex(value, map);
  if (n >= 0)
    return map[n].driverValue;
  return -1;
}

int DvbParameters::UserIndex(int value, const tDvbParameterMap *map)
{
  const tDvbParameterMap *m = map;
  while (m && m->userValue != -1)
  {
    if (m->userValue == value)
      return m - map;
    m++;
  }
  return -1;
}

int DvbParameters::DriverIndex(int value, const tDvbParameterMap *map)
{
  const tDvbParameterMap *m = map;
  while (m && m->userValue != -1)
  {
    if (m->driverValue == value)
      return m - map;
    m++;
  }
  return -1;
}

// --- cDvbTransponderParameters ---------------------------------------------

cDvbTransponderParameters::cDvbTransponderParameters(const string &strParameters)
{
  m_polarization = 0;
  m_inversion    = INVERSION_AUTO;
  m_bandwidth    = 8000000;
  m_coderateH    = FEC_AUTO;
  m_coderateL    = FEC_AUTO;
  m_modulation   = QPSK;
  m_system       = DVB_SYSTEM_1;
  m_transmission = TRANSMISSION_MODE_AUTO;
  m_guard        = GUARD_INTERVAL_AUTO;
  m_hierarchy    = HIERARCHY_AUTO;
  m_rollOff      = ROLLOFF_AUTO;
  m_streamId     = 0;
  Parse(strParameters.c_str());
}

int cDvbTransponderParameters::PrintParameter(char *p, char name, int value) const
{
  return value >= 0 && value != 999 ? sprintf(p, "%c%d", name, value) : 0;
}

cString cDvbTransponderParameters::ToString(char type) const
{
  string dummy;
  char buffer[64];
  char *q = buffer;
  *q = 0;
  if (ST("  S *", type, m_system)) q += sprintf(q, "%c", m_polarization);
  if (ST("   T*", type, m_system)) q += PrintParameter(q, 'B', DvbParameters::MapToUser(m_bandwidth, BandwidthValues, dummy));
  if (ST(" CST*", type, m_system)) q += PrintParameter(q, 'C', DvbParameters::MapToUser(m_coderateH, CoderateValues, dummy));
  if (ST("   T*", type, m_system)) q += PrintParameter(q, 'D', DvbParameters::MapToUser(m_coderateL, CoderateValues, dummy));
  if (ST("   T*", type, m_system)) q += PrintParameter(q, 'G', DvbParameters::MapToUser(m_guard, GuardValues, dummy));
  if (ST("ACST*", type, m_system)) q += PrintParameter(q, 'I', DvbParameters::MapToUser(m_inversion, InversionValues, dummy));
  if (ST("ACST*", type, m_system)) q += PrintParameter(q, 'M', DvbParameters::MapToUser(m_modulation, ModulationValues, dummy));
  if (ST("  S 2", type, m_system)) q += PrintParameter(q, 'O', DvbParameters::MapToUser(m_rollOff, RollOffValues, dummy));
  if (ST("  ST2", type, m_system)) q += PrintParameter(q, 'P', m_streamId);
  if (ST("  ST*", type, m_system)) q += PrintParameter(q, 'S', DvbParameters::MapToUser(m_system, SystemValuesSat, dummy)); // we only need the numerical value, so Sat or Terr doesn't matter
  if (ST("   T*", type, m_system)) q += PrintParameter(q, 'T', DvbParameters::MapToUser(m_transmission, TransmissionValues, dummy));
  if (ST("   T*", type, m_system)) q += PrintParameter(q, 'Y', DvbParameters::MapToUser(m_hierarchy, HierarchyValues, dummy));
  return buffer;
}

const char *cDvbTransponderParameters::ParseParameter(const char *s, int &value, const tDvbParameterMap *map /* = NULL */)
{
  if (*++s)
  {
    char *p = NULL;
    errno = 0;
    int n = strtol(s, &p, 10);
    if (!errno && p != s)
    {
      value = map ? DvbParameters::MapToDriver(n, map) : n;
      if (value >= 0)
        return p;
    }
  }

  esyslog("ERROR: invalid value for parameter '%c'", *(s - 1));
  return NULL;
}

bool cDvbTransponderParameters::Parse(const char *s)
{
  while (s && *s)
  {
    switch (toupper(*s))
    {
    case 'B': s = ParseParameter(s, m_bandwidth,    BandwidthValues);    break;
    case 'C': s = ParseParameter(s, m_coderateH,    CoderateValues);     break;
    case 'D': s = ParseParameter(s, m_coderateL,    CoderateValues);     break;
    case 'G': s = ParseParameter(s, m_guard,        GuardValues);        break;
    case 'I': s = ParseParameter(s, m_inversion,    InversionValues);    break;
    case 'M': s = ParseParameter(s, m_modulation,   ModulationValues);   break;
    case 'O': s = ParseParameter(s, m_rollOff,      RollOffValues);      break;
    case 'P': s = ParseParameter(s, m_streamId);                         break;
    case 'S': s = ParseParameter(s, m_system,       SystemValuesSat);    break; // we only need the numerical value, so Sat or Terr doesn't matter
    case 'T': s = ParseParameter(s, m_transmission, TransmissionValues); break;
    case 'Y': s = ParseParameter(s, m_hierarchy,    HierarchyValues);    break;

    case 'H': m_polarization = 'H'; s++; break;
    case 'L': m_polarization = 'L'; s++; break;
    case 'R': m_polarization = 'R'; s++; break;
    case 'V': m_polarization = 'V'; s++; break;

    default:
      esyslog("ERROR: unknown parameter key '%c'", *s);
      return false;
    }
  }
  return true;
}

// --- cDvbSourceParam -------------------------------------------------------

cDvbSourceParam::cDvbSourceParam(char source, const string &strDescription)
 : cSourceParam(source, strDescription.c_str()),
   m_param(0),
   m_srate(0)
{
}

void cDvbSourceParam::SetData(const cChannel &channel)
{
  m_srate = channel.Srate();
  m_dtp.Parse(channel.Parameters());
  m_param = 0;
}

void cDvbSourceParam::GetData(cChannel &channel) const
{
  channel.SetTransponderData(channel.Source(), channel.Frequency(), m_srate, m_dtp.ToString(Source()), true);
}

cOsdItem *cDvbSourceParam::GetOsdItem()
{
  char type = Source();
  const tDvbParameterMap *SystemValues = type == 'S' ? SystemValuesSat : SystemValuesTerr;
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
