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

#include "channels/Channel.h"
#include "channels/ChannelTypes.h"
#include "ScanReceiver.h"

#include <libsi/section.h>
#include <stdint.h>

namespace VDR
{

typedef struct
{
  uint16_t pid;
  uint16_t tsid;
  uint16_t sid;
} PMTFilter;

class cPmt : public cScanReceiver
{
public:
  cPmt(cDevice* device);
  virtual ~cPmt(void) { }

  bool AddTransport(uint16_t tsid, uint16_t sid, uint16_t pid);
  void ReceivePacket(uint16_t pid, const uint8_t* data);

private:
  ChannelPtr CreateChannel(/* const */ SI::PMT& pmt, uint16_t tsid) const; // TODO: libsi fails at const-correctness

  void SetStreams(const ChannelPtr& channel, /* const */ SI::PMT& pmt) const; // TODO: libsi fails at const-correctness
  void SetCaDescriptors(const ChannelPtr& channel, /* const */ SI::PMT& pmt) const; // TODO: libsi fails at const-correctness

  bool HasPid(uint16_t pid) const;

  std::vector<PMTFilter> m_filters;
};

}
