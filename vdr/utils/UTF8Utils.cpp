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

#include "UTF8Utils.h"
//#include "CharSetConverterVDR.h"
//#include "Tools.h"

using namespace std;

namespace VDR
{

// Mask Test
bool MT(unsigned char s, unsigned char mask, unsigned char value)
{
  return (s & mask) == value;
}

uint32_t cUtf8Utils::SystemToUtf8[128] = { 0 };

unsigned int cUtf8Utils::Utf8CharLen(const string& s)
{
  if (s.empty())
    return 0;
  /*
  if (cCharSetConv::SystemCharacterTable())
    return 1;
  */
  if (s.length() >= 2 && MT(s[0], 0xE0, 0xC0) && MT(s[1], 0xC0, 0x80))
    return 2;
  if (s.length() >= 3 && MT(s[0], 0xF0, 0xE0) && MT(s[1], 0xC0, 0x80) && MT(s[2], 0xC0, 0x80))
    return 3;
  if (s.length() >= 4 && MT(s[0], 0xF8, 0xF0) && MT(s[1], 0xC0, 0x80) && MT(s[2], 0xC0, 0x80) && MT(s[3], 0xC0, 0x80))
    return 4;
  return 1;
}

uint32_t cUtf8Utils::Utf8CharGet(const string& s)
{
  if (s.empty())
    return 0;
  /*
  if (cCharSetConv::SystemCharacterTable())
    return (uchar)s[0] < 128 ? s[0] : SystemToUtf8[(uchar)s[0] - 128];
  */
  switch (Utf8CharLen(s))
  {
    case 2: return ((s[0] & 0x1F) <<  6) |  (s[1] & 0x3F);
    case 3: return ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) <<  6) |  (s[2] & 0x3F);
    case 4: return ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
    default: break;
  }
  return s[0];
}

string cUtf8Utils::Utf8CharSet(uint32_t utf8Symbol)
{
  string s;
  if (utf8Symbol < 0x80 /* || cCharSetConv::SystemCharacterTable() */)
  {
    s.push_back(utf8Symbol);
  }
  if (utf8Symbol < 0x800)
  {
    s.push_back(((utf8Symbol >> 6) & 0x1F) | 0xC0);
    s.push_back((utf8Symbol & 0x3F) | 0x80);
  }
  if (utf8Symbol < 0x10000)
  {
    s.push_back(((utf8Symbol >> 12) & 0x0F) | 0xE0);
    s.push_back(((utf8Symbol >>  6) & 0x3F) | 0x80);
    s.push_back((utf8Symbol & 0x3F) | 0x80);
  }
  if (utf8Symbol < 0x110000)
  {
    s.push_back(((utf8Symbol >> 18) & 0x07) | 0xF0);
    s.push_back(((utf8Symbol >> 12) & 0x3F) | 0x80);
    s.push_back(((utf8Symbol >>  6) & 0x3F) | 0x80);
    s.push_back((utf8Symbol & 0x3F) | 0x80);
  }
  return s;
}

unsigned int cUtf8Utils::Utf8SymChars(const string& s, unsigned int symbols)
{
  if (s.empty())
    return 0;
  /*
  if (cCharSetConv::SystemCharacterTable())
    return symbols;
  */
  unsigned int n = 0;
  while (n < s.length() && symbols)
  {
    n += Utf8CharLen(s.substr(n));
    symbols--;
  }

  return n;
}

unsigned int cUtf8Utils::Utf8StrLen(const string& s)
{
  if (s.empty())
    return 0;
  /*
  if (cCharSetConv::SystemCharacterTable())
    return s.length();
  */
  unsigned int n = 0;
  unsigned int pos = 0;
  while (pos < s.length())
  {
    pos += Utf8CharLen(s.substr(pos));
    n++;
  }
  return n;
}

vector<uint32_t> cUtf8Utils::Utf8ToArray(const string& s)
{
  vector<uint32_t> utf8Symbols;
  if (s.empty())
    return utf8Symbols;

  unsigned int pos = 0;
  while (pos < s.length())
  {
    /*
    if (cCharSetConv::SystemCharacterTable())
    {
      utf8Symbols.push_back((uchar)s[pos]);
      pos++;
    }
    else
    */
    {
      string nextChar = s.substr(pos);
      utf8Symbols.push_back(Utf8CharGet(nextChar));
      pos += Utf8CharLen(nextChar);
    }
  }
  return utf8Symbols;
}

string cUtf8Utils::Utf8FromArray(const vector<uint32_t>& utf8Symbols)
{
  string s;
  for (vector<uint32_t>::const_iterator it = utf8Symbols.begin(); it != utf8Symbols.end(); ++it)
  {
    /*
    if (cCharSetConv::SystemCharacterTable())
      s.push_back(*it);
    else
    */
      s += Utf8CharSet(*it);
  }
  return s;
}

}
