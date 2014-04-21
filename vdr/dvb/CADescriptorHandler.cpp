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

#include "CADescriptorHandler.h"

using namespace std;

namespace VDR
{

bool operator<(const DescriptorId& lhs, const DescriptorId& rhs)
{
  if (lhs.source      < rhs.source) return true;
  if (lhs.source      > rhs.source) return false;

  if (lhs.transponder < rhs.transponder) return true;
  if (lhs.transponder > rhs.transponder) return false;

  if (lhs.serviceId   < rhs.serviceId) return true;
  if (lhs.serviceId   > rhs.serviceId) return false;

  return false;
}

cCaDescriptorHandler& cCaDescriptorHandler::Get()
{
  static cCaDescriptorHandler handler;
  return handler;
}

bool cCaDescriptorHandler::AddCaDescriptors(int source, int transponder, int serviceId,
    const CaDescriptorVector& caDescriptors)
{
  PLATFORM::CLockObject lock(m_mutex);

  DescriptorId descriptorId = { source, transponder, serviceId };

  CaDescriptorCollection::iterator it = m_caDescriptors.find(descriptorId);
  if (it != m_caDescriptors.end())
  {
    // If descriptors are unmodified, return false (no change)
    if (it->second == caDescriptors)
      return false;

    it->second = caDescriptors;
  }
  else
  {
    m_caDescriptors[descriptorId] = caDescriptors;
  }

  // Modifications were made
  return true;
}

int cCaDescriptorHandler::GetCaDescriptors(int source, int transponder, uint16_t serviceId,
    const vector<uint16_t>& caSystemIds, vector<uint8_t>& data, int esPid /* = -1 */)
{
  if (caSystemIds.empty())
    return 0;

  PLATFORM::CLockObject lock(m_mutex);

  DescriptorId descriptorId = { source, transponder, serviceId };

  CaDescriptorCollection::iterator it = m_caDescriptors.find(descriptorId);
  if (it != m_caDescriptors.end())
  {
    const CaDescriptorVector& caDescriptors = it->second;

    unsigned int length = 0;
    for (CaDescriptorVector::const_iterator it = caDescriptors.begin(); it != caDescriptors.end(); ++it)
    {
      const CaDescriptorPtr& descriptor = *it;

      // Filter by esPid (if provided)
      if (esPid >= 0 && descriptor->EsPid() != esPid)
        continue;

      bool bFound = false;
      for (vector<uint16_t>::const_iterator it2 = caSystemIds.begin(); it2 != caSystemIds.end(); ++it2)
      {
        uint16_t caSystemId = *it2;
        if (descriptor->CaSystem() == caSystemId)
        {
          bFound = true;
          break;
        }
      }

      if (bFound)
        data.insert(data.end(), descriptor->Data().begin(), descriptor->Data().end());
    }
    return length;
  }

  return 0;
}

}
