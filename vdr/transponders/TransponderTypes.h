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

#include "devices/linux/DVBLegacy.h"
#include "dvb/extended_frontend.h"
#include <linux/dvb/frontend.h>

// TODO: Remove me, left over from VDR
// Consider transponder frequencies equal if they are < 4 MHz apart
#include <cmath>
#define IS_TRANSPONDER(f1, f2)  (::std::abs((f1) - (f2)) <= 4)

namespace VDR
{

enum TRANSPONDER_TYPE
{
  TRANSPONDER_INVALID,
  TRANSPONDER_ATSC,
  TRANSPONDER_CABLE,
  TRANSPONDER_SATELLITE,
  TRANSPONDER_TERRESTRIAL,
};

// TODO: Adopt API-independent types
// Bandwidth
enum eTerrBandwidths {
  eBandwidth8Mhz = 0,
  eBandwidth7Mhz = 1,
  eBandwidth6Mhz = 2,
  eBandwidth5Mhz = 3,
  eBandwidthAuto = 4
};

// CodeRate
enum eCableSatCodeRates {
  eCoderateAuto = 0,
  eCoderate12   = 1,
  eCoderate23   = 2,
  eCoderate34   = 3,
  eCoderate56   = 4,
  eCoderate78   = 5,
  eCoderate89   = 6,
  eCoderate35   = 7,
  eCoderate45   = 8,
  eCoderate910  = 9,
  eCoderateNone = 15
};

enum eTerrCodeRates {
  eTerrCoderate12   = 0,
  eTerrCoderate23   = 1,
  eTerrCoderate34   = 2,
  eTerrCoderate56   = 3,
  eTerrCoderate78   = 4,
  eTerrCoderateAuto = 5,
  eTerrCoderateNone = 6
};

// DeliverySystem
enum eSatSystems {
  eDvbs  = 0,
  eDvbs2 = 1
};

// Guard
enum eTerrGuardIntervals {
  eGuardinterval32   = 0,
  eGuardinterval16   = 1,
  eGuardinterval8    = 2,
  eGuardinterval4    = 3,
  eGuardintervalAuto = 4
};

// Hierarchy
enum eTerrHierarchies {
  eHierarchyNone = 0,
  eHierarchy1    = 1,
  eHierarchy2    = 2,
  eHierarchy4    = 3,
  eHierarchyAuto = 8
};

// Inversion
enum eCableTerrInversions {
  eInversionOff  = 0,
  eInversionOn   = 1,
  eInversionAuto = 2
};

// Modulation
enum eCableModulations {
  eModulationQamAuto = 0,
  eModulationQam16   = 1,
  eModulationQam32   = 2,
  eModulationQam64   = 3,
  eModulationQam128  = 4,
  eModulationQam256  = 5,
  eModulationQam512  = 6,
  eModulationQam1024 = 7
};

enum eAtscModulations {
  eVsb8   = 0,
  eVsb16  = 1,
  eQam64  = 2,
  eQam256 = 3,
  eQamAuto= 4,
};

enum eSatModulationTypes {
  eSatModulationAuto  = 0,
  eSatModulationQpsk  = 1,
  eSatModulation8psk  = 2,
  eSatModulationQam16 = 3
};

enum eTerrConstellations {
  eModulationQpsk = 0,
  eModulation16   = 1,
  eModulation64   = 2,
  eModulationAuto = 3
};

// Polarization
enum eSatPolarizations {
  eHorizontal = 0,
  eVertical   = 1,
  eLeft       = 2,
  eRight      = 3
};

// RollOff
enum eSatRollOffs {
  eRolloff35   = 0,
  eRolloff25   = 1,
  eRolloff20   = 2,
  eRolloffAuto = 3
};

// Transmission
enum eTerrTransmissionModes {
  eTransmissionmode2K   = 0,
  eTransmissionmode8K   = 1,
  eTransmissionmode4K   = 2,
  eTransmissionmodeAuto = 3
};

}
