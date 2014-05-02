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

#include "devices/DeviceSubsystem.h"

namespace VDR
{
enum eVideoSystem
{
  vsPAL,
  vsNTSC
};

enum eVideoDisplayFormat
{
  vdfPanAndScan,
  vdfLetterBox,
  vdfCenterCutOut
};

class cDeviceVideoFormatSubsystem : protected cDeviceSubsystem
{
public:
  cDeviceVideoFormatSubsystem(cDevice *device);
  virtual ~cDeviceVideoFormatSubsystem() { }

  /*!
   * \brief Sets the video display format to the given one (only useful if this
   *        device has an MPEG decoder)
   *
   * A derived class must first call the base class function!
   */
  virtual void SetVideoDisplayFormat(eVideoDisplayFormat videoDisplayFormat);

  /*!
   * \brief Sets the output video format to either 16:9 or 4:3 (only useful if
   *        this device has an MPEG decoder)
   */
  virtual void SetVideoFormat(bool bVideoFormat16_9) { }

  /*!
   * \brief Returns the video system of the currently displayed material
   * \return The video system (default is PAL)
   */
  virtual eVideoSystem GetVideoSystem() { return vsPAL; }

  /*!
   * \brief Returns the width, height and video aspect ratio of the currently
   *        displayed video material.
   * \param width The width in pixels (e.g. 720)
   * \param height The height in pixels (e.g. 576)
   * \param videoAspect The aspect ratio (e.g. 1.33333 for a 4:3 broadcast, or
   *        1.77778 for 16:9)
   *
   * The default implementation returns 0 for width and height and 1.0 for videoAspect.
   */
  virtual void GetVideoSize(int &width, int &height, double &videoAspect);

  /*!
   * \brief Returns the width, height and pixelAspect ratio the OSD should use
   *        to best fit the resolution of the output device
   *
   * If PixelAspect is not 1.0, the OSD may take this as a hint to scale its
   * graphics in a way that, e.g., a circle will actually show up as a circle on
   * the screen, and not as an ellipse. Values greater than 1.0 mean to stretch
   * the graphics in the vertical direction (or shrink it in the horizontal
   * direction, depending on which dimension shall be fixed). Values less than
   * 1.0 work the other way round. Note that the OSD is not guaranteed to
   * actually use this hint.
   */
  virtual void GetOsdSize(int &width, int &height, double &pixelAspect);
};
}
