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

#include "DeviceImageGrabSubsystem.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

cDeviceImageGrabSubsystem::cDeviceImageGrabSubsystem(cDevice *device)
 : cDeviceSubsystem(device)
{
}

bool cDeviceImageGrabSubsystem::GrabImageFile(const string &strFileName, bool bJpeg /* = true */, int quality /* = -1 */, int sizeX /* = -1 */, int sizeY /* = -1 */)
{
  int result = 0;
  int fd = open(strFileName.c_str(), O_WRONLY | O_CREAT | O_NOFOLLOW | O_TRUNC, DEFFILEMODE);
  if (fd >= 0)
  {
    int ImageSize;
    uchar *image = GrabImage(ImageSize, bJpeg, quality, sizeX, sizeY);
    if (image)
    {
      if (safe_write(fd, image, ImageSize) == ImageSize)
        isyslog("grabbed image to %s", strFileName.c_str());
      else
      {
        esyslog("ERROR (%s,%d): %s: %m", __FILE__, __LINE__, strFileName.c_str());
        result |= 1;
      }
      free(image);
    }
    else
      result |= 1;
    close(fd);
  }
  else
  {
    esyslog("ERROR (%s,%d): %s: %m", __FILE__, __LINE__, strFileName.c_str());
    result |= 1;
  }
  return result == 0;
}