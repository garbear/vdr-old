#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

/* current service API version */
#define XBMC_SERVICE_API_VERSION "0.1.0"

/* min. service API version */
#define XBMC_SERVICE_MIN_API_VERSION "0.1.0"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#if !defined(__stat64) && defined(TARGET_POSIX)
  #if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
    #define __stat64 stat
  #else
    #define __stat64 stat64
  #endif
#endif

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

#ifdef __cplusplus
extern "C" {
#endif
  typedef struct SRV_PROPS
  {
    const char* strUserPath;           /*!< @brief path to the user profile */
    const char* strClientPath;         /*!< @brief path to this add-on */
  } SRV_PROPS;

  typedef struct ServiceDLL
  {
    const char* (__cdecl* GetServiceAPIVersion)(void);
    const char* (__cdecl* GetMininumServiceAPIVersion)(void);
    bool        (__cdecl* StartService)();
    bool        (__cdecl* StopService)();
  } ServiceDLL;
}
