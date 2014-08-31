/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      TODO copyright of these comes from elsewhere
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

#if defined(TARGET_ANDROID)

#include "sort.h"
#include "strchrnul.h"
#include <linux/byteorder/swab.h>

#define POSIX_FADV_RANDOM   1
#define POSIX_FADV_DONTNEED 1
#define POSIX_FADV_WILLNEED 1
#define posix_fadvise(a,b,c,d) (-1)


#ifndef DEFFILEMODE
#define DEFFILEMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)/* 0666*/
#endif

// Android does not declare mkdtemp even though it's implemented.
extern "C" char *mkdtemp(char *path);

static __inline__ __attribute_const__ __u16 __fswab16(__u16 x)
{
  return __arch__swab16(x);
}

static __inline__ __u16 __swab16p(__u16 *x)
{
  return __arch__swab16p(x);
}

static __inline__ void __swab16s(__u16 *addr)
{
  __arch__swab16s(addr);
}

static __inline__ __attribute_const__ __u32 __fswab32(__u32 x)
{
  return __arch__swab32(x);
}

static __inline__ __u32 __swab32p(__u32 *x)
{
  return __arch__swab32p(x);
}

static __inline__ void __swab32s(__u32 *addr)
{
  __arch__swab32s(addr);
}

#ifdef __BYTEORDER_HAS_U64__
static __inline__ __attribute_const__ __u64 __fswab64(__u64 x)
{
# ifdef __SWAB_64_THRU_32__
  __u32 h = x >> 32;
  __u32 l = x & ((1ULL<<32)-1);
  return (((__u64)__swab32(l)) << 32) | ((__u64)(__swab32(h)));
# else
  return __arch__swab64(x);
# endif
}

static __inline__ __u64 __swab64p(__u64 *x)
{
  return __arch__swab64p(x);
}

static __inline__ void __swab64s(__u64 *addr)
{
  __arch__swab64s(addr);
}
#endif /* __BYTEORDER_HAS_U64__ */

#endif
