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
#include "devices/DeviceTypes.h"
#include "lib/platform/threads/threads.h"
#include "transponders/Transponder.h"
#include "utils/Observer.h"
#include "utils/Version.h"

#include <algorithm>
#include <linux/dvb/frontend.h>
#include <stdint.h>
#include <string>

namespace VDR
{

class cDvbDevice;
class cDiseqc;
class cScr;

enum DVB_TUNER_STATE
{
  DVB_TUNER_STATE_NOT_INITIALISED,
  DVB_TUNER_STATE_ERROR,
  DVB_TUNER_STATE_IDLE,
  DVB_TUNER_STATE_TUNING,
  DVB_TUNER_STATE_TUNING_FAILED,
  DVB_TUNER_STATE_LOCKING,
  DVB_TUNER_STATE_LOCKED,
  DVB_TUNER_STATE_LOST_LOCK,
  DVB_TUNER_STATE_CANCEL,
};

class cDvbTuner : public PLATFORM::CThread, public Observable
{
  class cDvbTunerAction
  {
  public:
    cDvbTunerAction();
  };
public:
  cDvbTuner(cDvbDevice *device);
  virtual ~cDvbTuner(void) { Close(); }

  /*!
   * Open the tuner and query basic info. Returns true if the tuner is ready to
   * be used.
   */
  bool Open(void);
  bool IsOpen(void);
  void Close(void);

  /*!
   * Properties available after calling Open()
   *   - Name
   *   - Capabilities
   *   - Delivery Systems (the digital television systems offered by this tuner)
   *   - Modulations (TODO)
   *   - Version of the DVB API in use
   */
  const std::string&   Name(void) const { return m_strName; }
  fe_caps_t            Capabilities(void) const { return m_capabilities; }
  bool                 HasCapability(fe_caps_t capability) const { return m_capabilities & capability; }
  const std::vector<fe_delivery_system_t>& DeliverySystems(void) const { return m_deliverySystems; }
  bool                 HasDeliverySystem(fe_delivery_system_t deliverySystem) const;
  const std::vector<fe_modulation_t>& Modulations(void) const { return m_modulations; }
  bool                 HasModulation(fe_modulation_t modulation) const;
  unsigned int         SubsystemId(void) const { return m_subsystemId; }
  const Version&       ApiVersion(void) const { return m_apiVersion; }

  /*!
   * Switch the tuner to the specified transponder. Blocks until transponder is
   * switched (returns true) or the switch fails/times out (returns false).
   */
  bool Tune(const cTransponder& transponder);

  /*!
   * Get the transponder that the tuner is locking/has locked onto. Returns an
   * empty pointer if tuner is idling (before calling SetChannel() and after
   * calling ClearChannel()).
   */
  cTransponder GetTransponder(void) const;

  /*!
   * Check if the tuner has a lock on any channel.
   */
  bool HasLock(void) const { return m_status & FE_HAS_LOCK; }

  /*!
   * Check if the tuner found sync bytes on any channel.
   */
  bool IsSynced(void) const { return m_status & FE_HAS_SYNC; }

  /*!
   * Check if the tuner has a lock on a specified channel.
   */
  bool IsTunedTo(const cTransponder& transponder) const;

  /*!
   * Untune (detune?) the tuner.
   */
  void ClearTransponder(void);

  bool SignalQuality(signal_quality_info_t& info) const;

protected:
  virtual void* Process(void);

private:
  void SetQualityPercentage(signal_quality_info_t& info) const;
  void SetSignalStrength(signal_quality_info_t& info) const;

  // Called on Open()
  bool SetProperties(void);
  static std::vector<fe_delivery_system_t> GetDeliverySystems(int fileDescriptor, const Version& apiVersion, fe_type_t fallbackType, bool bCan2G);
  static std::vector<fe_modulation_t> GetModulations(fe_caps_t capabilities);
  static bool GetSubsystemId(const std::string& frontendPath, unsigned int& subsystemId);
  static Version GetApiVersion(int fileDescriptor);

  bool TuneDevice(void);

  void ResetToneAndVoltage(void);
  void ExecuteDiseqc(const cDiseqc* Diseqc, unsigned int* Frequency);
  bool IsBondedMaster(void) const { return false; } // TODO

  // Adjust for satellite receivers that offset the frequency
  unsigned int GetTunedFrequencyHz(const cTransponder& transponder); // TODO: const
  static std::string StatusToString(const fe_status_t status);

  bool InitialiseHardware(void);
  bool OpenFrontend(const std::string& strFrontendPath);
  void LogTunerInfo(void);
  inline bool CheckEvent(uint32_t iTimeoutMs = 0);
  void UpdateFrontendStatus(const fe_status_t& status);
  bool CancelTuning(uint32_t iTimeoutMs);
  DVB_TUNER_STATE State(void);

  cDvbDevice* const                 m_device;

  // Properties available after calling Open()
  std::string                       m_strName;
  fe_caps_t                         m_capabilities;
  std::vector<fe_delivery_system_t> m_deliverySystems;
  std::vector<fe_modulation_t>      m_modulations;
  unsigned int                      m_subsystemId;
  Version                           m_apiVersion;

  // Tuning properties
  cTransponder                      m_transponder;
  cTransponder                      m_nextTransponder;
  volatile fe_status_t              m_status;
  bool                              m_tunedLock;
  PLATFORM::CCondition<bool>        m_lockEvent;

  // Satellite properties
  const cDiseqc*                    m_lastDiseqc;
  const cScr*                       m_scr;
  bool                              m_bLnbPowerTurnedOn;
  PLATFORM::CMutex                  m_diseqcMutex;

  // Internal
  PLATFORM::CMutex                  m_tuneEventMutex;
  int                               m_fileDescriptor;
  PLATFORM::CCondition<bool>        m_tuneEventCondition;
  bool                              m_tuneEvent;

  PLATFORM::CMutex                  m_stateMutex;
  PLATFORM::CCondition<bool>        m_tunerIdleCondition;
  DVB_TUNER_STATE                   m_state;
  bool                              m_tunerIdle;
};

}
