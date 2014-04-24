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

#include "CharSetConverterVDR.h"
#include "UTF8Utils.h"
#include "Tools.h"

#include <string>

using namespace std;

namespace VDR
{

char *cCharSetConv::systemCharacterTable = NULL;

cCharSetConv::cCharSetConv(const char *FromCode, const char *ToCode)
{
  if (!FromCode)
    FromCode = systemCharacterTable ? systemCharacterTable : "UTF-8";
  if (!ToCode)
    ToCode = "UTF-8";
  cd = iconv_open(ToCode, FromCode);
  result = NULL;
  length = 0;
}

cCharSetConv::~cCharSetConv()
{
  free(result);
  if (cd != (iconv_t)-1)
    iconv_close(cd);
}

void cCharSetConv::SetSystemCharacterTable(const char *CharacterTable)
{
  free(systemCharacterTable);

  systemCharacterTable = NULL;

  if (!strcasestr(CharacterTable, "UTF-8"))
  {
    // Set up a map for the character values 128...255:
    char buf[129];

    for (unsigned int i = 0; i < 128; i++)
      buf[i] = i + 128;

    buf[128] = 0;

    cCharSetConv csc(CharacterTable);

    const char *converted = csc.Convert(buf);
    string strConverted(converted);
    for (unsigned int i = 0; !strConverted.empty(); i++)
    {
      int sl = cUtf8Utils::Utf8CharLen(strConverted);
      cUtf8Utils::SystemToUtf8[i] = cUtf8Utils::Utf8CharGet(strConverted);
      strConverted.erase(0, sl);
    }
    systemCharacterTable = strdup(CharacterTable);
  }
}

const char *cCharSetConv::Convert(const char *From, char *To, size_t ToLength)
{
  if (cd != (iconv_t)-1 && From && *From)
  {
    char *FromPtr = (char *)From;
    size_t FromLength = strlen(From);
    char *ToPtr = To;
    if (!ToPtr)
    {
      int NewLength = max(length, FromLength * 2); // some reserve to avoid later reallocations
      if (char *NewBuffer = (char *)realloc(result, NewLength))
      {
        length = NewLength;
        result = NewBuffer;
      }
      else
      {
        esyslog("ERROR: out of memory");
        return From;
      }
      ToPtr = result;
      ToLength = length;
    }
    else if (!ToLength)
      return From; // can't convert into a zero sized buffer
    ToLength--; // save space for terminating 0
    char *Converted = ToPtr;
    while (FromLength > 0)
    {
      if (iconv(cd, &FromPtr, &FromLength, &ToPtr, &ToLength) == size_t(-1))
      {
        if (errno == E2BIG || errno == EILSEQ && ToLength < 1)
        {
          if (To)
            break;
          // caller provided a fixed size buffer, but it was too small
          // The result buffer is too small, so increase it:
          size_t d = ToPtr - result;
          size_t r = length / 2;
          int NewLength = length + r;
          if (char *NewBuffer = (char *)realloc(result, NewLength))
          {
            length = NewLength;
            Converted = result = NewBuffer;
          }
          else
          {
            esyslog("ERROR: out of memory");
            return From;
          }
          ToLength += r;
          ToPtr = result + d;
        }
        if (errno == EILSEQ)
        {
          // A character can't be converted, so mark it with '?' and proceed:
          FromPtr++;
          FromLength--;
          *ToPtr++ = '?';
          ToLength--;
        }
        else if (errno != E2BIG)
          return From; // unknown error, return original string
      }
    }
    *ToPtr = 0;
    return Converted;
  }
  return From;
}

}
