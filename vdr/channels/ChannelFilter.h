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

#include "ChannelTypes.h"
#include "lib/platform/threads/mutex.h"

#include <string>
#include <vector>

namespace VDR
{
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
}
