/*
 * config.c: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.c 2.38 2013/03/18 08:57:50 kls Exp $
 */

#include "Config.h"
#include <ctype.h>
#include <stdlib.h>
#include "devices/Device.h"
#include "recordings/Recording.h"

#include "vdr/utils/UTF8Utils.h"

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
