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
#include "dvb/filters/Filter.h"

#include <libsi/section.h>
#include <stdint.h>

namespace VDR
{

class cPmt : public cFilter
{
public:
  cPmt(cDevice* device, uint16_t tsid, uint16_t sid, uint16_t pid);
  virtual ~cPmt(void) { }

  /*!
   * Scan for channel parameters like channel IDs, streams and CA descriptors.
   * Returns true if the scan ran until completion, or false if it was aborted
   * early (even if channels were found).
   */
  bool ScanChannel(iFilterCallback* callback);

private:
  ChannelPtr CreateChannel(/* const */ SI::PMT& pmt) const; // TODO: libsi fails at const-correctness

  void SetIds(const ChannelPtr& channel) const;
  void SetStreams(const ChannelPtr& channel, /* const */ SI::PMT& pmt) const; // TODO: libsi fails at const-correctness
  void SetCaDescriptors(const ChannelPtr& channel, /* const */ SI::PMT& pmt) const; // TODO: libsi fails at const-correctness

  const uint16_t m_tsid; // Transport stream ID
  const uint16_t m_sid; // Service ID
};

}
