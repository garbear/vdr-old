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

#include "ChannelFilter.h"
#include "Config.h"
#include "settings/Settings.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "utils/Tools.h"

using namespace PLATFORM;

cVNSIProvider::cVNSIProvider()
  :m_name(""), m_caid(0)
{

}

cVNSIProvider::cVNSIProvider(std::string name, int caid)
  :m_name(name), m_caid(caid)
{
};

bool cVNSIProvider::operator==(const cVNSIProvider &rhs)
{
  if (rhs.m_caid != m_caid)
    return false;
  if (rhs.m_name.compare(m_name) != 0)
    return false;
  return true;
}


bool cVNSIChannelFilter::IsRadio(const ChannelPtr channel)
{
  bool isRadio = false;

  // assume channels without VPID & APID are video channels
  if (channel->Vpid() == 0 && channel->Apid(0) == 0)
    isRadio = false;
  // channels without VPID are radio channels (channels with VPID 1 are encrypted radio channels)
  else if (channel->Vpid() == 0 || channel->Vpid() == 1)
    isRadio = true;

  return isRadio;
}

void cVNSIChannelFilter::Load()
{
  CLockObject lock(m_Mutex);

  cString filename;
  std::string line;
  std::ifstream rfile;
  cVNSIProvider provider;
  std::vector<cVNSIProvider>::iterator p_it;

  filename = cString::sprintf("%s/videowhitelist.vnsi", cSettings::Get().m_ConfigDirectory.c_str());
  m_providersVideo.clear();
  rfile.open(filename);
  if (rfile.is_open())
  {
    while(std::getline(rfile,line))
    {
      size_t pos = line.find("|");
      if(pos == line.npos)
      {
        provider.m_name = line;
        provider.m_caid = 0;
      }
      else
      {
        provider.m_name = line.substr(0, pos);
        std::string tmp = line.substr(pos+1);
        char *pend;
        provider.m_caid = strtol(tmp.c_str(), &pend, 10);
      }
      p_it = std::find(m_providersVideo.begin(), m_providersVideo.end(), provider);
      if(p_it == m_providersVideo.end())
      {
        m_providersVideo.push_back(provider);
      }
    }
    rfile.close();
  }

  filename = cString::sprintf("%s/radiowhitelist.vnsi", cSettings::Get().m_ConfigDirectory.c_str());
  rfile.open(filename);
  m_providersRadio.clear();
  if (rfile.is_open())
  {
    while(std::getline(rfile,line))
    {
      unsigned int pos = line.find("|");
      if(pos == line.npos)
      {
        provider.m_name = line;
        provider.m_caid = 0;
      }
      else
      {
        provider.m_name = line.substr(0, pos);
        std::string tmp = line.substr(pos+1);
        char *pend;
        provider.m_caid = strtol(tmp.c_str(), &pend, 10);
      }
      p_it = std::find(m_providersRadio.begin(), m_providersRadio.end(), provider);
      if(p_it == m_providersRadio.end())
      {
        m_providersRadio.push_back(provider);
      }
    }
    rfile.close();
  }

  filename = cString::sprintf("%s/videoblacklist.vnsi", cSettings::Get().m_ConfigDirectory.c_str());
  rfile.open(filename);
  m_channelsVideo.clear();
  if (rfile.is_open())
  {
    while(getline(rfile,line))
    {
      char *pend;
      int id = strtol(line.c_str(), &pend, 10);
      m_channelsVideo.push_back(id);
    }
    rfile.close();
  }

  filename = cString::sprintf("%s/radioblacklist.vnsi", cSettings::Get().m_ConfigDirectory.c_str());
  rfile.open(filename);
  m_channelsRadio.clear();
  if (rfile.is_open())
  {
    while(getline(rfile,line))
    {
      char *pend;
      int id = strtol(line.c_str(), &pend, 10);
      m_channelsRadio.push_back(id);
    }
    rfile.close();
  }
}

