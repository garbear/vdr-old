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

#include "CADescriptors.h"

#include "platform/threads/mutex.h"
#include <stdint.h>

class cCaDescriptorHandler
{
public:
  static cCaDescriptorHandler& Get();

  /*!
   * \brief Returns 0 if this is an already known descriptor, 1 if it is an all
   *        new descriptor with actual contents, and 2 if an existing descriptor
   *        was changed. TODO: Change to an enum return value
   */
  int AddCaDescriptors(const CaDescriptorsPtr& caDescriptors);

  int GetCaDescriptors(int source, int transponder, int serviceId, const int* caSystemIds, int bufSize, uint8_t* data, int esPid);

private:
  cCaDescriptorHandler() { }

  CaDescriptorsVector m_caDescriptors;
  PLATFORM::CMutex    m_mutex;
};
