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

#include "platform/threads/threads.h"

#define MAX_CAM_SLOTS_PER_ADAPTER     8 // maximum possible value is 255

enum eModuleStatus
{
  msNone,
  msReset,
  msPresent,
  msReady
};

class cCamSlot;
class cDevice;

class cCiAdapter : public PLATFORM::CThread
{
  friend class cCamSlot;

public:
  cCiAdapter();
  // The derived class must call StopThread(3000) in its destructor
  virtual ~cCiAdapter();

  /*!
   * \brief Returns 'true' if all present CAMs in this adapter are ready
   */
  virtual bool Ready();

protected:
  /*!
   * \brief Handles the attached CAM slots in a separate thread
   *
   * The derived class must call the Start() function to actually start CAM
   * handling.
   */
  virtual void *Process();

  /*!
   * \brief Reads one chunk of data into the given Buffer, up to MaxLength bytes
   * \return The number of bytes read (in case of an error it will also return 0)
   *
   * If no data is available immediately, wait for up to CAM_READ_TIMEOUT.
   */
  virtual int Read(uint8_t *Buffer, int MaxLength) = 0;

  // Writes Length bytes of the given Buffer
  virtual void Write(const uint8_t *Buffer, int Length) = 0;

  /*!
   * \brief Resets the CAM in the given Slot
   * \return True if the operation was successful
   */
  virtual bool Reset(int Slot) = 0;

  /*!
   * \brief
   * \return The status of the CAM in the given Slot
   */
  virtual eModuleStatus ModuleStatus(int Slot) = 0;

  /*!
   * \brief Assigns this adapter to the given Device, if possible
   * \param Device If NULL, the adapter will be unassigned from any device it
   *        was previously assigned to. The value of Query is ignored in that
   *        case, and this function always returns 'true'
   * \param Query If true, the adapter only checks whether it can be assigned
   *        to the Device, but doesn't actually assign itself to it
   * \return True if the adapter can be assigned to the Device
   */
  virtual bool Assign(cDevice *Device, bool Query = false) = 0;

private:
  /*!
   * \brief Adds the given CamSlot to this CI adapter
   */
  void AddCamSlot(cCamSlot *camSlot);

  cDevice  *m_assignedDevice;
  cCamSlot *m_camSlots[MAX_CAM_SLOTS_PER_ADAPTER];
};
