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

#include <set>
#include <map>
#include <shared_ptr/shared_ptr.hpp>
#include <vector>

namespace VDR
{

class cDevice;
class iReceiver;

typedef VDR::shared_ptr<cDevice> DevicePtr;
typedef std::vector<DevicePtr>   DeviceVector;

class cPidResource;
typedef VDR::shared_ptr<cPidResource> PidResourcePtr;
typedef std::set<PidResourcePtr>      PidResourceSet;

typedef struct PidReceivers {
  PidResourcePtr       resource; //TODO don't need a shared_ptr for this
  std::set<iReceiver*> receivers;
} PidReceivers;

typedef std::map<uint16_t, PidReceivers*> PidResourceMap; // pid -> resource + receivers

}
