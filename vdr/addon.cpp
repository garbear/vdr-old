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

#include "VDRDaemon.h"

#include "xbmc/libXBMC_addon.h"
#include "xbmc/xbmc_addon_types.h"
#include "xbmc/xbmc_content_types.h"
#include "xbmc/xbmc_service_dll.h"
#include "utils/log/ILog.h"

#include <string>

using namespace ADDON;
using namespace std;

namespace VDR
{

class CLogXBMC : public ILog
{
public:
  virtual ~CLogXBMC(void) {}

  void Log(sys_log_level_t level, const char* logline);
  sys_log_type_t Type(void) const { return SYS_LOG_TYPE_ADDON; }
};

CHelper_libXBMC_addon*   XBMC = NULL;
string         g_strUserPath    = "";
string         g_strClientPath  = "";
ADDON_STATUS   m_CurStatus      = ADDON_STATUS_UNKNOWN;

cVDRDaemon     vdr;

void CLogXBMC::Log(sys_log_level_t level, const char* logline)
{
  addon_log_t xbmcLogLevel;

  switch (level)
  {
  case SYS_LOG_ERROR:
    xbmcLogLevel = LOG_ERROR;
    break;
  case SYS_LOG_INFO:
    xbmcLogLevel = LOG_INFO;
    break;
  case SYS_LOG_DEBUG:
    xbmcLogLevel = LOG_DEBUG;
    break;
  default:
    return;
  }

  XBMC->Log(xbmcLogLevel, logline);
}

extern "C" {

void ADDON_ReadSettings(void)
{
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  CONTENT_PROPERTIES* cprops = (CONTENT_PROPERTIES*)props;

  m_CurStatus = ADDON_STATUS_UNKNOWN;
  try
  {
    XBMC = new CHelper_libXBMC_addon;
    if (!XBMC || !XBMC->RegisterMe(hdl))
      throw ADDON_STATUS_PERMANENT_FAILURE;

    g_strUserPath   = cprops->strUserPath;
    g_strClientPath = cprops->strClientPath;

    ADDON_ReadSettings();

    CLog::Get().SetPipe(new CLogXBMC);
    if (!vdr.Init())
      throw ADDON_STATUS_UNKNOWN;

    m_CurStatus = ADDON_STATUS_OK;

  }
  catch (const ADDON_STATUS &status)
  {
    delete XBMC;
    XBMC = NULL;
    m_CurStatus = status;
    return status;
  }

  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  vdr.Stop();
  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting *** sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  if (!settingName || !settingValue)
    return ADDON_STATUS_UNKNOWN;

  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
  vdr.Stop();
}

void ADDON_FreeSettings()
{
}

}

} // extern "C"
