/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>
#include <vector>
#include "lib/platform/threads/mutex.h"
#include "channels/ChannelManager.h"

class CChannelProvider
{
public:
  CChannelProvider();
  CChannelProvider(std::string name, int caid);
  bool operator==(const CChannelProvider &rhs);
  std::string m_name;
  int m_caid;
};

class CChannelFilter
{
public:
  void Load();
  void StoreWhitelist(bool radio);
  void StoreBlacklist(bool radio);
  bool IsWhitelist(const ChannelPtr channel);
  bool PassFilter(const ChannelPtr channel);
  void SortChannels();
  static bool IsRadio(const ChannelPtr channel);
  std::vector<CChannelProvider> m_providersVideo;
  std::vector<CChannelProvider> m_providersRadio;
  std::vector<int> m_channelsVideo;
  std::vector<int> m_channelsRadio;
  PLATFORM::CMutex m_Mutex;
};

extern CChannelFilter VNSIChannelFilter;
