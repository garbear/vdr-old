/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

//-----------------------------------------------------------------------
//
//  File:      StringUtils.h
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to support J O'Leary's CStdString class by kraqh3d
//
//------------------------------------------------------------------------

#include <vector>
#include <stdarg.h>
#include <stdint.h>
#include <string>

//#include "XBDateTime.h"

class StringUtils
{
public:
  /*! \brief Get a formatted string similar to sprintf

  Beware that this does not support directly passing in
  std::string objects. You need to call c_str() to pass
  the const char* buffer representing the value of the
  std::string object.

  \param fmt Format of the resulting string
  \param ... variable number of value type arguments
  \return Formatted string
  */
  static std::string Format(const char *fmt, ...);
  static std::string FormatV(const char *fmt, va_list args);
  static std::wstring Format(const wchar_t *fmt, ...);
  static std::wstring FormatV(const wchar_t *fmt, va_list args);
  static void ToUpper(std::string &str);
  static void ToUpper(std::wstring &str);
  static void ToLower(std::string &str);
  static void ToLower(std::wstring &str);
  static bool EqualsNoCase(const std::string &str1, const std::string &str2);
  static bool EqualsNoCase(const std::string &str1, const char *s2);
  static bool EqualsNoCase(const char *s1, const char *s2);
  static int  CompareNoCase(const std::string &str1, const std::string &str2);
  static int  CompareNoCase(const char *s1, const char *s2);
  static std::string Left(const std::string &str, size_t count);
  static std::string Mid(const std::string &str, size_t first, size_t count = std::string::npos);
  static std::string Right(const std::string &str, size_t count);
  static std::string& Trim(std::string &str);
  static std::string& TrimLeft(std::string &str);
  static std::string& TrimLeft(std::string &str, const std::string &chars);
  static std::string& TrimRight(std::string &str);
  static std::string& TrimRight(std::string &str, const std::string &chars);
  static std::string& RemoveDuplicatedSpacesAndTabs(std::string& str);
  static int Replace(std::string &str, char oldChar, char newChar);
  static int Replace(std::string &str, const std::string &oldStr, const std::string &newStr);
  static int Replace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr);
  static bool StartsWith(const std::string &str1, const std::string &str2);
  static bool StartsWith(const std::string &str1, const char *s2);
  static bool StartsWith(const char *s1, const char *s2);
  static bool StartsWithNoCase(const std::string &str1, const std::string &str2);
  static bool StartsWithNoCase(const std::string &str1, const char *s2);
  static bool StartsWithNoCase(const char *s1, const char *s2);
  static bool EndsWith(const std::string &str1, const std::string &str2);
  static bool EndsWith(const std::string &str1, const char *s2);
  static bool EndsWithNoCase(const std::string &str1, const std::string &str2);
  static bool EndsWithNoCase(const std::string &str1, const char *s2);

  static std::string Join(const std::vector<std::string> &strings, const std::string &delimiter);
  static int Split(const std::string &input, const std::string &delimiter, std::vector<std::string> &results, unsigned int iMaxStrings = 0);
  static int FindNumber(const std::string &strInput, const std::string &strFind);
  static int64_t AlphaNumericCompare(const wchar_t *left, const wchar_t *right);
  static long TimeStringToSeconds(const std::string &timeString);
  static void RemoveCRLF(std::string &strLine);

  /*! \brief utf8 version of strlen - skips any non-starting bytes in the count, thus returning the number of utf8 characters
   \param s c-string to find the length of.
   \return the number of utf8 characters in the string.
   */
  static size_t utf8_strlen(const char *s);

  /*! \brief check whether a string is a natural number.
   Matches [ \t]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is a natural number, false otherwise.
   */
  static bool IsNaturalNumber(const std::string &str);

  /*! \brief check whether a string is an integer.
   Matches [ \t]*[\-]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is an integer, false otherwise.
   */
  static bool IsInteger(const std::string &str);
  static std::string SizeToString(int64_t size);
  static const std::string Empty;
  static size_t FindWords(const char *str, const char *wordLowerCase);
  static int FindEndBracket(const std::string &str, char opener, char closer, int startPos = 0);
  static int DateStringToYYYYMMDD(const std::string &dateString);
  static void WordToDigits(std::string &word);
  //static double CompareFuzzy(const CStdString &left, const CStdString &right);
  //static int FindBestMatch(const CStdString &str, const CStdStringArray &strings, double &matchscore);
  static bool ContainsKeyword(const std::string &str, const std::vector<std::string> &keywords);

  /*! \brief Escapes the given string to be able to be used as a parameter.

   Escapes backslashes and double-quotes with an additional backslash and
   adds double-quotes around the whole string.

   \param param String to escape/paramify
   \return Escaped/Paramified string
   */
  static std::string Paramify(const std::string &param);
  static void Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters);

  /*! \brief Convert a string to an integer.
   \param str The string
   \param intVal The value, untouched if IntVal() returns false
   \param remainder The string following the parsed integer, or empty if the integer consumed the entire string
   \return True on success, or false on error (not an int, overflow, etc)
   */
  static bool IntVal(const std::string &str, long &intVal);
  static bool IntVal(const std::string &str, long &intVal, std::string &remainder);
};
