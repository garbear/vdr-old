#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "xbmc_addon_dll.h"
#include "xbmc_service_types.h"

#ifdef __cplusplus
extern "C" {
#endif

  const char* GetServiceAPIVersion(void);

  /*!
   * Get the XBMC_SERVICE_MIN_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_SERVICE_MIN_API_VERSION that was used to compile this add-on.
   * @remarks Valid implementation required.
   */
  const char* GetMininumServiceAPIVersion(void);

  bool StartService(void);

  bool StopService(void);

  /*!
   * Called by XBMC to assign the function pointers of this add-on to pClient.
   * @param pClient The struct to assign the function pointers to.
   */
  void __declspec(dllexport) get_addon(ServiceDLL* pClient)
  {
    pClient->GetServiceAPIVersion        = GetServiceAPIVersion;
    pClient->GetMininumServiceAPIVersion = GetMininumServiceAPIVersion;
    pClient->StartService                = StartService;
    pClient->StopService                 = StopService;
  };

#ifdef __cplusplus
};
#endif
