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

#include "Components.h"

using namespace std;

namespace VDR
{

void CEpgComponents::SetComponent(int Index, const char *s)
{
  CEpgComponent* component = Component(Index, true);
  if (component)
    component->Deserialse(s);
}

void CEpgComponents::SetComponent(int Index, uint8_t Stream, uint8_t Type, const std::string& Language, const std::string& Description)
{
  CEpgComponent* component = Component(Index, true);
  if (!component)
    return;

  component->SetStream(Stream);
  component->SetType(Type);
  component->SetLanguage(Language);
  component->SetDescription(Description);
}

CEpgComponent *CEpgComponents::GetComponent(int Index, uint8_t Stream, uint8_t Type)
{
  for (std::map<int, CEpgComponent>::iterator it = m_components.begin(); it != m_components.end(); ++it)
  {
    if (it->second.Stream() == Stream && (
          Type == 0 || // don't care about the actual Type
          (Stream == 2 && (it->second.Type() < 5) == (Type < 5)) // fallback "Dolby" component according to the "Premiere pseudo standard"
         ))
    {
      if (!Index--)
        return &it->second;
    }
  }
  return NULL;
}

int CEpgComponents::NumComponents(void) const
{
  return m_components.size();
}

CEpgComponent* CEpgComponents::Component(int Index, bool bCreate /* = false */)
{
  if (Index >= 0)
  {
    std::map<int, CEpgComponent>::iterator it = m_components.find(Index);
    if (it != m_components.end())
      return &it->second;
  }

  if (bCreate)
  {
    CEpgComponent component;
    if (Index < 0)
      Index = m_components.size();
    m_components.insert(std::make_pair(Index, component));
    return &m_components[Index];
  }

  return NULL;
}

}
