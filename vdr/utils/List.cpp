/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "List.h"

#include <stddef.h>
#include <stdlib.h>

namespace VDR
{

// --- cListObject -----------------------------------------------------------

cListObject::cListObject(void)
{
  prev = next = NULL;
}

cListObject::~cListObject()
{
}

void cListObject::Append(cListObject *Object)
{
  next = Object;
  Object->prev = this;
}

void cListObject::Insert(cListObject *Object)
{
  prev = Object;
  Object->next = this;
}

void cListObject::Unlink(void)
{
  if (next)
     next->prev = prev;
  if (prev)
     prev->next = next;
  next = prev = NULL;
}

int cListObject::Index(void) const
{
  cListObject *p = prev;
  int i = 0;

  while (p) {
        i++;
        p = p->prev;
        }
  return i;
}

// --- cListBase -------------------------------------------------------------

cListBase::cListBase(void)
{
  objects = lastObject = NULL;
  count = 0;
}

cListBase::~cListBase()
{
  Clear();
}

void cListBase::Add(cListObject *Object, cListObject *After)
{
  if (After && After != lastObject) {
     After->Next()->Insert(Object);
     After->Append(Object);
     }
  else {
     if (lastObject)
        lastObject->Append(Object);
     else
        objects = Object;
     lastObject = Object;
     }
  count++;
}

void cListBase::Ins(cListObject *Object, cListObject *Before)
{
  if (Before && Before != objects) {
     Before->Prev()->Append(Object);
     Before->Insert(Object);
     }
  else {
     if (objects)
        objects->Insert(Object);
     else
        lastObject = Object;
     objects = Object;
     }
  count++;
}

void cListBase::Del(cListObject *Object, bool DeleteObject)
{
  if (Object == objects)
     objects = Object->Next();
  if (Object == lastObject)
     lastObject = Object->Prev();
  Object->Unlink();
  if (DeleteObject)
     delete Object;
  count--;
}

void cListBase::Move(int From, int To)
{
  Move(Get(From), Get(To));
}

void cListBase::Move(cListObject *From, cListObject *To)
{
  if (From && To && From != To) {
     if (From->Index() < To->Index())
        To = To->Next();
     if (From == objects)
        objects = From->Next();
     if (From == lastObject)
        lastObject = From->Prev();
     From->Unlink();
     if (To) {
        if (To->Prev())
           To->Prev()->Append(From);
        From->Append(To);
        }
     else {
        lastObject->Append(From);
        lastObject = From;
        }
     if (!From->Prev())
        objects = From;
     }
}

void cListBase::Clear(void)
{
  while (objects) {
        cListObject *object = objects->Next();
        delete objects;
        objects = object;
        }
  objects = lastObject = NULL;
  count = 0;
}

cListObject *cListBase::Get(int Index) const
{
  if (Index < 0)
     return NULL;
  cListObject *object = objects;
  while (object && Index-- > 0)
        object = object->Next();
  return object;
}

static int CompareListObjects(const void *a, const void *b)
{
  const cListObject *la = *(const cListObject **)a;
  const cListObject *lb = *(const cListObject **)b;
  return la->Compare(*lb);
}

void cListBase::Sort(void)
{
  int n = Count();
  cListObject *a[n];
  cListObject *object = objects;
  int i = 0;
  while (object && i < n) {
        a[i++] = object;
        object = object->Next();
        }
  qsort(a, n, sizeof(cListObject *), CompareListObjects);
  objects = lastObject = NULL;
  for (i = 0; i < n; i++) {
      a[i]->Unlink();
      count--;
      Add(a[i]);
      }
}

}
