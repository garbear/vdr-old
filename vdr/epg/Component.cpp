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

#include "Component.h"
#include "utils/Tools.h"

using namespace std;

namespace VDR
{

CEpgComponent::CEpgComponent(void)
{
  m_stream      = 0;
  m_type        = 0;
}

string CEpgComponent::Serialise(void)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%X %02X %s %s", m_stream, m_type, m_strLanguage.c_str(), m_strDescription.c_str());
  return buffer;
}

bool CEpgComponent::Deserialse(const std::string& str)
{
  unsigned int Stream, Type;
  char* description;
  char language[MAXLANGCODE2];
  int n = sscanf(str.c_str(), "%X %02X %7s %a[^\n]", &Stream, &Type, language, &description); // 7 = MAXLANGCODE2 - 1
  if (n == 4)
  {
    m_strLanguage    = language;
    m_strDescription = description;
  }
  free(description);
  m_stream = Stream;
  m_type = Type;
  return n >= 3;
}

uint8_t CEpgComponent::Stream(void) const
{
  return m_stream;
}

uint8_t CEpgComponent::Type(void) const
{
  return m_type;
}

std::string CEpgComponent::Language(void) const
{
  return m_strLanguage;
}

std::string CEpgComponent::Description(void) const
{
  return m_strDescription;
}

void CEpgComponent::SetStream(uint8_t stream)
{
  m_stream = stream;
}

void CEpgComponent::SetType(uint8_t type)
{
  m_type = type;
}

void CEpgComponent::SetLanguage(const std::string& lang)
{
  size_t comma = lang.find(',');
  if (comma != std::string::npos)
    m_strLanguage = lang.substr(0, comma - 1); // strips rest of "normalized" language codes
  else
    m_strLanguage = lang;
}

void CEpgComponent::SetDescription(const std::string& desc)
{
  m_strDescription = desc;
}

}
