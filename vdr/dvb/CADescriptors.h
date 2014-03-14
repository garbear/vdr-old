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

#include "Types.h"
#include "CADescriptor.h"

#include <libsi/descriptor.h>
#include <stdint.h>

class cCaDescriptors
{
public:
  cCaDescriptors(int source, int transponder, int serviceId);

  bool operator==(const cCaDescriptors& arg) const;

  bool Is(int source, int transponder, int serviceId) const;
  bool Is(const cCaDescriptors& caDescriptors) const;

  bool Empty(void) { return m_caDescriptors.size() == 0; }

  void AddCaDescriptor(SI::CaDescriptor* d, int esPid);

  /*!
   * \param EsPid Used to select the "type" of CaDescriptor to be returned
   *        Possible ranges:
   *          >0 - CaDescriptor for the particular esPid
   *          =0 - common CaDescriptor
   *          <0 - all CaDescriptors regardless of type (old default)
   */
  int GetCaDescriptors(const int* caSystemIds, int bufSize, uint8_t* data, int esPid) const;

  const std::vector<int>& CaIds(void);

private:
  void AddCaId(int CaId);

  int                m_source;
  int                m_transponder;
  int                m_serviceId;
  std::vector<int>   m_caIds;
  CaDescriptorVector m_caDescriptors;
};

typedef VDR::shared_ptr<cCaDescriptors> CaDescriptorsPtr;
typedef std::vector<CaDescriptorsPtr>   CaDescriptorsVector;
