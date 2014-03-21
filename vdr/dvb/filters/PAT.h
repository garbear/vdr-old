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

#include "Types.h"
#include "Filter.h"
#include "utils/DateTime.h"

#include <stdint.h>
#include <vector>

namespace VDR
{

class cPatFilter : public cFilter
{
public:
  cPatFilter(cDevice* device);
  virtual ~cPatFilter() { }

  virtual void Enable(bool bEnabled);

  void Trigger(void);

protected:
  virtual void ProcessData(u_short pid, u_char tid, const std::vector<uint8_t>& data);

private:
  bool PmtVersionChanged(int pmtPid, int sid, int version);

  CDateTime m_lastPmtScan;
  int       m_pmtIndex;
  int       m_pmtPid;
  int       m_pmtSid;
  std::vector<uint64_t> m_pmtVersions;
};

int GetCaDescriptors(int Source, int Transponder, int ServiceId, const int *CaSystemIds, int BufSize, uchar *Data, int EsPid);
         ///< Gets all CA descriptors for a given channel.
         ///< Copies all available CA descriptors for the given Source, Transponder and ServiceId
         ///< into the provided buffer at Data (at most BufSize bytes). Only those CA descriptors
         ///< are copied that match one of the given CA system IDs.
         ///< Returns the number of bytes copied into Data (0 if no CA descriptors are
         ///< available), or -1 if BufSize was too small to hold all CA descriptors.
         ///< The return value tells whether these CA descriptors are to be used
         ///< for the individual streams.

}
