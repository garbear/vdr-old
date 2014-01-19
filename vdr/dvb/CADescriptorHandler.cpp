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

cCaDescriptorHandler& cCaDescriptorHandler::Get()
{
  static cCaDescriptorHandler handler;
  return handler;
}

int cCaDescriptorHandler::AddCaDescriptors(const CaDescriptorsPtr& caDescriptors)
{
  PLATFORM::CLockObject lock(m_mutex);

  for (CaDescriptorsVector::iterator itCaDes = m_caDescriptors.begin(); itCaDes != m_caDescriptors.end(); ++itCaDes)
  {
    if ((*itCaDes)->Is(*caDescriptors))
    {
      if (**itCaDes == *caDescriptors)
        return 0;

      m_caDescriptors.erase(itCaDes);
      m_caDescriptors.push_back(caDescriptors);
      return 2;
    }
  }

  m_caDescriptors.push_back(caDescriptors);
  return caDescriptors->Empty() ? 0 : 1;
}

int cCaDescriptorHandler::GetCaDescriptors(int source, int transponder, int serviceId, const int* caSystemIds, int bufSize, uint8_t* data, int esPid)
{
  PLATFORM::CLockObject lock(m_mutex);

  for (CaDescriptorsVector::const_iterator itCa = m_caDescriptors.begin(); itCa != m_caDescriptors.end(); ++itCa)
  {
    if ((*itCa)->Is(source, transponder, serviceId))
      return (*itCa)->GetCaDescriptors(caSystemIds, bufSize, data, esPid);
  }

  return 0;
}
