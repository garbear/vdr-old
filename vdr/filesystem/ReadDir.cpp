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

#include "ReadDir.h"

#include <dirent.h>
#include <string.h>

cReadDir::cReadDir(const char *Directory)
 : result(NULL)
{
  directory = opendir(Directory);
}

cReadDir::~cReadDir()
{
  if (directory)
    closedir(directory);
}

struct dirent *cReadDir::Next()
{
  if (directory)
  {
    while (readdir_r(directory, &u.d, &result) == 0 && result)
    {
      if (strcmp(result->d_name, ".") && strcmp(result->d_name, ".."))
        return result;
    }
  }
  return NULL;
}