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
#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <map>
#include "platform/threads/mutex.h"

class CChannelGroup
{
public:
  CChannelGroup(void) :
    m_bRadio(false),
    m_bAutomatic(false) {}
  CChannelGroup(bool bRadio, const std::string& strName, bool bAutomatic) :
    m_bRadio(bRadio),
    m_strName(strName),
    m_bAutomatic(bAutomatic) {}
  virtual ~CChannelGroup(void) {}

  CChannelGroup& operator=(const CChannelGroup& other);

  bool Radio(void) const       { return m_bRadio; }
  std::string Name(void) const { return m_strName; }
  bool Automatic(void) const   { return m_bAutomatic; }

private:
  bool        m_bRadio;
  std::string m_strName;
  bool        m_bAutomatic;
};

class CChannelGroups
{
public:
  static CChannelGroups& Get(bool bRadio);
  virtual ~CChannelGroups(void) {}

  void Clear(void);
  size_t Size(void) const;
  void AddGroup(const CChannelGroup& group);
  bool HasGroup(const std::string& strName) const;
  std::vector<std::string> GroupNames(void) const;
  const CChannelGroup* GetGroup(const std::string& strName) const;
private:
  CChannelGroups(void) {}

  std::map<std::string, CChannelGroup> m_groups;
  PLATFORM::CMutex                     m_mutex;
};
