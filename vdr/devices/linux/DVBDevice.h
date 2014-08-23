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

#include "devices/Device.h"
#include "devices/DeviceTypes.h"
#include "DVBLegacy.h"
#include "DVBTuner.h"
#include "lib/platform/threads/threads.h"
#include "utils/Observer.h"

#include <linux/dvb/frontend.h>
#include <stdint.h>
#include <string>

#if defined(TARGET_ANDROID)
  #define DEV_DVB_BASE    "/dev"
  #define DEV_DVB_ADAPTER "dvb"
#else
  #define DEV_DVB_BASE    "/dev/dvb"
  #define DEV_DVB_ADAPTER "adapter"
#endif
#define DEV_DVB_FRONTEND  "frontend"
#define DEV_DVB_OSD       "osd"
#define DEV_DVB_DVR       "dvr"
#define DEV_DVB_DEMUX     "demux"
#define DEV_DVB_VIDEO     "video"
#define DEV_DVB_AUDIO     "audio"
#define DEV_DVB_CA        "ca"

namespace VDR
{

class cDvbAudioSubsystem;
class cDvbChannelSubsystem;
class cDvbCommonInterfaceSubsystem;
class cDvbReceiverSubsystem;
class cDvbSectionFilterSubsystem;

class cDvbDevice : public cDevice, public Observer
{
public:
  cDvbDevice(unsigned int adapter, unsigned int frontend);
  virtual ~cDvbDevice(void);

  unsigned int Adapter() const { return m_adapter; }
  unsigned int Frontend() const { return m_frontend; }
  std::string DvbPath(const char *dvbDeviceName) const;
  virtual std::string ID() const;

  /*!
   * \brief Discover DVB devices on the system. This scans for frontends of the
   *        format /dev/dvb/adapterN/frontendM.
   * \return A vector of uninitialised devices instantiated from the discovered
   *         frontend nodes.
   */
  static DeviceVector FindDevices();

  virtual bool Initialise(unsigned int index);

  // Available after calling Initialise()
  virtual std::string Name() const;
  bool HasCam(void) const { return m_fd_ca > 0; }

  // Inherited from Observer
  virtual void Notify(const Observable &obs, const ObservableMessage msg);

  // Safely access subsystem DVB subclasses
  cDvbChannelSubsystem         *DvbChannel() const;
  cDvbCommonInterfaceSubsystem *DvbCommonInterface() const;
  cDvbReceiverSubsystem        *DvbReceiver() const;
  cDvbSectionFilterSubsystem   *DvbSectionFilter() const;

public: // TODO: Make private!
  cDvbTuner    m_dvbTuner;

private:
  /*!
   * \brief Create a struct with the allocated subsystems
   * \param device The owner of the subsystems
   * \return A fully-allocated cSubsystems struct. Must be freed by calling cSubsystems::Free()
   */
  static cSubsystems CreateSubsystems(cDvbDevice* device);

  static DeviceVector FindDevicesMdev(void);

  unsigned int m_adapter;
  unsigned int m_frontend;
  //int          m_fd_dvr; // (Moved to DVBReceiverSubsystem.h)
  int          m_fd_ca;
};
}
