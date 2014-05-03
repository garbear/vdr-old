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

#include "utils/log/Log.h" // for LOG_ERROR

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))
#endif

#define KILOBYTE(n) ((n) * 1024)
#define MEGABYTE(n) ((n) * 1024LL * 1024LL)

#define MALLOC(type, size)  (type *)malloc(sizeof(type) * (size))

#ifndef PRId64
#define PRId64       "I64d"
#endif

#define CHECK(s) { if ((s) < 0) LOG_ERROR; } // used for 'ioctl()' calls
#define FATALERRNO (errno && errno != EAGAIN && errno != EINTR)

#define BCDCHARTOINT(x) (10 * ((x & 0xF0) >> 4) + (x & 0xF))

#if defined(TARGET_ANDROID)
#include <time64.h>
#define vdr_time_gm(x) timegm64(x)
#define vdr_realpath(x) realpath(x, NULL)
#else
#include <time.h>
#define vdr_time_gm(x)  timegm(x)
#define vdr_realpath(x) canonicalize_file_name(x)
#endif

namespace VDR
{

#if defined(TARGET_ANDROID)
typedef time64_t vdr_time_t;
#else
typedef time_t vdr_time_t;
#endif

inline int BCD2INT(int x)
{
  return ((1000000 * BCDCHARTOINT((x >> 24) & 0xFF)) +
            (10000 * BCDCHARTOINT((x >> 16) & 0xFF)) +
              (100 * BCDCHARTOINT((x >>  8) & 0xFF)) +
                     BCDCHARTOINT( x        & 0xFF));
}

template<class T> inline void DELETENULL(T *&p) { T *q = p; p = NULL; delete q; }

template<class T> inline T constrain(T v, T l, T h) { return v < l ? l : v > h ? h : v; }

// Unfortunately there are no platform independent macros for unaligned
// access, so we do it this way:

template<class T> inline T get_unaligned(T *p)
{
  struct s { T v; } __attribute__((packed));
  return ((s *)p)->v;
}

template<class T> inline void put_unaligned(unsigned int v, T* p)
{
  struct s { T v; } __attribute__((packed));
  ((s *)p)->v = v;
}

// Comparing doubles for equality is unsafe, but unfortunately we can't
// overwrite operator==(double, double), so this will have to do:

inline bool DoubleEqual(double a, double b)
{
  return fabs(a - b) <= DBL_EPSILON;
}

}
