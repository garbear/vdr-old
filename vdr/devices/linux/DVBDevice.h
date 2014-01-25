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

#include "devices/Device.h"
#include "DVBLegacy.h"
#include "DVBTuner.h"
#include "sources/linux/DVBSourceParams.h"
#include "platform/threads/threads.h"
#include "utils/Observer.h"

#include <linux/dvb/frontend.h>
#include <stdint.h>
#include <string>
#include <vector>

//#define MAXDVBDEVICES      8
#define MAXDELIVERYSYSTEMS 8

#define DEV_VIDEO         "/dev/video"
#define DEV_DVB_BASE      "/dev/dvb"
#define DEV_DVB_ADAPTER   "adapter"
#define DEV_DVB_OSD       "osd"
#define DEV_DVB_FRONTEND  "frontend"
#define DEV_DVB_DVR       "dvr"
#define DEV_DVB_DEMUX     "demux"
#define DEV_DVB_VIDEO     "video"
#define DEV_DVB_AUDIO     "audio"
#define DEV_DVB_CA        "ca"

class cDvbAudioSubsystem;
class cDvbChannelSubsystem;
class cDvbCommonInterfaceSubsystem;
class cDvbPIDSubsystem;
class cDvbReceiverSubsystem;
class cDvbSectionFilterSubsystem;

/// The cDvbDevice implements a DVB device which can be accessed through the Linux DVB driver API.

class cDvbDevice : public cDevice, public Observable, public Observer
{
public:
  cDvbDevice(unsigned int adapter, unsigned int frontend);
  virtual ~cDvbDevice();

  /*!
   * \brief Discover DVB devices on the system. This scans for frontends of the
   *        format /dev/dvb/adapterN/frontendM.
   * \return A vector of uninitialised devices instantiated from the discovered
   *         frontend nodes.
   */
  static DeviceVector FindDevices();

  unsigned int Adapter() const { return m_adapter; }
  unsigned int Frontend() const { return m_frontend; }

  /*!
   * \brief Computes the subsystem ID for this device.
   */
  unsigned int GetSubsystemId() const;

  virtual bool Ready();

  virtual std::string DeviceType() const;
  virtual std::string DeviceName() const;

  /*!
   * \brief Bonds this device with the given device, making both of them use the
   *        same satellite cable and LNB
   * \return True if the bonding was successful
   *
   * Only DVB-S(2) devices can be bonded. When this function is called, the
   * calling device must not be bonded to any other device. The given device,
   * however, may already be bonded to an other device. That way several devices
   * can be bonded together.
   */
  bool Bond(cDvbDevice *Device);

  /*!
   * \brief Removes this device from any bonding it might have with other devices
   *
   * If this device is not bonded with any other device, nothing happens.
   */
  void UnBond();

  /*!
   * \brief Returns true if this device is either not bonded to any other device,
   *        or the given Channel is on the same satellite, polarization and band
   *        as those the bonded devices are tuned to (if any)
   * \param bConsiderOccupied If true, any bonded devices that are currently
   *        occupied but not otherwise receiving will cause this function to
   *        return false
   */
  bool BondingOk(const cChannel &channel, bool bConsiderOccupied = false) const;

  /*!
   * \brief Bonds the devices as defined in the given string
   * \param bondings The bondings string
   * \return False if an error occurred
   *
   * A bonding is a sequence of device numbers (starting at 1), separated by '+'
   * characters. Several bondings are separated by commas, as in "1+2,3+4+5".
   */
  static bool BondDevices(const char *bondings);

  /*!
   * \brief Unbonds all devices
   */
  static void UnBondDevices();

  cDvbChannelSubsystem         *DvbChannel() const;
  cDvbCommonInterfaceSubsystem *DvbCommonInterface() const;
  cDvbPIDSubsystem             *DvbPID() const;
  cDvbReceiverSubsystem        *DvbReceiver() const;
  cDvbSectionFilterSubsystem   *DvbSectionFilter() const;

  // TODO: Move to cDvbTransponderParams
  static std::vector<std::string> GetModulationsFromCaps(fe_caps_t caps);

  bool HasCam(void) const { return m_fd_ca > 0; }
  bool Initialise(void);
  void Notify(const Observable &obs, const ObservableMessage msg);

  /*!
   * \brief Translate a vector of delivery system enums into a comma-separated string
   * \return Comma-separated list of delivery systems, e.g. "ATSC,DVB-C"
   */
  static std::string TranslateDeliverySystems(const std::vector<fe_delivery_system>& deliverySystems);

protected:
public: // TODO
  static std::string DvbName(const char *name, unsigned int adapter, unsigned int frontend);
  int DvbOpen(const char *name, int mode) const;

protected:
  static bool Exists(unsigned int adapter, unsigned int frontend);

private:
  /*!
   * \brief Create a struct with the allocated subsystems
   * \param device The owner of the subsystems
   * \return A fully-allocated cSubsystems struct. Must be freed by calling cSubsystems::Free()
   */
  static cSubsystems CreateSubsystems(cDvbDevice* device);

  // Delivery systems translation table
  static const char*   DeliverySystemNames[];

  unsigned int         m_adapter;
  unsigned int         m_frontend;

public://TODO
  cDvbTuner            m_dvbTuner;
private:
  //int                  m_fd_dvr; // (Moved to DVBReceiverSubsystem.h)
  int                  m_fd_ca;

public: // TODO
  cDvbDevice*          m_bondedDevice;
  mutable bool         m_bNeedsDetachBondedReceivers;

public: // TODO
  static PLATFORM::CMutex m_bondMutex;
};
