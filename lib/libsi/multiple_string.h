/*
 * Copyright (C) 2013-2014 Garrett Brown
 * Copyright (C) 2013-2014 Lars Op den Kamp
 * Portions Copyright (c) 2003 by Marcel Wiesweg
 * Portions Copyright (C) 2006-2009 Alex Lasnier <alex@fepg.org>
 *
 * Adapted from ATSC EPG's huffman.cpp (v0.3.0)
 * Adapted from MythTV's atsc_huffman.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBSI_MULTIPLE_STRING_H
#define LIBSI_MULTIPLE_STRING_H

#include "si.h"
#include "headers.h"

namespace SI {

class PSIPString : public VariableLengthPart {
public:
   PSIPString();
   void setData(CharArray d);
   void setDataAndOffset(CharArray d, int &offset) { setData(d); offset += getLength(); }

   virtual char *getText(char *buffer, int size);

   int getISO639LanguageCode() const { return iso639LanguageCode; }
protected:
   virtual void Parse() {}
   void decodeText(char *buffer, int size);
private:
   int iso639LanguageCode;
};

class PSIPStringLoop : public StructureLoop<PSIPString> {
public:
   void setData(CharArray d);
   void setDataAndOffset(CharArray d, int &offset) { setData(d); offset += getLength(); }
};

} //end of namespace

#endif //LIBSI_MULTIPLE_STRING_H
