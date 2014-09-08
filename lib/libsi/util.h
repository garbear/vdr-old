/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: util.h 2.3 2012/02/26 13:58:26 kls Exp $
 *                                                                         *
 ***************************************************************************/

#ifndef LIBSI_UTIL_H
#define LIBSI_UTIL_H

#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#if !defined(si_time_gm)
#if defined(TARGET_ANDROID)
#include <time64.h>
typedef time64_t si_time_t;
#define si_time_gm(x) timegm64(x)
#else
typedef time_t si_time_t;
#define si_time_gm(x)  timegm(x)
#endif
#endif

#define HILO(x) (x##_hi << 8 | x##_lo)
#define LOHILO(x) (x##_hi_lo << 16 | x##_lo_hi << 8 | x##_lo_lo)
#define HILOHILO(x) (x##_hi_hi << 24 | x##_hi_lo << 16 | x##_lo_hi << 8 | x##_lo_lo)
#define BCD_TIME_TO_SECONDS(x) ((3600 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                             (60 * ((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))) + \
                             ((10*((x##_s & 0xF0)>>4)) + (x##_s & 0xF)))

namespace SI {

//Holds an array of unsigned char which is deleted
//when the last object pointing to it is deleted.
//Optimized for use in libsi.
class CharArray {
public:
   CharArray();

   //can be called exactly once
   void assign(const unsigned char*data, int size);
   //compares to a null-terminated string
   bool operator==(const char *string) const;
   //compares to another CharArray (data not necessarily null-terminated)
   bool operator==(const CharArray &other) const;

   //returns another CharArray with its offset incremented by offset
   CharArray operator+(const int offset) const;

   //access and convenience methods
   const unsigned char* getData() const { return data+off; }
   const unsigned char* getData(int offset) const { return data+offset+off; }
   template <typename T> const T* getData() const { return (T*)(data+off); }
   template <typename T> const T* getData(int offset) const { return (T*)(data+offset+off); }
      //sets p to point to data+offset, increments offset
   template <typename T> void setPointerAndOffset(const T* &p, int &offset) const { p=(T*)getData(offset); offset+=sizeof(T); }
   unsigned char operator[](const int index) const { return data ? data[off+index] : (unsigned char)0; }
   int getLength() const { return size; }
   u_int16_t TwoBytes(const int index) const { return data ? u_int16_t((data[off+index] << 8) | data[off+index+1]) : u_int16_t(0); }
   u_int32_t FourBytes(const int index) const { return data ? u_int32_t((data[off+index] << 24) | (data[off+index+1] << 16) | (data[off+index+2] << 8) | data[off+index+3]) : u_int32_t(0); }

   bool isValid() const { return valid; }
   bool checkSize(int offset);

   void addOffset(int offset) { off+=offset; }

private:
   const unsigned char*data;
   int size;
   bool valid;
   int off;
};



//abstract base class
class Parsable {
public:
   void CheckParse();
protected:
   Parsable();
   virtual ~Parsable() {}
   //actually parses given data.
   virtual void Parse() = 0;
private:
   bool parsed;
};

//taken and adapted from libdtv, (c) Rolf Hakenes and VDR, (c) Klaus Schmidinger
namespace DVBTime {
  si_time_t getTime(unsigned char date_hi, unsigned char date_lo, unsigned char timehr, unsigned char timemi, unsigned char timese);
  si_time_t getDuration(unsigned char timehr, unsigned char timemi, unsigned char timese);
  inline unsigned char bcdToDec(unsigned char b) { return (unsigned char)(((b >> 4) & 0x0F) * 10 + (b & 0x0F)); }
}

//taken and adapted from libdtv, (c) Rolf Hakenes
class CRC32 {
public:
   CRC32(const char *d, int len, u_int32_t CRCvalue=0xFFFFFFFF);
   bool isValid() { return crc32(data, length, value) == 0; }
   static bool isValid(const char *d, int len, u_int32_t CRCvalue=0xFFFFFFFF) { return crc32(d, len, CRCvalue) == 0; }
   static u_int32_t crc32(const char *d, int len, u_int32_t CRCvalue);
protected:
   static u_int32_t crc_table[256];

   const char *data;
   int length;
   u_int32_t value;
};

} //end of namespace

#endif
