/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "../../../tools.h"

//#include <list>
#include <stdint.h>

// A plugin that implements a DVB device derived from cDvbDevice needs to create
// a cDvbDeviceProbe derived object on the heap in order to have its Probe()
// function called, where it can actually create the appropriate device.
// The cDvbDeviceProbe object must be created in the plugin's constructor,
// and deleted in its destructor.

class cDvbDeviceProbe : public cListObject
{
public:
  cDvbDeviceProbe();
  virtual ~cDvbDeviceProbe();

  static uint32_t GetSubsystemId(int adapter, int frontend);

  /*!
   * \brief Probes for a DVB device at the given adapter and creates the
   *        appropriate object derived from cDvbDevice if applicable
   * \return True if a device has been created
   */
  virtual bool Probe(int adapter, int frontend) = 0;
};

extern cList<cDvbDeviceProbe> DvbDeviceProbes;
