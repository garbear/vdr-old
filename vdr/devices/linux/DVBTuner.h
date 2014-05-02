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

#include "channels/Channel.h"
#include "platform/threads/threads.h"

#include <linux/dvb/frontend.h>
#include <stdint.h>
#include <string>

namespace VDR
{

class cDvbDevice;
class cDiseqc;
class cScr;

class cDvbTuner : public PLATFORM::CThread
{
public:
  cDvbTuner(cDvbDevice *device);
  virtual ~cDvbTuner(void) { Close(); }

  /*!
   * Open the tuner and query basic info. Returns true if the tuner is ready to
   * be used.
   */
  bool Open(void);
  bool IsOpen(void) const { return m_bIsOpen; }
  void Close(void);

  /*!
   * Query info about the tuner. Should only be called after Open() succeeds.
   */
  const std::string&   GetName(void) const { return m_frontendInfo.strName; }
  fe_caps_t            GetCapabilities(void) const { return m_frontendInfo.capabilities; }
  bool                 HasCapability(fe_caps_t capability) const { return m_frontendInfo.capabilities & capability; }
  fe_delivery_system_t FrontendType(void) const { return m_frontendType; }
  std::string          DeviceType(void) const;
  bool                 HasDeliverySystem(fe_delivery_system_t deliverySystem) const;
  unsigned int         ModulationCount(void) const { return m_numModulations; }
  unsigned int         DeliverySystemCount(void) const { return m_deliverySystems.size(); }

  /*!
   * Switch the tuner to the specified channel. Blocks until channel is switched
   * (returns true) or the switch fails/times out (returns false).
   */
  bool SwitchChannel(const ChannelPtr& channel);

  /*!
   * Get the transponder that the tuner is locking/has locked onto. Returns an
   * empty pointer if tuner is idling (before calling SetChannel() and after
   * calling ClearChannel()).
   */
  ChannelPtr GetTransponder(void);

  /*!
   * Check if the tuner has a lock on any channel.
   */
  bool HasLock(void) const { return m_frontendStatus & FE_HAS_LOCK; }

  /*!
   * Check if the tuner has a lock on a specified channel.
   */
  bool IsTunedTo(const cChannel& channel) const;

  /*!
   * Untune (detune?) the tuner.
   */
  void ClearChannel(void);

  int GetSignalStrength(void) const;
  int GetSignalQuality(void) const;

  bool Bond(cDvbTuner* tuner);
  void UnBond(void);
  bool BondingOk(const cChannel& channel, bool bConsiderOccupied = false) const;

  static fe_delivery_system_t GetRequiredDeliverySystem(const cChannel& channel);

protected:
  /*!
   * Tight loop to monitor the status of the frontend and fire an event when
   * changed. Loop is active after calling SwitchChannel() (while tuning/locked)
   * and until ClearChannel() is called.
   */
  virtual void* Process(void);

private:
  /*!
   * Get the version of the DVB driver actually in use. Compare to DVBAPIVERSION
   * in DVBLegacy.h
   */
  uint32_t GetDvbApiVersion(void) const;

  std::vector<fe_delivery_system_t> GetDeliverySystems(void) const;

  /*!
   * Set m_channel and reset other parameters in preparation for a channel switch.
   */
  void SetChannel(const ChannelPtr& channel);

  void ClearEventQueue(void) const;

  bool GetFrontendStatus(fe_status_t &Status) const;

  /*!
   * Instruct the frontend to tune to a channel.
   */
  bool SetFrontend(const ChannelPtr& channel);

  void ResetToneAndVoltage(void);

  void ExecuteDiseqc(const cDiseqc* Diseqc, unsigned int* Frequency);

  std::string GetBondingParams(void) const { return GetBondingParams(*m_channel); }
  std::string GetBondingParams(const cChannel& channel) const;
  cDvbTuner *GetBondedMaster(void);
  bool IsBondedMaster(void) const { return !m_bondedTuner || m_bBondedMaster; }

  /*!
   * Translate delivery system enums into a comma-separated string of delivery
   * systems, e.g. "ATSC,DVB-C".
   */
  static std::string TranslateDeliverySystems(const std::vector<fe_delivery_system_t>& deliverySystems);

  /*!
   * Return the maximum time to wait for a lock for the given frontend type.
   */
  static unsigned int GetLockTimeout(fe_delivery_system_t frontendType);

  struct FrontendInfo
  {
    std::string strName;
    fe_caps_t   capabilities;
    fe_type_t   type; // Deprecated, for legacy mode (DVB-API < 5.5)
  };

  cDvbDevice* const                 m_device;
  FrontendInfo                      m_frontendInfo;
  fe_delivery_system_t              m_frontendType;
  bool                              m_bIsOpen;
  int                               m_fileDescriptor;
  uint32_t                          m_dvbApiVersion;
  std::vector<fe_delivery_system_t> m_deliverySystems; // Will be non-empty if m_bIsOpen == true
  unsigned int                      m_numModulations;
  ChannelPtr                        m_channel;         // Channel currently tuned/tuning to
  fe_status_t                       m_frontendStatus;
  PLATFORM::CEvent                  m_statusChangeEvent;
  PLATFORM::CEvent                  m_sleepEvent;      // Wait on this event in thread loop
  PLATFORM::CMutex                  m_mutex;

  const cDiseqc*                    m_lastDiseqc;
  const cScr*                       m_scr;
  bool                              m_bLnbPowerTurnedOn;
  cDvbTuner*                        m_bondedTuner;
  bool                              m_bBondedMaster;

  static PLATFORM::CMutex           m_bondMutex;
};

}
