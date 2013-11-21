/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LIBSI_DISH_H
#define LIBSI_DISH_H

#include "si.h"

#define SIZE_TABLE_128 128
#define SIZE_TABLE_255 255

namespace SI {

class DishDescriptor : public Descriptor {
public:
   DishDescriptor(void);
   ~DishDescriptor();
   const char* getText(void) const { return text; }
   const char* getShortText(void) const { return shortText; }
   // Decompress the byte arrary and stores the result to a text string
   void Decompress(unsigned char Tid);
protected: 
   virtual void Parse() {};

   const char* text; // name or description of the event
   const char* shortText; // usually the episode name
   unsigned char* decompressed;
   
   struct HuffmanTable {
      unsigned int  startingAddress;
      unsigned char character;
      unsigned char numberOfBits;
   };
   static HuffmanTable Table128[SIZE_TABLE_128];
   static HuffmanTable Table255[SIZE_TABLE_255];  
};

} //end of namespace 

#endif //LIBSI_DISH_H
