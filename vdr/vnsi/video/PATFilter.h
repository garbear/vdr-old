#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *
 */

#include "Types.h"
#include "channels/Channel.h"
#include "dvb/filters/Filter.h"

namespace VDR
{

class cVideoInput;

class cLivePatFilter : public cFilter
{
private:
  int             m_pmtPid;
  int             m_pmtSid;
  int             m_pmtVersion;
  ChannelPtr      m_Channel;
  cVideoInput    *m_VideoInput;

  virtual void ProcessData(u_short pid, u_char tid, const std::vector<uint8_t>& data);

public:
  cLivePatFilter(cDevice* device, cVideoInput *VideoInput, ChannelPtr Channel);
};

}
