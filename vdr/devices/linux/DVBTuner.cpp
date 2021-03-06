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

#include "DVBTuner.h"
#include "DVBDevice.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
//#include "devices/subsystems/DevicePIDSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "dvb/DiSEqC.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/Poller.h"
#include "lib/platform/util/timeutils.h"
#include "settings/Settings.h"
#include "transponders/Stringifier.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

using namespace std;
using namespace PLATFORM;

#define FE_STATUS_UNKNOWN          ((fe_status_t)0x00)

#define INVALID_FD                 -1

#define DVBS_LOCK_TIMEOUT_MS       3000
#define DVBC_LOCK_TIMEOUT_MS       3000
#define DVBT_LOCK_TIMEOUT_MS       3000
#define ATSC_LOCK_TIMEOUT_MS       3000

#define TUNER_POLL_TIMEOUT_MS      100 // tuning usually takes about a second

#define DEBUG_SIGNALQUALITY        1

#define TUNE_DELAY_MS              100 // Some drivers report stale status immediately after tuning

namespace VDR
{

enum AM_AMX_SOURCE
{
  AM_DMX_SRC_TS0,
  AM_DMX_SRC_TS1,
  AM_DMX_SRC_TS2,
  AM_DMX_SRC_HIU
};

/*!
 * Helper class to perform frontend commands
 */
class cFrontendCommands
{
public:
  cFrontendCommands(int fileDescriptor) : m_fd(fileDescriptor) { }

  void AddCommand(uint32_t command, uint32_t data = 0)
  {
    dtv_property frontend = { };
    frontend.cmd = command;
    frontend.u.data = data;
    m_frontendCmds.push_back(frontend);
  }

