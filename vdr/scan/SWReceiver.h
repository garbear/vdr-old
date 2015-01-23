/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "channels/ChannelTypes.h"
#include "devices/Receiver.h"
#include "lib/platform/threads/threads.h"
#include "utils/DateTime.h"

#include <stdint.h>

namespace VDR
{

class cRingBufferLinear;

class cSwReceiver : public iReceiver, public PLATFORM::CThread
{
public:
  cSwReceiver(ChannelPtr channel);
  virtual ~cSwReceiver();

  void Reset();

  bool Finished()       const { return m_bStopped; }
  uint16_t CNI_8_30_1() const { return cni_8_30_1; };
  uint16_t CNI_8_30_2() const { return cni_8_30_2; };
  uint16_t CNI_X_26()   const { return cni_X_26;   };
  uint16_t CNI_VPS()    const { return cni_vps;    };
  uint16_t CNI_CR_IDX() const { return cni_cr_idx; };
  bool     Fuzzy()      const { return fuzzy;      };

  void UpdatefromName(const char* name);
  const char* GetCniNameFormat1();
  const char* GetCniNameFormat2();
  const char* GetCniNameX26();
  const char* GetCniNameVPS();
  const char* GetCniNameCrIdx();
  const char* GetFuzzyName() const { return fuzzy_network; };

protected:
  virtual void Receive(uint8_t* Data, int Length, ts_crc_check_t& crcvalid);
  virtual void* Process();
  virtual void Stop() { m_bStopped = true; };
  void Decode(uint8_t* data, int count);
  void DecodePacket(uint8_t* data);

private:
  ChannelPtr channel;
  cRingBufferLinear* buffer;
  bool m_bStopped;
  bool fuzzy;
  int hits;
  CDateTime m_timeout;
  uint32_t TsCount;
  uint16_t cni_8_30_1;
  uint16_t cni_8_30_2;
  uint16_t cni_X_26;
  uint16_t cni_vps;
  uint16_t cni_cr_idx;
  char fuzzy_network[48];
};

}
