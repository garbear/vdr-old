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
#include <stddef.h>
#include <sys/types.h>

// When allocating buffer space, make sure we reserve enough space to hold
// a string in UTF-8 representation:

#define Utf8BufSize(s) ((s) * 4)

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
  static int Utf8CharLen(const char *s);

  /*!
   * \brief Returns the UTF-8 symbol at the beginning of the given string
   *
   * Length can be given from a previous call to Utf8CharLen() to avoid
   * calculating it again. If no Length is given, Utf8CharLen() will be called.
   */
  static uint Utf8CharGet(const char *s, int Length = 0);

  /*!
   * \brief Converts the given UTF-8 symbol to a sequence of character bytes
   *        and copies them to the given string.
   * \return The number of bytes written. If no string is given, only the
   *         number of bytes is returned and nothing is copied.
   */
  static int Utf8CharSet(uint c, char *s = NULL);

  /*!
   * \brief Returns the number of character bytes at the beginning of the given
   *        string that form at most the given number of UTF-8 symbols.
   */
  static int Utf8SymChars(const char *s, int Symbols);

  /*!
   * \brief Returns the number of UTF-8 symbols formed by the given string of
   *        character bytes.
   */
  static unsigned int Utf8StrLen(const char *s);

  /*!
   * \brief Copies at most n character bytes from Src to Dest, making sure that
   *        the resulting copy ends with a complete UTF-8 symbol. The copy is
   *        guaranteed to be zero terminated.
   * \return A pointer to Dest
   */
  static char *Utf8Strn0Cpy(char *Dest, const char *Src, int n);

  /*!
   * \brief Converts the given character bytes (including the terminating 0)
   *        into an array of UTF-8 symbols of the given Size
   * \return The number of symbols in the array (without the terminating 0)
   */
  static unsigned int Utf8ToArray(const char *s, uint *a, int Size);

  /*!
   * \brief Converts the given array of UTF-8 symbols (including the terminating
   *        0) into a sequence of character bytes of at most Size length
   * \return The number of character bytes written (without the terminating 0)
   *
   * If Max is given, only that many symbols will be converted. The resulting
   * string is always zero-terminated if Size is big enough.
   */
  static int Utf8FromArray(const uint *a, char *s, int Size, int Max = -1);

  static uint SystemToUtf8[128];
};
