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

#include <stdint.h>
#include <string>
#include <vector>

namespace VDR
{
// When handling strings that might contain UTF-8 characters, it may be necessary
// to process a "symbol" that consists of several actual character bytes. The
// following functions allow transparently accessing a "char *" string without
// having to worry about what character set is actually used.

class cUtf8Utils
{
public:
  /*!
   * \brief Returns the number of character bytes at the beginning of the given
   *        string that form a UTF-8 symbol.
   */
  static unsigned int Utf8CharLen(const std::string& s);

  /*!
   * \brief Returns the UTF-8 symbol at the beginning of the given string.
   */
  static uint32_t Utf8CharGet(const std::string& s);

  /*!
   * \brief Converts the given UTF-8 symbol to a string
   */
  static std::string Utf8CharSet(uint32_t utf8Symbol);

  /*!
   * \brief Returns the number of character bytes at the beginning of the given
   *        string that form at most the given number of UTF-8 symbols.
   */
  static unsigned int Utf8SymChars(const std::string& s, unsigned int symbols);

  /*!
   * \brief Returns the number of UTF-8 symbols formed by the given string of
   *        character bytes.
   */
  static unsigned int Utf8StrLen(const std::string& s);

  /*!
   * \brief Converts the given character bytes (including the terminating 0)
   *        into an array of UTF-8 symbols of the given Size
   */
  static std::vector<uint32_t> Utf8ToArray(const std::string& s);

  /*!
   * \brief Converts the given array of UTF-8 symbols into a sequence of
   *        character bytes of at most Size length
   *
   * If Max is given, only that many symbols will be converted. The resulting
   * string is always zero-terminated if Size is big enough.
   */
  static std::string Utf8FromArray(const std::vector<uint32_t>& utf8Symbols);

  static uint32_t SystemToUtf8[128];
};
}
