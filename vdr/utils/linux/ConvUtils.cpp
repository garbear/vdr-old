/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "ConvUtils.h"

#ifdef TARGET_POSIX

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

LONGLONG Int32x32To64(LONG Multiplier, LONG Multiplicand)
{
  LONGLONG result = Multiplier;
  result *= Multiplicand;
  return result;
}

DWORD GetLastError()
{
  return errno;
}

VOID SetLastError(DWORD dwErrCode)
{
  errno = dwErrCode;
}

#endif // TARGET_POSIX