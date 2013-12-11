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

#include "channels/Channels.h"
#include "platform/threads/threads.h"

#include <linux/dvb/frontend.h>
#include <stdint.h>
#include <string>
#include <time.h>

class cDvbDevice;
class cDvbTransponderParams;
class cDiseqc;
class cScr;

class cDvbTuner : public cThread
{
public:
  cDvbTuner(const cDvbDevice *device, int fd_frontend, unsigned int adapter, unsigned int frontend);
  virtual ~cDvbTuner();

  int FrontendType() const { return m_frontendType; }
  const cChannel &GetTransponder() const { return m_channel; }
  uint32_t SubsystemId() const { return m_subsystemId; }

  bool Bond(cDvbTuner *tuner);
  void UnBond();
  bool BondingOk(const cChannel &channel, bool bConsiderOccupied = false) const;

  bool IsTunedTo(const cChannel &channel) const;
  void SetChannel(const cChannel &channel);
  void ClearChannel();
  bool Locked(int timeoutMs = 0);
  int GetSignalStrength() const;
  int GetSignalQuality() const;

  static int GetRequiredDeliverySystem(const cChannel &channel, const cDvbTransponderParams *dtp);

private:
  virtual void Action();

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

  int                        m_frontendType;
  const cDvbDevice*          m_device;
  int                        m_fd_frontend;
  unsigned int               m_adapter;
  unsigned int               m_frontend;
  uint32_t                   m_subsystemId;
  int                        m_tuneTimeout;
  int                        m_lockTimeout;
  time_t                     m_lastTimeoutReport;
  cChannel                   m_channel;
  const cDiseqc*             m_lastDiseqc;
  const cScr*                m_scr;
  bool                       m_bLnbPowerTurnedOn;
  eTunerStatus               m_tunerStatus;
  PLATFORM::CMutex           m_mutex;
  PLATFORM::CCondition<bool> m_locked;
  PLATFORM::CCondition<bool> m_newSet;
  cDvbTuner*                 m_bondedTuner;
  bool                       m_bBondedMaster;
  bool                       m_bLocked;
  bool                       m_bNewSet;

  static PLATFORM::CMutex    m_bondMutex;
};
