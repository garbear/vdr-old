/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "DVBTypes.h"
#include "CADescriptor.h"

#include <map>
#include "platform/threads/mutex.h"
#include <stdint.h>
#include <vector>

namespace VDR
{
// TODO: Should this be defined in CADescriptor.h?
struct DescriptorId
{
  int source;
  int transponder;
  int serviceId;
};

bool operator<(const DescriptorId& lhs, const DescriptorId& rhs);

class cCaDescriptorHandler
{
public:
  static cCaDescriptorHandler& Get();

  /*!
   * \brief Returns true if any descriptors were added or modified
   */
  bool AddCaDescriptors(int source, int transponder, int serviceId,
      const CaDescriptorVector& caDescriptors);

  int GetCaDescriptors(int source, int transponder, uint16_t serviceId,
      const std::vector<uint16_t>& caSystemIds, std::vector<uint8_t>& data, int esPid = -1);

private:
  cCaDescriptorHandler() { }

  typedef std::map<DescriptorId, CaDescriptorVector> CaDescriptorCollection;

  CaDescriptorCollection m_caDescriptors;
  PLATFORM::CMutex       m_mutex;
};
}
