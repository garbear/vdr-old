/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "Filesystem.h"

#ifdef TARGET_XBMC
//TODO
#else
#include "native/Directory.h"
#include "native/File.h"
#endif

CFilesystem::~CFilesystem(void)
{
  //TODO track open files
}

CFilesystem& CFilesystem::Get(void)
{
  static CFilesystem _instance;
  return _instance;
}

bool CFilesystem::CreateDirectory(const std::string& strPath)
{
#ifdef TARGET_XBMC
  //TODO
#else
  return cDirectory::Create(strPath);
#endif
}

bool CFilesystem::FileExists(const std::string& strPath)
{
#ifdef TARGET_XBMC
  //TODO
#else
  cFile tmp;
  return tmp.Open(strPath);
#endif
}
