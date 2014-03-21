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

#include <sys/types.h>

namespace VDR
{
class Buffer;
class cFilter;
class cSectionHandler;
class cEitFilter;
class cPatFilter;
class cSdtFilter;
class cNitFilter;

class cDeviceSectionFilterSubsystem : protected cDeviceSubsystem
{
public:
  cDeviceSectionFilterSubsystem(cDevice *device);
  virtual ~cDeviceSectionFilterSubsystem() { }

  /*!
   * \brief Opens a file handle for the given filter data
   *
   * A derived device that provides section data must implement this function.
   */
  virtual int OpenFilter(u_short pid, u_char tid, u_char mask) { return -1; }

  /*!
   * \brief Reads data from a handle for the given filter
   *
   * A derived class need not implement this function, because this is done by
   * the default implementation.
   */
  virtual int ReadFilter(int handle, void *buffer, size_t length); // TODO: Switch to std::vector

  /*!
   * \brief Closes a file handle that has previously been opened by OpenFilter()
   *
   * If this is as simple as calling close(Handle), a derived class need not
   * implement this function, because this is done by the default implementation.
   */
  virtual void CloseFilter(int handle);

  /*!
   * \brief Attaches the given filter to this device
   */
  void AttachFilter(cFilter *filter);

  /*!
   * \brief Detaches the given filter from this device
   */
  void Detach(cFilter *filter);

protected:
  /*!
   * \brief A derived device that provides section data must call this function
   *        (typically in its constructor) to actually set up the section handler
   */
public: // TODO
  void StartSectionHandler();

  /*!
   * \brief A device that has called StartSectionHandler() must call this
   *        function (typically in its destructor) to stop the section handler
   */
  void StopSectionHandler();

//private: // TODO
  cSectionHandler *m_sectionHandler;
private:
  cEitFilter      *m_eitFilter;
  cPatFilter      *m_patFilter;
  cSdtFilter      *m_sdtFilter;
  cNitFilter      *m_nitFilter;
};
}
