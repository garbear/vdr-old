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

#include "filesystem/File.h"
#include "utils/List.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"

#include <stdlib.h>

namespace VDR
{

// VDR's own version number:
#define VDRVERSION  "2.0.4-2"
#define VDRVERSNUM   20004  // Version * 10000 + Major * 100 + Minor

#define MAXPRIORITY       99
#define MINPRIORITY       (-MAXPRIORITY)
#define LIVEPRIORITY      0                  // priority used when selecting a device for live viewing
#define TRANSFERPRIORITY  (LIVEPRIORITY - 1) // priority used for actual local Transfer Mode
#define IDLEPRIORITY      (MINPRIORITY - 1)  // priority of an idle device
#define MAXLIFETIME       99
#define DEFINSTRECTIME    180 // default instant recording time (minutes)

#define TIMERMACRO_TITLE    "TITLE"
#define TIMERMACRO_EPISODE  "EPISODE"

#define MaxFileName NAME_MAX // obsolete - use NAME_MAX directly instead!
#define MaxSkinName 16
#define MaxThemeName 16

// Error flags
#define ERROR_PES_GENERAL   0x01
#define ERROR_PES_SCRAMBLE  0x02
#define ERROR_PES_STARTCODE 0x04
#define ERROR_DEMUX_NODATA  0x10

// TODO: Define these in a better place
#ifndef INSTALL_PATH
#define INSTALL_PATH    "/usr/share/vdr"
#endif

#ifndef BIN_INSTALL_PATH
#define BIN_INSTALL_PATH "/usr/lib/vdr"
#endif

#define VDR_HOME_ENV_VARIABLE  "VDR_HOME"

// Basically VDR works according to the DVB standard, but there are countries/providers
// that use other standards, which in some details deviate from the DVB standard.
// This makes it necessary to handle things differently in some areas, depending on
// which standard is actually used. The following macros are used to distinguish
// these cases (make sure to adjust cMenuSetupDVB::standardComplianceTexts accordingly
// when adding a new standard):

#define STANDARD_DVB       0
#define STANDARD_ANSISCTE  1

typedef uint32_t in_addr_t; //XXX from /usr/include/netinet/in.h (apparently this is not defined on systems with glibc < 2.2)

template<class T> class cConfig : public cList<T> {
private:
  char *fileName;
  bool allowComments;
  void Clear(void)
  {
    free(fileName);
    fileName = NULL;
    cList<T>::Clear();
  }
public:
  cConfig(void)
  {
    fileName = NULL;
    allowComments = true;
  }

  virtual ~cConfig()
  {
    free(fileName);
  }

  const char *FileName(void)
  {
    return fileName;
  }

  bool Load(const char *FileName = NULL, bool AllowComments = false, bool MustExist = false)
  {
    bool result = !MustExist;
    cConfig<T>::Clear();

    if (FileName)
    {
      free(fileName);
      fileName = strdup(FileName);
      allowComments = AllowComments;
    }

    isyslog("loading %s", fileName);
    CFile file;
    if (file.Open(fileName))
    {
      std::string strLine;
      dsyslog("opened %s", fileName);
      result = true;
      unsigned int line = 0; // For error logging
      while (file.ReadLine(strLine))
      {
        // Strip comments and spaces
        if (allowComments)
        {
          size_t comment_pos = strLine.find('#');
          strLine = strLine.substr(0, comment_pos);
        }

        StringUtils::Trim(strLine);

        if (strLine.empty())
          continue;

        T *l = new T;
        if (l->Parse(strLine.c_str()))
        {
          this->Add(l);
        }
        else
        {
          esyslog("ERROR: error in %s, line %d", fileName, line);
          delete l;
          result = false;
        }

        line++;
      }
    }
    else
    {
      LOG_ERROR_STR(fileName);
      result = false;
    }

    if (!result)
       fprintf(stderr, "vdr: error while reading '%s'\n", fileName);
    return result;
  }
};

}
