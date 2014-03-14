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

#include "Types.h"
#include "devices/DeviceSubsystem.h"
#include "utils/Tools.h"

#include <string>

class cDeviceImageGrabSubsystem : protected cDeviceSubsystem
{
public:
  cDeviceImageGrabSubsystem(cDevice *device);
  virtual ~cDeviceImageGrabSubsystem() { }

  /*!
   * \brief Grabs the currently visible screen image
   *
   * \param size The size of the returned data block
   * \param bJpeg If true, it will write a JPEG file, otherwise a PNM file will be written
   * \param quality The compression factor for JPEG. 1 will create a very blocky
   *        and small image, 70..80 will yield reasonable quality images while
   *        keeping the image file size around 50 KB for a full frame. The
   *        default will create a big but very high quality image.
   * \param sizeX The number of horizontal pixels in the frame (default is the current screen width)
   * \param sizeY The number of vertical pixels in the frame (default is the current screen height)
   * \return A pointer to the grabbed image data, or NULL in case of an error
   *
   * The caller takes ownership of the returned memory and must free() it once
   * it isn't needed any more.
   */
  virtual uchar *GrabImage(int &size, bool bJpeg = true, int quality = -1, int sizeX = -1, int sizeY = -1) { return NULL; }

  /*!
   * \brief Calls GrabImage() and stores the resulting image in a file with the
   *        given name
   * \param strFileName The filename. The caller is responsible for making sure
   *        that the given filename doesn't overwrite an important other file.
   * \return True if all went well
   */
  bool GrabImageFile(const std::string &strFileName, bool bJpeg = true, int quality = -1, int sizeX = -1, int sizeY = -1);
};