  // https://github.com/MythTV/mythtv/blob/fc82ee5/mythtv/libs/libmythtv/dvbchannel.cpp
  bool Execute(void)
  {
    dtv_properties cmdSeq = { (uint32_t)m_frontendCmds.size(), m_frontendCmds.data() };
    bool bSuccess = (ioctl(m_fd, FE_SET_PROPERTY, &cmdSeq) >= 0);
    m_frontendCmds.clear();

    if (!bSuccess)
    {
      esyslog("FE_SET_PROPERTY returned %m");
      return false;
    }
    return true;
  }

private:
  const int            m_fd;           // File descriptor
  vector<dtv_property> m_frontendCmds; // Command sequence
};

cDvbTuner::cDvbTuner(cDvbDevice *device)
 : m_device(device),
   m_capabilities(FE_IS_STUPID),
   m_subsystemId(0),
   m_apiVersion(Version::EmptyVersion),
   m_status(FE_STATUS_UNKNOWN),
   m_tunedLock(false),
   m_lastDiseqc(NULL),
   m_scr(NULL),
   m_bLnbPowerTurnedOn(false),
   m_fileDescriptor(INVALID_FD),
   m_tuneEvent(false),
   m_state(DVB_TUNER_STATE_NOT_INITIALISED),
   m_tunerIdle(true)
{
  assert(m_device);
}

bool cDvbTuner::HasDeliverySystem(fe_delivery_system_t deliverySystem) const
{
  return std::find(m_deliverySystems.begin(), m_deliverySystems.end(), deliverySystem) != m_deliverySystems.end();
}

bool cDvbTuner::HasModulation(fe_modulation_t modulation) const
{
  return std::find(m_modulations.begin(), m_modulations.end(), modulation) != m_modulations.end();
}

bool cDvbTuner::OpenFrontend(const std::string& strFrontendPath)
{
  errno = 0;
  while (INVALID_FD == (m_fileDescriptor = open(strFrontendPath.c_str(), O_RDWR | O_NONBLOCK)))
  {
    if (errno == EINTR)
    {
      esyslog("DVB tuner: Failed to open frontend: Interrupted system call. Trying again");
      continue;
    }
    else if (errno == EBUSY)
    {
      // TODO: If error is "Device or resource busy", kill the owner process
      esyslog("DVB tuner: Frontend is held by another process (try `lsof +D /dev/dvb`)");
      return false;
    }
    else
    {
      esyslog("DVB tuner: Can't open frontend (errno=%d)", errno);
      m_state = DVB_TUNER_STATE_ERROR;
      return false;
    }
  }
  return true;
}

void cDvbTuner::LogTunerInfo(void)
{
  isyslog("DVB tuner: Detected DVB API version %s", m_apiVersion.c_str());

  // Log delivery systems and modulations
  vector<string> vecDeliverySystems;
  for (vector<fe_delivery_system_t>::const_iterator it = m_deliverySystems.begin(); it != m_deliverySystems.end(); ++it)
    vecDeliverySystems.push_back(Stringifier::DeliverySystemToString(*it));
  string strDeliverySystems = StringUtils::Join(vecDeliverySystems, ", ");

  vector<string> vecModulations;
  for (vector<fe_modulation_t>::const_iterator it = m_modulations.begin(); it != m_modulations.end(); ++it)
    vecModulations.push_back(Stringifier::ModulationToString(*it));
  string strModulations = StringUtils::Join(vecModulations, ", ");
  if (strModulations.empty())
    strModulations = "unknown modulations";

  isyslog("Dvb tuner: provides %s with %s", strDeliverySystems.c_str(), strModulations.c_str());
}

bool cDvbTuner::Open(void)
{
  CLockObject lock(m_stateMutex);
  if (m_state != DVB_TUNER_STATE_NOT_INITIALISED)
    return true;

  isyslog("Opening DVB tuner %s", m_device->DvbPath(DEV_DVB_FRONTEND).c_str());

  if (!OpenFrontend(m_device->DvbPath(DEV_DVB_FRONTEND)) ||
      !SetProperties() ||
      !InitialiseHardware() ||
      !CreateThread(false))
  {
    m_state = DVB_TUNER_STATE_ERROR;
  }
  else
  {
    LogTunerInfo();
    m_state = DVB_TUNER_STATE_IDLE;
  }

  return true;
}

bool cDvbTuner::IsOpen(void)
{
  CLockObject lock(m_stateMutex);
  return m_state != DVB_TUNER_STATE_ERROR &&
      m_state != DVB_TUNER_STATE_NOT_INITIALISED &&
      !IsStopped();
}

bool cDvbTuner::SetProperties(void)
{
  dvb_frontend_info frontendInfo = { };
  if (ioctl(m_fileDescriptor, FE_GET_INFO, &frontendInfo) < 0 || frontendInfo.name == NULL)
  {
    esyslog("DVB tuner: Can't get frontend info");
    return false;
  }

  // Name
  m_strName = frontendInfo.name;

  // Capabilities
  m_capabilities = frontendInfo.caps;

  // API Version
  m_apiVersion = GetApiVersion(m_fileDescriptor);
  if (m_apiVersion.empty())
    return false;

  // Delivery Systems
  m_deliverySystems = GetDeliverySystems(m_fileDescriptor, m_apiVersion, frontendInfo.type, HasCapability(FE_CAN_2G_MODULATION));
  if (m_deliverySystems.empty())
    return false;

  // Modulations
  m_modulations = GetModulations(frontendInfo.caps);

  // Subsystem ID
  if (!GetSubsystemId(m_device->DvbPath(DEV_DVB_FRONTEND), m_subsystemId))
    return false;

  return true;
}

vector<fe_delivery_system_t> cDvbTuner::GetDeliverySystems(int fileDescriptor, const Version& apiVersion, fe_type_t fallbackType, bool bCan2G)
{
  vector<fe_delivery_system_t> deliverySystems;

  bool bLegacyMode = true;
  if (apiVersion >= "5.5")
  {
    dtv_property frontend[1] = { };
    frontend[0].cmd = DTV_ENUM_DELSYS;

    dtv_properties cmdSeq = { ARRAY_SIZE(frontend), frontend };

    if (ioctl(fileDescriptor, FE_GET_PROPERTY, &cmdSeq) >= 0 && frontend[0].u.data != 0)
    {
      for (uint8_t* deliverySystem = frontend[0].u.buffer.data; deliverySystem != frontend[0].u.buffer.data + frontend[0].u.buffer.len; deliverySystem++)
        deliverySystems.push_back(static_cast<fe_delivery_system_t>(*deliverySystem));

      if (deliverySystems.empty())
        esyslog("DVB tuner: No delivery systems");

      bLegacyMode = false;
    }
    else
    {
      esyslog("DVB tuner: can't query delivery systems - falling back to legacy mode");
    }
  }

  if (bLegacyMode)
  {
    // Legacy mode (DVB-API < 5.5)
    switch (fallbackType)
    {
    case FE_QPSK:
      deliverySystems.push_back(SYS_DVBS);
      if (bCan2G)
        deliverySystems.push_back(SYS_DVBS2);
      break;
    case FE_OFDM:
      deliverySystems.push_back(SYS_DVBT);
      if (bCan2G)
        deliverySystems.push_back(SYS_DVBT2);
      break;
    case FE_QAM:
      deliverySystems.push_back(SYS_DVBC_ANNEX_A); // TODO: what about SYS_DVBC_ANNEX_C?
      break;
    case FE_ATSC:
      deliverySystems.push_back(SYS_ATSC);
      break;
    default:
      esyslog("DVB tuner: unknown frontend type %d", fallbackType);
      break;
    }
  }

  return deliverySystems;
}

vector<fe_modulation_t> cDvbTuner::GetModulations(fe_caps_t capabilities)
{
  vector<fe_modulation_t> modulations;

  if (capabilities & FE_CAN_QPSK)      { modulations.push_back(QPSK); }
  if (capabilities & FE_CAN_QAM_16)    { modulations.push_back(QAM_16); }
  if (capabilities & FE_CAN_QAM_32)    { modulations.push_back(QAM_32); }
  if (capabilities & FE_CAN_QAM_64)    { modulations.push_back(QAM_64); }
  if (capabilities & FE_CAN_QAM_128)   { modulations.push_back(QAM_128); }
  if (capabilities & FE_CAN_QAM_256)   { modulations.push_back(QAM_256); }
  if (capabilities & FE_CAN_8VSB)      { modulations.push_back(VSB_8); }
  if (capabilities & FE_CAN_16VSB)     { modulations.push_back(VSB_16); }
  //if (capabilities & FE_CAN_TURBO_FEC) { modulations.push_back("TURBO_FEC"); }

  return modulations;
}

bool cDvbTuner::GetSubsystemId(const string& frontendPath, unsigned int& subsystemId)
{
  bool bFoundSubsystemId = false;

  struct __stat64 st;
  if (CFile::Stat(frontendPath, &st) != 0)
  {
    esyslog("Dvb tuner: failed to stat %s for subsystem ID", frontendPath.c_str());
  }
  else
  {
    DirectoryListing items;
    if (CDirectory::GetDirectory("/sys/class/dvb", items))
    {
      for (DirectoryListing::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
      {
        string name = itItem->Name(); // name looks like "dvb0.frontend0"
        if (name.find(DEV_DVB_FRONTEND) == string::npos)
          continue;

        string frontendDev = StringUtils::Format("/sys/class/dvb/%s/dev", name.c_str());
        CFile file;
        string line;
        if (!file.Open(frontendDev) || !file.ReadLine(line))
          continue;

        vector<string> devParts;
        StringUtils::Split(line, ":", devParts);
        if (devParts.size() <= 2)
          continue;

        unsigned int major = StringUtils::IntVal(devParts[0]);
        unsigned int minor = StringUtils::IntVal(devParts[1]);
        unsigned int version = ((major << 8) | minor);
        if (version != st.st_rdev)
          continue;

        if (file.Open(StringUtils::Format("/sys/class/dvb/%s/device/subsystem_device", name.c_str())) && file.ReadLine(line))
        {
          subsystemId = StringUtils::IntVal(line);
          bFoundSubsystemId = true;
          break;
        }
        else if (file.Open(StringUtils::Format("/sys/class/dvb/%s/device/subsystem_vendor", name.c_str())) && file.ReadLine(line))
        {
          subsystemId = StringUtils::IntVal(line) << 16;
          bFoundSubsystemId = true;
          break;
        }
      }
    }
  }

  if (!bFoundSubsystemId)
  {
    esyslog("Dvb tuner: could not determine subsystem ID");
    //return false; // OK to not have a subsystem ID
    subsystemId = 0;
  }

  return true;
}

Version cDvbTuner::GetApiVersion(int fileDescriptor)
{
  Version apiVersion(Version::EmptyVersion);

  dtv_property frontend[1] = { };
  frontend[0].cmd = DTV_API_VERSION;

  dtv_properties cmdSeq = { ARRAY_SIZE(frontend), frontend };

  if (ioctl(fileDescriptor, FE_GET_PROPERTY, &cmdSeq) >= 0 && frontend[0].u.data != 0)
    apiVersion = StringUtils::Format("%d.%d", frontend[0].u.data >> 8, frontend[0].u.data & 0xff);
  else
    esyslog("DVB tuner: Can't get DVB API version (errno=%d)", errno);

  return apiVersion;
}

void cDvbTuner::Close(void)
{
  CLockObject lock(m_stateMutex);

  if (IsOpen())
  {
    isyslog("DVB tuner: Closing frontend");
    StopThread(-1);

    {
      CLockObject lock2(m_tuneEventMutex);
      CancelTuning(2000);
      m_state = DVB_TUNER_STATE_NOT_INITIALISED;
      m_tuneEvent = true;
      m_tuneEventCondition.Signal();
    }

    close(m_fileDescriptor);
    m_fileDescriptor = INVALID_FD;
    m_transponder.Reset();
    m_nextTransponder.Reset();

    /* looks like this irritates the SCR switch, so let's leave it out for now
    if (m_lastDiseqc && m_lastDiseqc->IsScr())
    {
      unsigned int frequency = 0;
      ExecuteDiseqc(m_lastDiseqc, &frequency);
    }
    */
  }
}

static unsigned int GetLockTimeout(TRANSPONDER_TYPE frontendType)
{
  switch (frontendType)
  {
  case TRANSPONDER_ATSC:
    return ATSC_LOCK_TIMEOUT_MS;
  case TRANSPONDER_CABLE:
    return DVBC_LOCK_TIMEOUT_MS;
  case TRANSPONDER_SATELLITE:
    return DVBS_LOCK_TIMEOUT_MS;
  case TRANSPONDER_TERRESTRIAL:
    return DVBT_LOCK_TIMEOUT_MS;
  default:
    return 0; // Unknown DVB frontend type, don't wait for a lock
  }
}

bool cDvbTuner::Tune(const cTransponder& transponder)
{
  CLockObject lock(m_stateMutex);
  if (!IsOpen())
  {
    esyslog("tuning failed - tuner is not open");
    return false;
  }

  if (!CancelTuning(GetLockTimeout(transponder.Type())))
  {
    esyslog("tuning failed - failed to cancel previous tuning");
    return false;
  }

  /** tune to new transponder */
  {
    CLockObject lock2(m_tuneEventMutex);
    SetState(DVB_TUNER_STATE_TUNING);
    m_nextTransponder = transponder;
    m_tuneEvent = true;
    m_tunedLock = false;
    lock.Unlock(); //TODO use something smarter...
    m_tuneEventCondition.Signal();
    lock.Lock();
  }

  /** wait for a lock */
  unsigned int lockTimeoutMs(GetLockTimeout(transponder.Type()));
  CTimeout timeout(lockTimeoutMs);
  if (m_lockEvent.Wait(m_stateMutex, m_tunedLock, timeout.TimeLeft()))
  {
    if (m_transponder == transponder && State() == DVB_TUNER_STATE_LOCKED)
    {
      dsyslog("Dvb tuner: tuned to %u MHz in %d ms", transponder.FrequencyMHz(), lockTimeoutMs - timeout.TimeLeft());
      return true;
    }
    else
    {
      dsyslog("Dvb tuner: tuning to %u MHz cancelled after %d ms", transponder.FrequencyMHz(), lockTimeoutMs - timeout.TimeLeft());
    }
  }
  else
  {
    dsyslog("Dvb tuner: tuning to %u MHz timed out after %d ms", transponder.FrequencyMHz(), lockTimeoutMs);
  }
  return false;
}

bool cDvbTuner::TuneDevice(void)
{
  if (!IsOpen())
    return false;

  if (m_nextTransponder == m_transponder)
    return true;

  m_transponder.Reset();
  m_status = FE_STATUS_UNKNOWN;

  dsyslog("##### Dvb tuner: tuning to channel %u (%u MHz)", m_nextTransponder.ChannelNumber(), m_nextTransponder.FrequencyMHz());

  cFrontendCommands frontendCmds(m_fileDescriptor);

  // Reset frontend's cache of data
  frontendCmds.AddCommand(DTV_CLEAR);
  if (!frontendCmds.Execute())
    return false;

  // Set common parameters
  frontendCmds.AddCommand(DTV_DELIVERY_SYSTEM, m_nextTransponder.DeliverySystem());
  frontendCmds.AddCommand(DTV_MODULATION,      m_nextTransponder.Modulation());
  frontendCmds.AddCommand(DTV_INVERSION,       m_nextTransponder.Inversion());

  // For satellital delivery systems, DTV_FREQUENCY is measured in kHz. For
  // the other ones, it is measured in Hz.
  // http://linuxtv.org/downloads/v4l-dvb-apis/FE_GET_SET_PROPERTY.html
  if (m_nextTransponder.IsSatellite())
  {
    unsigned int frequencyHz = GetTunedFrequencyHz(m_nextTransponder);
    if (frequencyHz == 0) // TODO
      return false;       // TODO
    frontendCmds.AddCommand(DTV_FREQUENCY, frequencyHz / 1000);
  }
  else
  {
    frontendCmds.AddCommand(DTV_FREQUENCY, m_nextTransponder.FrequencyHz());
  }

  // Set delivery-system-specific parameters
  if (m_nextTransponder.IsSatellite())
  {
    // DVB-S/DVB-S2 (common parts)
    frontendCmds.AddCommand(DTV_SYMBOL_RATE, m_nextTransponder.SatelliteParams().SymbolRateHz());
    frontendCmds.AddCommand(DTV_INNER_FEC,   m_nextTransponder.SatelliteParams().CoderateH());
    if (m_nextTransponder.DeliverySystem() != SYS_DVBS2)
    {
      // DVB-S
      frontendCmds.AddCommand(DTV_ROLLOFF,   ROLLOFF_35); // DVB-S always has a ROLLOFF of 0.35
    }
    else
    {
      // DVB-S2
      frontendCmds.AddCommand(DTV_PILOT,   PILOT_AUTO);
      frontendCmds.AddCommand(DTV_ROLLOFF, m_nextTransponder.SatelliteParams().RollOff());
      if (m_apiVersion >= "5.8")
        frontendCmds.AddCommand(DTV_STREAM_ID, m_nextTransponder.SatelliteParams().StreamId());
    }
  }
  else if (m_nextTransponder.IsCable())
  {
    // DVB-C
    frontendCmds.AddCommand(DTV_SYMBOL_RATE, m_nextTransponder.CableParams().SymbolRateHz());
    frontendCmds.AddCommand(DTV_INNER_FEC,   m_nextTransponder.CableParams().CoderateH());
  }
  else if (m_nextTransponder.IsTerrestrial())
  {
    // DVB-T/DVB-T2 (common parts)
    frontendCmds.AddCommand(DTV_BANDWIDTH_HZ,      m_nextTransponder.TerrestrialParams().BandwidthHz()); // Use hertz value, not enum
    frontendCmds.AddCommand(DTV_CODE_RATE_HP,      m_nextTransponder.TerrestrialParams().CoderateH());
    frontendCmds.AddCommand(DTV_CODE_RATE_LP,      m_nextTransponder.TerrestrialParams().CoderateL());
    frontendCmds.AddCommand(DTV_TRANSMISSION_MODE, m_nextTransponder.TerrestrialParams().Transmission());
    frontendCmds.AddCommand(DTV_GUARD_INTERVAL,    m_nextTransponder.TerrestrialParams().Guard());
    frontendCmds.AddCommand(DTV_HIERARCHY,         m_nextTransponder.TerrestrialParams().Hierarchy());

    if (m_nextTransponder.DeliverySystem() == SYS_DVBT2)
    {
      // DVB-T2
      if (m_apiVersion >= "5.8")
        frontendCmds.AddCommand(DTV_STREAM_ID, m_nextTransponder.TerrestrialParams().StreamId());
      else if (m_apiVersion >= "5.3")
        frontendCmds.AddCommand(DTV_DVBT2_PLP_ID_LEGACY, m_nextTransponder.TerrestrialParams().StreamId());
    }
  }
  else if (m_nextTransponder.IsAtsc())
  {
    // ATSC - no delivery-system-specific parameters
  }
  else
  {
    esyslog("ERROR: attempt to set channel with unknown DVB frontend type");
    return false;
  }

  // Perform the tune
  frontendCmds.AddCommand(DTV_TUNE);
  if (!frontendCmds.Execute())
    return false;

  // Some drivers report stale status immediately after tuning. Give stale lock
  // events a chance to clear, then reset m_tunedLock and wait for real lock events
  Sleep(TUNE_DELAY_MS);

  m_transponder = m_nextTransponder;
  m_nextTransponder.Reset();
  return true;
}

unsigned int cDvbTuner::GetTunedFrequencyHz(const cTransponder& transponder)
{
  unsigned int frequencyHz = transponder.FrequencyHz();

  if (transponder.IsSatellite())
  {
    const fe_polarization_t polarization = transponder.SatelliteParams().Polarization();
    if (cSettings::Get().m_bDiSEqC)
    {
      if (const cDiseqc *diseqc = Diseqcs.Get(m_device->Index(), transponder.Type(), frequencyHz, polarization, &m_scr))
      {
        frequencyHz -= diseqc->Lof();
        if (diseqc != m_lastDiseqc || diseqc->IsScr())
        {
          if (IsBondedMaster())
          {
            ExecuteDiseqc(diseqc, &frequencyHz);
            if (frequencyHz == 0)
              return 0;
          }
          else
            ResetToneAndVoltage();
          m_lastDiseqc = diseqc;
        }
      }
      else
      {
        esyslog("No DiSEqC parameters found for channel %d", transponder.ChannelNumber());
        return 0;
      }
    }
    else
    {
      int tone = SEC_TONE_OFF;
      if (frequencyHz < (unsigned int)cSettings::Get().m_iLnbSLOF)
      {
        frequencyHz -= cSettings::Get().m_iLnbFreqLow;
        tone = SEC_TONE_OFF;
      }
      else
      {
        frequencyHz -= cSettings::Get().m_iLnbFreqHigh;
        tone = SEC_TONE_ON;
      }

      int volt;
      switch (transponder.SatelliteParams().Polarization())
      {
      case POLARIZATION_HORIZONTAL:
      case POLARIZATION_VERTICAL:
        volt = SEC_VOLTAGE_13;
        break;
      case POLARIZATION_CIRCULAR_LEFT:
      case POLARIZATION_CIRCULAR_RIGHT:
      default:
        volt = SEC_VOLTAGE_18;
        break;
      }

      if (!IsBondedMaster())
      {
        tone = SEC_TONE_OFF;
        volt = SEC_VOLTAGE_13;
      }

      /* TODO
      if (ioctl(m_fileDescriptor, FE_SET_VOLTAGE, volt) < 0)
        LOG_ERROR;

      if (ioctl(m_fileDescriptor, FE_SET_TONE, tone) < 0)
        LOG_ERROR;
      */
    }

    frequencyHz = ::abs(frequencyHz); // Allow for C-band, where the frequency is less than the LOF
  }

  return frequencyHz;
}

std::string cDvbTuner::StatusToString(const fe_status_t status)
{
  std::string retval;

  if (status & FE_HAS_SIGNAL)
    retval.append(" [signal]");
  if (status & FE_HAS_CARRIER)
    retval.append(" [carrier]");
  if (status & FE_HAS_VITERBI)
    retval.append(" [stable]");
  if (status & FE_HAS_SYNC)
    retval.append(" [sync]");
  if (status & FE_HAS_LOCK)
    retval.append(" [lock]");
  if (status & FE_TIMEDOUT)
    retval.append(" [timeout]");
  if (status & FE_REINIT)
    retval.append(" [reinit]");

  if (!retval.empty())
    retval.erase(retval.begin());
  else
    retval = "[unknown/error]";

  return retval;
}

inline bool cDvbTuner::CheckEvent(uint32_t iTimeoutMs /* = 0 */)
{
  CLockObject lock(m_tuneEventMutex);
  if (m_tuneEventCondition.Wait(m_tuneEventMutex, m_tuneEvent, iTimeoutMs))
  {
    m_tuneEvent = false;
    return true;
  }
  return false;
}

void cDvbTuner::UpdateFrontendStatus(const fe_status_t& status)
{
  bool bLocked(false);
  bool bLostLock(false);

  if (m_status == status)
    return;

  // Update status, recording lock state before and after status change
  const bool bWasSynced(IsSynced());
  m_status = status;
  const bool bIsSynced(IsSynced());

  dsyslog("frontend %d '%s' status changed to %s", m_device->Index(), Name().c_str(), StatusToString(status).c_str());

  // Report new lock status to observers
  if (!bWasSynced && bIsSynced)
  {
    SetChanged();
    bLocked = true;
    CLockObject lock(m_stateMutex);
    SetState(DVB_TUNER_STATE_LOCKED);
    m_tunedLock = true;
    m_lockEvent.Broadcast();
  }
  else if (bWasSynced && !bIsSynced)
  {
    SetChanged();
    bLostLock = true;
    CLockObject lock(m_stateMutex);
    SetState(DVB_TUNER_STATE_LOST_LOCK);
  }

  // Check for frontend re-initialisation
  if (m_status & FE_REINIT)
  {
    // TODO
  }

  if (bLocked)
  {
    bLocked = false;
    NotifyObservers(ObservableMessageChannelLock);
  }

  if (bLostLock)
  {
    bLostLock = false;
    NotifyObservers(ObservableMessageChannelLostLock);
  }
}

DVB_TUNER_STATE cDvbTuner::State(void)
{
  CLockObject lock(m_stateMutex);
  return m_state;
}

void cDvbTuner::SetState(DVB_TUNER_STATE state)
{
  CLockObject lock(m_stateMutex);
  if (m_state != DVB_TUNER_STATE_CANCEL || state == DVB_TUNER_STATE_IDLE)
    m_state = state;
}

bool cDvbTuner::CancelTuning(uint32_t iTimeoutMs)
{
  /** cancel previous tuning */
  if (m_state != DVB_TUNER_STATE_IDLE)
  {
    dsyslog("cancel previous tuning (state %d)", m_state);
    m_state = DVB_TUNER_STATE_CANCEL;
    m_tuneEvent = true;
    m_tuneEventCondition.Signal();
    if (m_tunerIdleCondition.Wait(m_stateMutex, m_tunerIdle, iTimeoutMs))
    {
      dsyslog("cancelled previous tuning (state %d)", m_state);
      return true;
    }
    esyslog("failed to cancel previous tuning (state %d)", m_state);
    return false;
  }
  return true;
}

void* cDvbTuner::Process(void)
{
  fe_status_t status;
  cPoller poller(m_fileDescriptor, false, true);
  struct dvb_frontend_event event;

  while (!IsStopped())
  {
    switch (State())
    {
      case DVB_TUNER_STATE_CANCEL:
        {
          CLockObject lock(m_stateMutex);
          m_transponder.Reset();
          m_nextTransponder.Reset();
          m_tunedLock = true;
          m_lockEvent.Broadcast();

          SetChanged();
          NotifyObservers(ObservableMessageChannelLostLock);

          // TODO: When is this needed?
          //ResetToneAndVoltage();

          m_tunedLock = false;
          m_status = FE_STATUS_UNKNOWN;
          SetState(DVB_TUNER_STATE_IDLE);
          // detach remaining receivers that didn't get closed correctly
          m_device->Receiver()->DetachAllReceivers(false);
          m_tunerIdleCondition.Signal();
        }
        break;
      case DVB_TUNER_STATE_IDLE:
        {
          CLockObject lock(m_stateMutex);
          m_tunerIdle = true;
          m_tunerIdleCondition.Signal();
        }
        CheckEvent();
        break;
      case DVB_TUNER_STATE_TUNING:
        {
          CLockObject lock(m_stateMutex);
          m_tunerIdle = false;
          SetState(TuneDevice() ?
              DVB_TUNER_STATE_LOCKING :
              DVB_TUNER_STATE_TUNING_FAILED);
        }
        break;
      case DVB_TUNER_STATE_LOCKING:
      case DVB_TUNER_STATE_LOCKED:
      case DVB_TUNER_STATE_LOST_LOCK:
        {
          while (!IsStopped() && State() != DVB_TUNER_STATE_CANCEL)
          {
            // According to MythTv, waiting for DVB events fails with several DVB cards.
            // They either don't send the required events or delete them from the event
            // queue before we can read the event. Using a FE_GET_FRONTEND call has also
            // been tried, but returns the old data until a 2 sec timeout elapses on at
            // least one DVB card.

            // If polling fails (which happens on several DVB cards), then at least we
            // have a timeout.

            poller.Poll(TUNER_POLL_TIMEOUT_MS);
            if (IsStopped() || State() == DVB_TUNER_STATE_CANCEL)
              break;

            // Read all the events off the queue, so we can poll until backend gets
            // another tune message
            while (ioctl(m_fileDescriptor, FE_GET_EVENT, &event) == 0 && !IsStopped() && State() != DVB_TUNER_STATE_CANCEL)
              Sleep(TUNER_POLL_TIMEOUT_MS);

            if (ioctl(m_fileDescriptor, FE_READ_STATUS, &status) >= 0)
              UpdateFrontendStatus(status);
            else
              esyslog("Dvb tuner: failed to read status from tuner: %m");
          }
        }
        break;
    }

    CheckEvent(TUNER_POLL_TIMEOUT_MS);
  }

  return NULL;
}

cTransponder cDvbTuner::GetTransponder(void) const
{
  CLockObject lock(m_stateMutex);
  return m_transponder;
}

bool cDvbTuner::IsTunedTo(const cTransponder& transponder) const
{
  CLockObject lock(m_stateMutex);

  if (!HasLock())
    return false;

  return m_transponder == transponder;
}

void cDvbTuner::ClearTransponder(const cTransponder& transponder)
{
  CLockObject lock(m_stateMutex);
  if (m_transponder == transponder)
  {
    SetChanged();
    CancelTuning(2000);
    NotifyObservers(ObservableMessageChannelLostLock);
  }
}

void cDvbTuner::SetSignalStrength(signal_quality_info_t& info) const
{
  uint16_t MaxSignal = 0xFFFF; // Let's assume the default is using the entire range.

  // Use the subsystemId to identify individual devices in case they need
  // special treatment to map their Signal value into the range 0...0xFFFF.
  switch (m_subsystemId)
  {
  case 0x13C21019: // TT-budget S2-3200 (DVB-S/DVB-S2)
  case 0x1AE40001: // TechniSat SkyStar HD2 (DVB-S/DVB-S2)
    MaxSignal = 670;
    break;
  }

  int s = int(info.signal) * 100 / MaxSignal;
  if (s > 100)
    s = 100;
  if (s < 0)
    s = 0;

  info.strength = s;
}

#define LOCK_THRESHOLD 5 // indicates that all 5 FE_HAS_* flags are set

#define READ_SIGNAL(item, value, defval) \
    while (1) \
    { \
      if (ioctl(m_fileDescriptor, item, &(value)) >= 0) \
        break; \
      if (errno != EINTR) \
      { \
        (value) = defval; \
        break; \
      } \
    }

bool cDvbTuner::SignalQuality(signal_quality_info_t& info) const
{
  CLockObject lock(m_stateMutex);
  info.status = (signal_quality_status_t)m_status;
  info.name   = StringUtils::Format("%s (%d/%d)", Name().c_str(), m_device->Adapter(), m_device->Frontend());

  if (HasLock())
  {
    READ_SIGNAL(FE_READ_SNR, info.snr, 0xFFFF);
    READ_SIGNAL(FE_READ_BER, info.ber, 0);
    READ_SIGNAL(FE_READ_UNCORRECTED_BLOCKS, info.unc, 0);
    READ_SIGNAL(FE_READ_SIGNAL_STRENGTH, info.signal, 0);
    SetQualityPercentage(info);
    SetSignalStrength(info);
    info.status_string = StringUtils::Format("%s:%s:%s:%s:%s",
                                             (info.status & SIG_HAS_LOCK) ? "LOCKED" : "-",
                                             (info.status & SIG_HAS_SIGNAL) ? "SIGNAL" : "-",
                                             (info.status & SIG_HAS_CARRIER) ? "CARRIER" : "-",
                                             (info.status & SIG_HAS_VITERBI) ? "VITERBI" : "-",
                                             (info.status & SIG_HAS_SYNC) ? "SYNC" : "-");
  }
  else
  {
    info.snr      = -2;
    info.ber      = -2;
    info.unc      = -2;
    info.signal   = -2;
    info.quality  = 0;
    info.strength = 0;
  }

#if DEBUG_SIGNALQUALITY
  dsyslog("FE %s: %08X s/n:%04X ber:%5d unc:%5d qual:%u str:%u", info.name.c_str(), m_subsystemId, info.snr, int(info.ber), int(info.unc), info.quality, info.strength);
#endif

  return true;
}

void cDvbTuner::SetQualityPercentage(signal_quality_info_t& info) const
{
  uint16_t MinSnr = 0x0000;
  uint16_t MaxSnr = 0xFFFF; // Let's assume the default is using the entire range.

  // Use the subsystemId to identify individual devices in case they need
  // special treatment to map their Snr value into the range 0...0xFFFF.
  switch (m_subsystemId)
  {
  case 0x13C21019: // TT-budget S2-3200 (DVB-S/DVB-S2)
  case 0x1AE40001: // TechniSat SkyStar HD2 (DVB-S/DVB-S2)
    if (m_transponder.DeliverySystem() == SYS_DVBS2)
    {
      MinSnr = 10;
      MaxSnr = 70;
    }
    else
      MaxSnr = 200;
    break;
  case 0x20130245: // PCTV Systems PCTV 73ESE
  case 0x2013024F: // PCTV Systems nanoStick T2 290e
    MaxSnr = 255; break;
  }

  int a = int(constrain(info.snr, MinSnr, MaxSnr)) * 100 / (MaxSnr - MinSnr);
  int b = 100 - (info.unc * 10 + (info.ber / 256) * 5);
  if (b < 0)
    b = 0;

  int q = LOCK_THRESHOLD + a * b * (100 - LOCK_THRESHOLD) / 100 / 100;
  if (q > 100)
    q = 100;
  if (q < 0)
    q = 0;

  info.quality = q;
}

void cDvbTuner::ResetToneAndVoltage(void)
{
  /* TODO
  if (ioctl(m_fileDescriptor, FE_SET_VOLTAGE, m_bondedTuner ? SEC_VOLTAGE_OFF : SEC_VOLTAGE_13) < 0)
    LOG_ERROR;

  if (ioctl(m_fileDescriptor, FE_SET_TONE, SEC_TONE_OFF) < 0)
    LOG_ERROR;
  */
}

void cDvbTuner::ExecuteDiseqc(const cDiseqc* Diseqc, unsigned int* Frequency)
{
  if (!m_bLnbPowerTurnedOn)
  {
    // must explicitly turn on LNB power
    if (ioctl(m_fileDescriptor, FE_SET_VOLTAGE, SEC_VOLTAGE_13) < 0)
      LOG_ERROR;
    m_bLnbPowerTurnedOn = true;
  }

  if (Diseqc->IsScr())
    m_diseqcMutex.Lock();

  struct dvb_diseqc_master_cmd cmd;
  const char* CurrentAction = NULL;

  for (;;)
  {
    cmd.msg_len = sizeof(cmd.msg);
    cDiseqc::eDiseqcActions da = Diseqc->Execute(&CurrentAction, cmd.msg, &cmd.msg_len, m_scr, Frequency);
    if (da == cDiseqc::daNone)
      break;
    switch (da)
    {

    case cDiseqc::daToneOff:
      if (ioctl(m_fileDescriptor, FE_SET_TONE, SEC_TONE_OFF) < 0)
        LOG_ERROR;
      break;
    case cDiseqc::daToneOn:
      if (ioctl(m_fileDescriptor, FE_SET_TONE, SEC_TONE_ON) < 0)
        LOG_ERROR;
      break;
    case cDiseqc::daVoltage13:
      if (ioctl(m_fileDescriptor, FE_SET_VOLTAGE, SEC_VOLTAGE_13) < 0)
        LOG_ERROR;
      break;
    case cDiseqc::daVoltage18:
      if (ioctl(m_fileDescriptor, FE_SET_VOLTAGE, SEC_VOLTAGE_18) < 0)
        LOG_ERROR;
      break;
    case cDiseqc::daMiniA:
      if (ioctl(m_fileDescriptor, FE_DISEQC_SEND_BURST, SEC_MINI_A) < 0)
        LOG_ERROR;
      break;
    case cDiseqc::daMiniB:
      if (ioctl(m_fileDescriptor, FE_DISEQC_SEND_BURST, SEC_MINI_B) < 0)
        LOG_ERROR;
      break;
    case cDiseqc::daCodes:
      if (ioctl(m_fileDescriptor, FE_DISEQC_SEND_MASTER_CMD, &cmd) < 0)
        LOG_ERROR;
      break;
    default:
      esyslog("ERROR: unknown diseqc command %d", da);
      break;
    }
  }

  if (m_scr)
    ResetToneAndVoltage(); // makes sure we don't block the bus!

  if (Diseqc->IsScr())
    m_diseqcMutex.Unlock();
}

// TODO move this
#if defined(TARGET_ANDROID)
static const string DemuxSourceToString(AM_AMX_SOURCE src)
{
  string cmd;

  switch(src)
  {
  case AM_DMX_SRC_TS0:
    cmd = "ts0";
    break;
  case AM_DMX_SRC_TS1:
    cmd = "ts1";
    break;
  case AM_DMX_SRC_TS2:
    cmd = "ts2";
    break;
  case AM_DMX_SRC_HIU:
    cmd = "hiu";
    break;
  default:
    dsyslog("Demux source not supported: %d", src);
    break;
  }

  return cmd;
}

static bool WriteToFile(const string& filename, const string& data)
{
  bool retval(false);
  CFile demuxSource;

  if (data.empty() || filename.empty())
    return false;

  if (demuxSource.OpenForWrite(filename, false))
  {
    dsyslog("Writing '%s' to '%s'", data.c_str(), filename.c_str());
    retval = demuxSource.Write(data.c_str(), data.length()) == data.length();
    demuxSource.Close();
  }
  else
  {
    dsyslog("Can't open %s", filename.c_str());
  }

  return retval;
}
#endif

bool cDvbTuner::InitialiseHardware(void)
{
#if defined(TARGET_ANDROID)
  // TODO: Magic code that makes everything work on Geniatech's Android box
  const string strDemuxSourcePath(StringUtils::Format("/sys/class/stb/demux%d_source", m_device->Frontend()));
  const string strDemuxSource(DemuxSourceToString(AM_DMX_SRC_TS2));
  const string strAsyncFifoPath(StringUtils::Format("/sys/class/stb/asyncfifo%d_source", m_device->Frontend()));
  const string strAsyncFifo(StringUtils::Format("dmx%u", m_device->Frontend()));

  return WriteToFile(strDemuxSourcePath, strDemuxSource) &&
      WriteToFile(strAsyncFifoPath, strAsyncFifo);
#endif

  return true;
}

}
