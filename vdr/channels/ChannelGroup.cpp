/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "ChannelGroup.h"

using namespace std;
using namespace PLATFORM;

CChannelGroups& CChannelGroups::Get(bool bRadio)
{
  static CChannelGroups radio;
  static CChannelGroups tv;
  return bRadio ? radio : tv;
}

void CChannelGroups::Clear(void)
{
  CLockObject lock(m_mutex);
  m_groups.clear();
}

size_t CChannelGroups::Size(void) const
{
  CLockObject lock(m_mutex);
  return m_groups.size();
}

void CChannelGroups::AddGroup(const CChannelGroup& group)
{
  CLockObject lock(m_mutex);
  m_groups[group.Name()] = group;
}

bool CChannelGroups::HasGroup(const std::string& strName) const
{
  CLockObject lock(m_mutex);
  return m_groups.find(strName) != m_groups.end();
}

std::vector<std::string> CChannelGroups::GroupNames(void) const
{
  vector<string> retval;
  CLockObject lock(m_mutex);
  for (std::map<std::string, CChannelGroup>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    retval.push_back(it->second.Name());
  return retval;
}

const CChannelGroup* CChannelGroups::GetGroup(const std::string& strName) const
{
  CLockObject lock(m_mutex);
  std::map<std::string, CChannelGroup>::const_iterator it = m_groups.find(strName);

  return it == m_groups.end() ?
      &it->second :
      NULL;

}

CChannelGroup& CChannelGroup::operator=(const CChannelGroup& other)
{
  if (this != &other)
  {
    m_bAutomatic = other.m_bAutomatic;
    m_bRadio     = other.m_bRadio;
    m_strName    = other.m_strName;
  }

  return *this;
}
