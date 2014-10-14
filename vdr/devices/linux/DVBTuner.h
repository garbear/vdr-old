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

class cDvbTuner : public PLATFORM::CThread, public Observable
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

  int GetSignalStrength(void) const;
  int GetSignalQuality(void) const;

protected:
  virtual void* Process(void);

private:
  // Called on Open()
  bool SetProperties(void);
  static std::vector<fe_delivery_system_t> GetDeliverySystems(int fileDescriptor, const Version& apiVersion, fe_type_t fallbackType, bool bCan2G);
  static std::vector<fe_modulation_t> GetModulations(fe_caps_t capabilities);
  static bool GetSubsystemId(const std::string& frontendPath, unsigned int& subsystemId);
  static Version GetApiVersion(int fileDescriptor);

  bool TuneDevice(const cTransponder& transponder);

  void ResetToneAndVoltage(void);
  void ExecuteDiseqc(const cDiseqc* Diseqc, unsigned int* Frequency);
  bool IsBondedMaster(void) const { return false; } // TODO

  // Adjust for satellite receivers that offset the frequency
  unsigned int GetTunedFrequencyHz(const cTransponder& transponder); // TODO: const
  static std::string StatusToString(const fe_status_t status);

  cDvbDevice* const                 m_device;
  bool                              m_bIsOpen;

  // Properties available after calling Open()
  std::string                       m_strName;
  fe_caps_t                         m_capabilities;
  std::vector<fe_delivery_system_t> m_deliverySystems;
  std::vector<fe_modulation_t>      m_modulations;
  unsigned int                      m_subsystemId;
  Version                           m_apiVersion;

  // Tuning properties
  cTransponder                      m_transponder;
  volatile fe_status_t              m_status;
  bool                              m_tunedLock;
  bool                              m_tunedSignal;
  PLATFORM::CCondition<bool>        m_lockEvent;
  PLATFORM::CCondition<bool>        m_tuneCondition;
  PLATFORM::CMutex                  m_mutex;

  // Satellite properties
  const cDiseqc*                    m_lastDiseqc;
  const cScr*                       m_scr;
  bool                              m_bLnbPowerTurnedOn;
  PLATFORM::CMutex                  m_diseqcMutex;

  // Internal
  int                               m_fileDescriptor;
};

}
