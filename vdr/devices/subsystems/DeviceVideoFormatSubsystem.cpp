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

#include "DeviceVideoFormatSubsystem.h"
#include "DeviceSPUSubsystem.h"
#include "Config.h"
#include "dvb/SPU.h"
#include "utils/Tools.h"

cDeviceVideoFormatSubsystem::cDeviceVideoFormatSubsystem(cDevice *device)
 : cDeviceSubsystem(device)
{
}

void cDeviceVideoFormatSubsystem::SetVideoDisplayFormat(eVideoDisplayFormat videoDisplayFormat)
{
  //XXX
//  cSpuDecoder *spuDecoder = SPU()->GetSpuDecoder();
//  if (spuDecoder)
//  {
//    if (g_setup.VideoFormat)
//      spuDecoder->setScaleMode(cSpuDecoder::eSpuNormal);
//    else
//    {
//      switch (videoDisplayFormat)
//      {
//      case vdfPanAndScan:
//        spuDecoder->setScaleMode(cSpuDecoder::eSpuPanAndScan);
//        break;
//      case vdfLetterBox:
//        spuDecoder->setScaleMode(cSpuDecoder::eSpuLetterBox);
//        break;
//      case vdfCenterCutOut:
//        spuDecoder->setScaleMode(cSpuDecoder::eSpuNormal);
//        break;
//      default:
//        esyslog("ERROR: invalid value for VideoDisplayFormat '%d'", videoDisplayFormat);
//        break;
//      }
//    }
//  }
}

void cDeviceVideoFormatSubsystem::GetVideoSize(int &width, int &height, double &videoAspect)
{
  width = 0;
  height = 0;
  videoAspect = 1.0;
}

void cDeviceVideoFormatSubsystem::GetOsdSize(int &width, int &height, double &pixelAspect)
{
  width = 720;
  height = 480;
  pixelAspect = 1.0;
}
