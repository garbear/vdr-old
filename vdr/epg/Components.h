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
#pragma once

#include "Component.h"
#include "channels/Channel.h"
#include "libsi/section.h"

#include <map>
#include <stdint.h>
#include <string>

namespace VDR
{
class CEpgComponents
{
#define COMPONENT_ADD_NEW (-1)

public:
  CEpgComponents(void) {}
  virtual ~CEpgComponents(void) {}

  int NumComponents(void) const;
  void SetComponent(int Index, const char *s);
  void SetComponent(int Index, uint8_t Stream, uint8_t Type, const std::string& Language, const std::string& Description);

  CEpgComponent* Component(int Index, bool bCreate = false);
  CEpgComponent* GetComponent(int Index, uint8_t Stream, uint8_t Type); // Gets the Index'th component of Stream and Type, skipping other components
                                                                 // In case of an audio stream the 'type' check actually just distinguishes between "normal" and "Dolby Digital"

private:
  std::map<int, CEpgComponent> m_components;
};
}
