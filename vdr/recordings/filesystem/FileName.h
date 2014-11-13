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

#include "utils/CommonIncludes.h" // off_t problems on x86
#include "filesystem/File.h"

#include <stdint.h>
#include <stdio.h>
#include <string>

namespace VDR
{
class cFileName
{
public:
  cFileName(const std::string& strFileName, bool Record, bool Blocking = false, bool IsPesRecording = false);
  ~cFileName();
  std::string Name(void) { return m_strFileName + m_strFileOffset; }
  uint16_t Number(void) { return m_iFileNumber; }
  bool GetLastPatPmtVersions(int &PatVersion, int &PmtVersion);
  CFile* Open(void);
  void Close(void);
  CFile *SetOffset(int Number, off_t Offset = 0); // yes, Number is int for easier internal calculating
  CFile *NextFile(void);

private:
  CFile       m_file;
  uint16_t    m_iFileNumber;
  std::string m_strFileName;
  std::string m_strFileOffset;
  bool        m_bRecord;
  bool        m_bBlocking;
  bool        m_bIsPesRecording;
};
}
