/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

/*
 * This file defines the frontend names (result of FE_GET_INFO ioctl) that
 * include unit tests specific to the device. Switching on the device name
 * allows unit tests to be tailored to specific hardware on an availability
 * basis.
 */

#include "utils/StringUtils.h"

#include <string>

// Hauppauge WinTV-HVR 950Q hybrid TV stick
#define WINTVHVR950Q         "Auvitek AU8522 QAM/8VSB Frontend"
#define WINTVHVR2250         "Samsung S5H1411 QAM/8VSB Frontend"

namespace VDR
{

class cDvbDevices
{
public:
  static bool IsATSC(const std::string& deviceName)
  {
    return StringUtils::StartsWith(deviceName, WINTVHVR950Q) ||
           StringUtils::StartsWith(deviceName, WINTVHVR2250);
  }
};

}
