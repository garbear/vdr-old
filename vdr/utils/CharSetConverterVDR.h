/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Contains code Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "Types.h"
#include <iconv.h>
#include <stddef.h>

namespace VDR
{
class cCharSetConv
{
public:
  /*!
   * \brief Sets up a character set converter to convert from FromCode to ToCode
   *
   * If FromCode is NULL, the previously set systemCharacterTable is used (or
   * "UTF-8" if no systemCharacterTable has been set). If ToCode is NULL,
   * "UTF-8" is used.
   */
  cCharSetConv(const char *FromCode = NULL, const char *ToCode = NULL);
  ~cCharSetConv();

  /*!
   * \brief Converts the given Text from FromCode to ToCode (as set in the constructor)
   *
   * If To is given, it is used to copy at most ToLength bytes of the result
   * (including the terminating 0) into that buffer. If To is not given, the
   * result is copied into a dynamically allocated buffer and is valid as long
   * as this object lives, or until the next call to Convert(). The return
   * value always points to the result if the conversion was successful (even
   * if a fixed size To buffer was given and the result didn't fit into it).
   * If the string could not be converted, the result points to the original
   * From string.
   */
  const char *Convert(const char *From, char *To = NULL, size_t ToLength = 0);

  static const char *SystemCharacterTable(void) { return systemCharacterTable; }
  static void SetSystemCharacterTable(const char *CharacterTable);

private:
  iconv_t cd;
  char *result;
  size_t length;
  static char *systemCharacterTable;
};
}
