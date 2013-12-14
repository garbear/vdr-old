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
#include "CharSetConverter.h"
#include "Tools.h"

// Mask Test
#define MT(s, m, v) ((*(s) & (m)) == (v))

uint cUtf8Utils::SystemToUtf8[128] = { 0 };

int cUtf8Utils::Utf8CharLen(const char *s)
{
  if (cCharSetConv::SystemCharacterTable())
    return 1;
  if (MT(s, 0xE0, 0xC0) && MT(s + 1, 0xC0, 0x80))
    return 2;
  if (MT(s, 0xF0, 0xE0) && MT(s + 1, 0xC0, 0x80) && MT(s + 2, 0xC0, 0x80))
    return 3;
  if (MT(s, 0xF8, 0xF0) && MT(s + 1, 0xC0, 0x80) && MT(s + 2, 0xC0, 0x80) && MT(s + 3, 0xC0, 0x80))
    return 4;
  return 1;
}

uint cUtf8Utils::Utf8CharGet(const char *s, int Length)
{
  if (cCharSetConv::SystemCharacterTable())
    return (uchar)*s < 128 ? *s : SystemToUtf8[(uchar)*s - 128];
  if (!Length)
    Length = Utf8CharLen(s);
  switch (Length)
  {
    case 2: return ((*s & 0x1F) <<  6) |  (*(s + 1) & 0x3F);
    case 3: return ((*s & 0x0F) << 12) | ((*(s + 1) & 0x3F) <<  6) |  (*(s + 2) & 0x3F);
    case 4: return ((*s & 0x07) << 18) | ((*(s + 1) & 0x3F) << 12) | ((*(s + 2) & 0x3F) << 6) | (*(s + 3) & 0x3F);
    default: break;
  }
  return *s;
}

int cUtf8Utils::Utf8CharSet(uint c, char *s)
{
  if (c < 0x80 || cCharSetConv::SystemCharacterTable())
  {
    if (s)
      *s = c;
    return 1;
  }
  if (c < 0x800)
  {
    if (s)
    {
      *s++ = ((c >> 6) & 0x1F) | 0xC0;
      *s   = (c & 0x3F) | 0x80;
    }
    return 2;
  }
  if (c < 0x10000)
  {
    if (s)
    {
      *s++ = ((c >> 12) & 0x0F) | 0xE0;
      *s++ = ((c >>  6) & 0x3F) | 0x80;
      *s   = (c & 0x3F) | 0x80;
    }
    return 3;
  }
  if (c < 0x110000)
  {
    if (s)
    {
      *s++ = ((c >> 18) & 0x07) | 0xF0;
      *s++ = ((c >> 12) & 0x3F) | 0x80;
      *s++ = ((c >>  6) & 0x3F) | 0x80;
      *s   = (c & 0x3F) | 0x80;
    }
    return 4;
  }
  return 0; // can't convert to UTF-8
}

int cUtf8Utils::Utf8SymChars(const char *s, int Symbols)
{
  if (cCharSetConv::SystemCharacterTable())
    return Symbols;
  int n = 0;
  while (*s && Symbols--)
  {
    int sl = Utf8CharLen(s);
    s += sl;
    n += sl;
  }
  return n;
}

int cUtf8Utils::Utf8StrLen(const char *s)
{
  if (cCharSetConv::SystemCharacterTable())
    return strlen(s);
  int n = 0;
  while (*s)
  {
    s += Utf8CharLen(s);
    n++;
  }
  return n;
}

char *cUtf8Utils::Utf8Strn0Cpy(char *Dest, const char *Src, int n)
{
  if (cCharSetConv::SystemCharacterTable())
    return strn0cpy(Dest, Src, n);
  char *d = Dest;
  while (*Src)
  {
    int sl = Utf8CharLen(Src);
    n -= sl;
    if (n > 0)
    {
      while (sl--)
        *d++ = *Src++;
    }
    else
      break;
  }
  *d = 0;
  return Dest;
}

int cUtf8Utils::Utf8ToArray(const char *s, uint *a, int Size)
{
  int n = 0;
  while (*s && --Size > 0)
  {
    if (cCharSetConv::SystemCharacterTable())
      *a++ = (uchar)(*s++);
    else
    {
      int sl = Utf8CharLen(s);
      *a++ = Utf8CharGet(s, sl);
      s += sl;
    }
    n++;
  }
  if (Size > 0)
    *a = 0;
  return n;
}

int cUtf8Utils::Utf8FromArray(const uint *a, char *s, int Size, int Max)
{
  int NumChars = 0;
  int NumSyms = 0;
  while (*a && NumChars < Size)
  {
    if (Max >= 0 && NumSyms++ >= Max)
      break;
    if (cCharSetConv::SystemCharacterTable())
    {
      *s++ = *a++;
      NumChars++;
    }
    else
    {
      int sl = Utf8CharSet(*a);
      if (NumChars + sl <= Size)
      {
        Utf8CharSet(*a, s);
        a++;
        s += sl;
        NumChars += sl;
      }
      else
        break;
    }
  }
  if (NumChars < Size)
    *s = 0;
  return NumChars;
}
