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

#include "channels/Channel.h"
#include "libsi/section.h"

#include <map>
#include <stdint.h>

namespace VDR
{
class CEpgComponent
{
public:
  CEpgComponent(void);
  virtual ~CEpgComponent(void) {}

  std::string Serialise(void);
  bool Deserialse(const std::string& str);

  uint8_t Stream(void) const;
  uint8_t Type(void) const;
  std::string Language(void) const;
  std::string Description(void) const;

  void SetStream(uint8_t stream);
  void SetType(uint8_t type);
  void SetLanguage(const std::string& lang);
  void SetDescription(const std::string& desc);

private:
  uint8_t     m_stream;
  uint8_t     m_type;
  std::string m_strLanguage;
  std::string m_strDescription;
};
}
