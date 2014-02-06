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

#include "Source.h"
#include "utils/Tools.h" // for logging

#include <stdio.h>
#include <stdlib.h>

using namespace std;

cSource::cSource()
 : m_code(stNone)
{
}

cSource::cSource(char source, const string &strDescription)
 : m_code(((int)source) << 24),
   m_strDescription(strDescription)
{
}

bool cSource::Parse(const string &str)
{
  char *codeBuf = NULL;
  char *descriptionBuf = NULL;
  if (2 == sscanf(str.c_str(), "%a[^ ] %a[^\n]", &codeBuf, &descriptionBuf))
  {
    m_code = FromString(codeBuf);
    m_strDescription = descriptionBuf;
  }
  free(codeBuf);
  free(descriptionBuf);
  return m_code != stNone && !m_strDescription.empty();
}

string cSource::ToString(int code)
{
  char buffer[16];
  char *q = buffer;
  *q++ = (code & st_Mask) >> 24;
  int n = (code & st_Pos);
  if (n > 0x00007FFF)
    n |= 0xFFFF0000;
  if (n)
  {
    // can't simply use "%g" here since the silly 'locale' messes up the decimal point
    q += snprintf(q, sizeof(buffer) - 2, "%u.%u", abs(n) / 10, abs(n) % 10);
    *q++ = (n < 0) ? 'E' : 'W';
  }
  *q = 0;
  return buffer;
}

int cSource::FromString(const string &str)
{
  if (!str.empty())
  {
    if ('A' <= str[0] && str[0] <= 'Z')
    {
      int code = ((int)str[0]) << 24;
      if (code == stSat)
      {
        // TODO: Switch to a FSM
        int pos = 0;
        bool dot = false;
        bool neg = false;
        for (size_t i = 0; i < str.size(); i++)
        {
          switch (str[i])
          {
          case '0' ... '9':
            pos *= 10;
            pos += str[i] - '0';
            break;
          case '.':
            dot = true;
            break;
          case 'E':
            neg = true;
            // fall through to 'W'
          case 'W':
            if (!dot)
              pos *= 10;
            break;
          default:
            esyslog("ERROR: unknown source character '%c' in '%s'", str[i], str.c_str());
            return stNone;
          }
        }
        if (neg)
          pos = -pos;
        code |= (pos & st_Pos);
      }
      return code;
    }
    else
      esyslog("ERROR: unknown source key '%c' at beginning of '%s'", str[0], str.c_str());
  }
  return stNone;
}

int cSource::FromData(eSourceType sourceType, int position /* = 0 */, eSatelliteDirection dir /* = sdWest */)
{
  int code = sourceType;
  if (sourceType == stSat)
  {
    if (dir == sdEast)
      position = -position;
    code |= (position & st_Pos);;
  }
  return code;
}
