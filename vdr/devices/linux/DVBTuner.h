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

#include "channels/ChannelManager.h"
#include "platform/threads/threads.h"

#include <linux/dvb/frontend.h>
#include <stdint.h>
#include <string>
#include <time.h>

class cDvbDevice;
class cDvbTransponderParams;
class cDiseqc;
class cScr;

class cDvbTuner : public PLATFORM::CThread
{
public:
  cDvbTuner(cDvbDevice *device);
  virtual ~cDvbTuner() { Close(); }

  /*!
   * \brief Open the tuner and query basic info
   * \return If true, the tuner is ready to be used and m_deliverySystems is
   *         guaranteed to be non-empty
   */
  bool Open();
  bool IsOpen() const { return !m_deliverySystems.empty(); } // TODO: need boolean m_bOpen
  void Close();

  unsigned int Adapter() const; // Alias for device->Adapter()
  unsigned int Frontend() const; // Alias for device->Frontend()

  const std::string &Name() const { return m_strName; }
  fe_delivery_system FrontendType() const { return m_frontendType; }

  const cChannel &GetTransponder() const { return m_channel; }
  bool HasDeliverySystem(fe_delivery_system deliverySystem) const;
  bool HasCapability(fe_caps_t capability) const { return m_frontendInfo.caps & capability; }

  /*!
   * \brief Get the version of the DVB driver actually in use
   * \return The DVB API version. Compare to DVBAPIVERSION in DVBLegacy.h
   */
  uint32_t GetDvbApiVersion() const { return m_dvbApiVersion; }
  bool QueryDvbApiVersion();

  bool Bond(cDvbTuner *tuner);
  void UnBond();
  bool BondingOk(const cChannel &channel, bool bConsiderOccupied = false) const;

  bool IsTunedTo(const cChannel &channel) const;
  void SetChannel(const cChannel &channel);
  void ClearChannel();
  bool Locked(int timeoutMs = 0);
  int GetSignalStrength() const;
  int GetSignalQuality() const;

  static fe_delivery_system GetRequiredDeliverySystem(const cChannel &channel, const cDvbTransponderParams *dtp);

  int IoControl(unsigned long int request, void* param) const;

private:
  std::vector<fe_delivery_system> GetDeliverySystems() const;

  virtual void *Process();

  bool SetFrontendType(const cChannel *Channel);
  std::string GetBondingParams() const { return GetBondingParams(m_channel); }
  std::string GetBondingParams(const cChannel &channel) const;
  cDvbTuner *GetBondedMaster();
  bool IsBondedMaster() const { return !m_bondedTuner || m_bBondedMaster; }
  void ClearEventQueue() const;
  bool GetFrontendStatus(fe_status_t &Status) const;
  void ExecuteDiseqc(const cDiseqc *Diseqc, unsigned int *Frequency);
  void ResetToneAndVoltage();
  bool SetFrontend();

  static unsigned int FrequencyToHz(unsigned int f);

  enum eTunerStatus
  {
    tsIdle,
    tsSet,
    tsTuned,
    tsLocked
  };


  cDvbDevice* const           m_device;

  std::string                 m_strName;
  fe_delivery_system          m_frontendType;
  dvb_frontend_info           m_frontendInfo;
  int                         m_fileDescriptor;
  uint32_t                    m_dvbApiVersion;

public: // TODO
  std::vector<fe_delivery_system> m_deliverySystems;
  //std::vector<std::string>    m_modulations;
  unsigned int                m_numModulations;
private:
  int                         m_tuneTimeout;
  int                         m_lockTimeout;
  time_t                      m_lastTimeoutReport;
  cChannel                    m_channel;
  const cDiseqc*              m_lastDiseqc;
  const cScr*                 m_scr;
  bool                        m_bLnbPowerTurnedOn;
  eTunerStatus                m_tunerStatus;
  PLATFORM::CMutex            m_mutex;
  PLATFORM::CCondition<bool>  m_locked;
  PLATFORM::CCondition<bool>  m_newSet;
  cDvbTuner*                  m_bondedTuner;
  bool                        m_bBondedMaster;
  bool                        m_bLocked;
  bool                        m_bNewSet;

  static PLATFORM::CMutex     m_bondMutex;
};
