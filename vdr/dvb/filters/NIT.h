/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "channels/ChannelTypes.h"
#include "dvb/filters/Filter.h"
#include "libsi/si.h"

#include <sys/types.h>

#define NETWORK_ID_UNKNOWN  0

namespace VDR
{

//#define INVALID_CHANNEL  0x4000 // TODO: Is this needed?

class iNitScannerCallback
{
public:
  virtual void NitFoundTransponder(ChannelPtr transponder, SI::TableId tid) = 0;
  virtual std::vector<ChannelPtr>& GetChannels() = 0;
  virtual ~iNitScannerCallback() { }
};

class cDevice;

class cNit : public cFilter
{
public:
  cNit(cDevice* device);
  virtual ~cNit() { }

  ChannelVector GetTransponders();

private:
  struct Network
  {
    Network() : nid(NETWORK_ID_UNKNOWN), bHasTransponder(false) { }
    Network(uint16_t nid, std::string strName) : nid(nid), name(strName), bHasTransponder(false) { }

    uint16_t    nid;  // Network ID
    std::string name; // Network name
    bool        bHasTransponder;
  };

  uint16_t m_networkId;
};

}
