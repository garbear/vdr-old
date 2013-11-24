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

#include "DVBDeviceProbe.h"
#include "utils/StringUtils.h"
#include "filesystem/ReadDir.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/stat.h>

using namespace std;

cList<cDvbDeviceProbe> DvbDeviceProbes;

cDvbDeviceProbe::cDvbDeviceProbe()
{
  DvbDeviceProbes.Add(this);
}

cDvbDeviceProbe::~cDvbDeviceProbe()
{
  DvbDeviceProbes.Del(this, false);
}

uint32_t cDvbDeviceProbe::GetSubsystemId(int adapter, int frontend)
{
  uint32_t subsystemId = 0;
  string fileName = StringUtils::Format("/dev/dvb/adapter%d/frontend%d", adapter, frontend);
  struct stat st;
  if (stat(fileName.c_str(), &st) == 0)
  {
    cReadDir d("/sys/class/dvb");
    if (d.Ok())
    {
      struct dirent *e;
      while ((e = d.Next()) != NULL)
      {
        if (strstr(e->d_name, "frontend"))
        {
          fileName = StringUtils::Format("/sys/class/dvb/%s/dev", e->d_name);
          if (FILE *f = fopen(fileName.c_str(), "r"))
          {
            cReadLine readLine;
            char *s = readLine.Read(f);
            fclose(f);
            unsigned int major;
            unsigned int minor;
            if (s && 2 == sscanf(s, "%u:%u", &major, &minor))
            {
              if (((major << 8) | minor) == st.st_rdev)
              {
                fileName = StringUtils::Format("/sys/class/dvb/%s/device/subsystem_vendor", e->d_name);
                if ((f = fopen(fileName.c_str(), "r")) != NULL)
                {
                  if (char *s = readLine.Read(f))
                    subsystemId = strtoul(s, NULL, 0) << 16;
                  fclose(f);
                }
                fileName = StringUtils::Format("/sys/class/dvb/%s/device/subsystem_device", e->d_name);
                if ((f = fopen(fileName.c_str(), "r")) != NULL)
                {
                  if (char *s = readLine.Read(f))
                    subsystemId |= strtoul(s, NULL, 0);
                  fclose(f);
                }
                break;
              }
            }
          }
        }
      }
    }
  }
  return subsystemId;
}