void cVNSIChannelFilter::StoreWhitelist(bool radio)
{
  CLockObject lock(m_Mutex);

  cString filename;
  std::ofstream wfile;
  cVNSIProvider provider;
  std::vector<cVNSIProvider>::iterator p_it;
  std::vector<cVNSIProvider> *whitelist;

  if (radio)
  {
    filename = cString::sprintf("%s/radiowhitelist.vnsi", cSettings::Get().m_ConfigDirectory.c_str());
    whitelist = &m_providersRadio;
  }
  else
  {
    filename = cString::sprintf("%s/videowhitelist.vnsi", cSettings::Get().m_ConfigDirectory.c_str());
    whitelist = &m_providersVideo;
  }

  wfile.open(filename);
  if(wfile.is_open())
  {
    std::string tmp;
    char buf[16];
    for(p_it=whitelist->begin(); p_it!=whitelist->end(); ++p_it)
    {
      tmp = p_it->m_name;
      tmp += "|";
      sprintf(buf, "%d\n", p_it->m_caid);
      tmp += buf;
      wfile << tmp;
    }
    wfile.close();
  }

  SortChannels();
}

void cVNSIChannelFilter::StoreBlacklist(bool radio)
{
  CLockObject lock(m_Mutex);

  cString filename;
  std::ofstream wfile;
  cVNSIProvider provider;
  std::vector<int>::iterator it;
  std::vector<int> *blacklist;

  if (radio)
  {
    filename = cString::sprintf("%s/radioblacklist.vnsi", cSettings::Get().m_ConfigDirectory.c_str());
    blacklist = &m_channelsRadio;
  }
  else
  {
    filename = cString::sprintf("%s/videoblacklist.vnsi", cSettings::Get().m_ConfigDirectory.c_str());
    blacklist = &m_channelsVideo;
  }

  wfile.open(filename);
  if(wfile.is_open())
  {
    std::string tmp;
    char buf[16];
    for(it=blacklist->begin(); it!=blacklist->end(); ++it)
    {
      sprintf(buf, "%d\n", *it);
      tmp = buf;
      wfile << tmp;
    }
    wfile.close();
  }

  SortChannels();
}

bool cVNSIChannelFilter::IsWhitelist(const ChannelPtr channel)
{
  cVNSIProvider provider;
  std::vector<cVNSIProvider>::iterator p_it;
  std::vector<cVNSIProvider> *providers;
  provider.m_name = channel->Provider();

  if (IsRadio(channel))
    providers = &m_providersRadio;
  else
    providers = &m_providersVideo;

  if(providers->empty())
    return true;

  if (channel->Ca(0) == 0)
  {
    provider.m_caid = 0;
    p_it = std::find(providers->begin(), providers->end(), provider);
    if(p_it!=providers->end())
      return true;
    else
      return false;
  }

  int caid;
  int idx = 0;
  while((caid = channel->Ca(idx)) != 0)
  {
    provider.m_caid = caid;
    p_it = std::find(providers->begin(), providers->end(), provider);
    if(p_it!=providers->end())
      return true;

    idx++;
  }
  return false;
}

bool cVNSIChannelFilter::PassFilter(const ChannelPtr channel)
{
  CLockObject lock(m_Mutex);

  if(channel->GroupSep())
    return true;

  if (!IsWhitelist(channel))
    return false;

  std::vector<int>::iterator it;
  if (IsRadio(channel))
  {
    it = std::find(m_channelsRadio.begin(), m_channelsRadio.end(), channel->Hash());
    if(it!=m_channelsRadio.end())
      return false;
  }
  else
  {
    it = std::find(m_channelsVideo.begin(), m_channelsVideo.end(), channel->Hash());
    if(it!=m_channelsVideo.end())
      return false;
  }

  return true;
}

void cVNSIChannelFilter::SortChannels()
{
  cChannelManager::Get().IncBeingEdited();
//  XXX Channels.Lock(true);
  std::vector<ChannelPtr> channels = cChannelManager::Get().GetCurrent();

  for (std::vector<ChannelPtr>::iterator it = channels.begin(); it != channels.end(); ++it)
  {
    if(!PassFilter(*it))
    {
      std::vector<ChannelPtr>::iterator it2 = it;
      if (it2 != channels.end())
        ++it2;
      for (; it2 != channels.end(); ++it2)
      {
        if(PassFilter(*it2))
        {
//          TODO cChannelManager::Get().Move(**it2, **it);
          it = it2;
          break;
        }
      }
    }
  }

  cChannelManager::Get().SetModified(true);
//  XXX Channels.Unlock();
  cChannelManager::Get().DecBeingEdited();
}

cVNSIChannelFilter VNSIChannelFilter;