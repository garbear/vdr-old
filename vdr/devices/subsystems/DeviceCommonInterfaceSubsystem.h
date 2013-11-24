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

#include "devices/DeviceSubsystem.h"

#include <time.h>

class cCamSlot;

class cDeviceCommonInterfaceSubsystem : protected cDeviceSubsystem
{
public:
  cDeviceCommonInterfaceSubsystem(cDevice *device);
  virtual ~cDeviceCommonInterfaceSubsystem() { }

  /*!
   * \brief Returns true if this device has a Common Interface
   */
  virtual bool HasCi() { return false; }

  /*!
   * \brief Returns true if this device handles encrypted channels itself
   *        without VDR assistance
   *
   * This can be e.g. if the device is a client that gets the stream from
   * another VDR instance that has already decrypted the stream. In this case
   * ProvidesChannel() shall check whether the channel can be decrypted.
   */
  virtual bool HasInternalCam() { return false; }

  /*!
   * \brief Sets the given CamSlot to be used with this device
   */
  void SetCamSlot(cCamSlot *camSlot) { m_camSlot = camSlot; }

  /*!
   * \brief Returns the CAM slot that is currently used with this device, or
   *        NULL if no CAM slot is in use.
   */
  cCamSlot *CamSlot() const { return m_camSlot; }

private:
public: // TODO
  cCamSlot *m_camSlot;
  time_t    m_startScrambleDetection;
};
