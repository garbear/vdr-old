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

#include "Config.h"
#include "devices/Device.h"
#include "recordings/Recording.h"
#include "utils/CommonMacros.h"
#include "utils/UTF8Utils.h"

#include <ctype.h>
#include <stdlib.h>

namespace VDR
{

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

#define ChkDoublePlausibility(Variable, Default) { if (Variable < 0.00001) Variable = Default; }

// --- cSatCableNumbers ------------------------------------------------------

cSatCableNumbers::cSatCableNumbers(int Size, const std::string& bindings)
{
  size = Size;
  array = MALLOC(int, size);
  FromString(bindings.c_str());
}

cSatCableNumbers::~cSatCableNumbers()
{
  free(array);
}

bool cSatCableNumbers::FromString(const char *s)
{
  char *t;
  int i = 0;
  const char *p = s;
  while (p && *p) {
        int n = strtol(p, &t, 10);
        if (t != p) {
           if (i < size)
              array[i++] = n;
           else {
              esyslog("ERROR: too many sat cable numbers in '%s'", s);
              return false;
              }
           }
        else {
           esyslog("ERROR: invalid sat cable number in '%s'", s);
           return false;
           }
        p = skipspace(t);
        }
  for ( ; i < size; i++)
      array[i] = 0;
  return true;
}

std::string cSatCableNumbers::ToString(void)
{
  std::string s("");
  for (int i = 0; i < size; i++)
    s = StringUtils::Format("%s%d ", s.c_str(), array[i]);
  return s;
}

int cSatCableNumbers::FirstDeviceIndex(int DeviceIndex) const
{
  if (0 <= DeviceIndex && DeviceIndex < size) {
     if (int CableNr = array[DeviceIndex]) {
        for (int i = 0; i < size; i++) {
            if (i < DeviceIndex && array[i] == CableNr)
               return i;
            }
        }
     }
  return -1;
}

}
