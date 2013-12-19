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
#include "sources/linux/DVBSourceParams.h"
#include "platform/threads/threads.h"

#include <linux/dvb/frontend.h>
#include <string>

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

typedef struct
{
  unsigned long iAdapter;
  unsigned long iFrontend;
} linux_dvb_device_t;

class cDvbDevice : public cDevice
{
public:
  cDvbDevice(unsigned int adapter, unsigned int frontend);
  virtual ~cDvbDevice();

  /*!
   * \brief Initializes the DVB devices. Must be called before accessing any DVB functions.
   * \return True if any devices are available
   */
  static bool Initialize();

  virtual bool Ready();

  unsigned int Adapter() const { return m_adapter; }
  unsigned int Frontend() const { return m_frontend; }

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
   * \brief Get the version of the DVB driver actually in use (will be
   *        determined by the first device created)
   */
  static int GetDvbApiVersion() { return m_dvbApiVersion; }

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

  cDvbAudioSubsystem           *DvbAudio() const;
  cDvbChannelSubsystem         *DvbChannel() const;
  cDvbCommonInterfaceSubsystem *DvbCommonInterface() const;
  cDvbPIDSubsystem             *DvbPID() const;
  cDvbReceiverSubsystem        *DvbReceiver() const;
  cDvbSectionFilterSubsystem   *DvbSectionFilter() const;

protected:
public: // TODO
  static std::string DvbName(const char *name, unsigned int adapter, unsigned int frontend);
  static int DvbOpen(const char *name, unsigned int adapter, unsigned int frontend, int mode, bool bReportError = false);

private:
  static void FindAdapters(std::vector<linux_dvb_device_t>& nodes);
  static size_t InitialiseAdapters(std::vector<linux_dvb_device_t>& nodes);

  const cSubsystems &CreateSubsystems();

  /*!
   * \brief Checks whether the given adapter/frontend exists by opening
   *        /dev/dvb/adapterN/frontendM in read-only mode
   */
  static bool Exists(unsigned int adapter, unsigned int frontend);

  /*!
   * \brief Probes for existing DVB devices
   */
  static bool Probe(unsigned int adapter, unsigned int frontend);

  bool QueryDeliverySystems(int fd_frontend);

  /*!
   * \brief Determine the version of the running DVB API
   */
  static bool FindDvbApiVersion(int fd_frontend);

  cSubsystems       m_subsystems;
public: // TODO
  unsigned int      m_adapter;
  unsigned int      m_frontend;
  dvb_frontend_info m_frontendInfo;
  int               m_deliverySystems[MAXDELIVERYSYSTEMS];
  unsigned int      m_numDeliverySystems;
  unsigned int      m_numModulations;
  //int               m_fd_dvr; // (Moved to DVBReceiverSubsystem.h)
private:
  int               m_fd_ca;
public: // TODO
  cDvbDevice       *m_bondedDevice;
  mutable bool      m_bNeedsDetachBondedReceivers;

private:
  static int        m_dvbApiVersion;

public: // TODO
  static PLATFORM::CMutex m_bondMutex;
};
