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

#include "CADescriptors.h"
#include "channels/Channel.h" // for MAXCAIDS
#include "utils/StringUtils.h"

#include <algorithm>
#include <sstream>

using namespace std;

#define DEBUG_CA_DESCRIPTORS  1

cCaDescriptors::cCaDescriptors(int source, int transponder, int serviceId)
 : m_source(source),
   m_transponder(transponder),
   m_serviceId(serviceId)
{
  //numCaIds = 0;
  //caIds[0] = 0;
}

bool cCaDescriptors::operator==(const cCaDescriptors &arg) const
{
  return m_caDescriptors == arg.m_caDescriptors;
}

bool cCaDescriptors::Is(int Source, int Transponder, int ServiceId) const
{
  return m_source == Source && m_transponder == Transponder && m_serviceId == ServiceId;
}

bool cCaDescriptors::Is(const cCaDescriptors& caDescriptors) const
{
  return Is(caDescriptors.m_source, caDescriptors.m_transponder, caDescriptors.m_serviceId);
}

void cCaDescriptors::AddCaId(int CaId)
{
  if (m_caIds.size() < MAXCAIDS)
  {
    if (std::find(m_caIds.begin(), m_caIds.end(), CaId) != m_caIds.end())
      return;
    m_caIds.push_back(CaId);
  }
}

void cCaDescriptors::AddCaDescriptor(SI::CaDescriptor* d, int esPid)
{
  CaDescriptorPtr nca = CaDescriptorPtr(new cCaDescriptor(d->getCaType(), d->getCaPid(), esPid, d->privateData.getLength(), d->privateData.getData()));
  for (CaDescriptorVector::const_iterator it = m_caDescriptors.begin(); it != m_caDescriptors.end(); ++it)
  {
    if (**it == *nca)
      return;
  }

  AddCaId(nca->CaSystem());
  m_caDescriptors.push_back(nca);

#if DEBUG_CA_DESCRIPTORS
  stringstream buffer;
  buffer << StringUtils::Format("CAM: %04X %5d %5d %04X %04X -", m_source, m_transponder, m_serviceId, d->getCaType(), esPid);

  for (size_t i = 0; i < nca->Data().size(); i++)
    buffer << StringUtils::Format(" %02X", nca->Data()[i]);

  dsyslog("%s", buffer.str().c_str());
#endif

}

int cCaDescriptors::GetCaDescriptors(const int* caSystemIds, int bufSize, uchar* data, int esPid) const
{
  if (!caSystemIds || !*caSystemIds)
    return 0;
  if (bufSize > 0 && data)
  {
    int length = 0;
    for (CaDescriptorVector::const_iterator it = m_caDescriptors.begin(); it != m_caDescriptors.end(); ++it)
    {
      if (esPid < 0 || (*it)->EsPid() == esPid)
      {
        const int *caids = caSystemIds;
        do
        {
          if ((*it)->CaSystem() == *caids)
          {
            if (length + (*it)->Data().size() <= bufSize)
            {
              memcpy(data + length, (*it)->Data().data(), (*it)->Data().size());
              length += (*it)->Data().size();
            }
            else
              return -1;
          }
        } while (*++caids);
      }
    }
    return length;
  }
  return -1;
}

const vector<int>& cCaDescriptors::CaIds(void)
{
  // Zero-terminate the vector (TODO: remove this after converting everything to vectors and zero-padding is no longer necessary)
  m_caIds.reserve(m_caIds.size() + 1);
  *(m_caIds.data() + m_caIds.size()) = 0;

  return m_caIds;
}
