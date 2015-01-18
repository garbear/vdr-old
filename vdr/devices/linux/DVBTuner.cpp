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

#define DVBS_LOCK_TIMEOUT_MS       2000
#define DVBC_LOCK_TIMEOUT_MS       2000
#define DVBT_LOCK_TIMEOUT_MS       2000
#define ATSC_LOCK_TIMEOUT_MS       2000

#define TUNER_POLL_TIMEOUT_MS      100 // tuning usually takes about a second

#define DEBUG_SIGNALSTRENGTH       1
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
   m_bIsOpen(false),
   m_capabilities(FE_IS_STUPID),
   m_subsystemId(0),
   m_apiVersion(Version::EmptyVersion),
   m_status(FE_STATUS_UNKNOWN),
   m_tunedLock(false),
   m_tunedSignal(false),
   m_lastDiseqc(NULL),
   m_scr(NULL),
   m_bLnbPowerTurnedOn(false),
   m_fileDescriptor(INVALID_FD)
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

bool cDvbTuner::Open(void)
{
  isyslog("Opening DVB tuner %s", m_device->DvbPath(DEV_DVB_FRONTEND).c_str());

  CLockObject lock(m_mutex);

  if (IsRunning())
    return true;

  errno = 0;
  while (INVALID_FD == (m_fileDescriptor = open(m_device->DvbPath(DEV_DVB_FRONTEND).c_str(), O_RDWR | O_NONBLOCK)))
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
      return false;
    }
  }

  if (!SetProperties())
    return false;

  isyslog("DVB tuner: Detected DVB API version %s", m_apiVersion.c_str());

  if (!InitialiseHardware())
    return false;

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

  m_bIsOpen = true;

  return true;
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
  CLockObject lock(m_mutex);

  if (m_bIsOpen)
  {
    isyslog("DVB tuner: Closing frontend");

    StopThread(-1);

    close(m_fileDescriptor);
    m_fileDescriptor = INVALID_FD;

    /* looks like this irritates the SCR switch, so let's leave it out for now
    if (m_lastDiseqc && m_lastDiseqc->IsScr())
    {
      unsigned int frequency = 0;
      ExecuteDiseqc(m_lastDiseqc, &frequency);
    }
    */
  }
}

unsigned int GetLockTimeout(TRANSPONDER_TYPE frontendType)
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
  ClearTransponder();

  if (!TuneDevice(transponder))
    return false;

  int64_t startMs = GetTimeMs();

  // Device is tuning. Create the event-handling thread. Thread stays running
  // even after tuner gets lock.
  CreateThread(false);

  // Some drivers report stale status immediately after tuning. Give stale lock
  // events a chance to clear, then reset m_tunedLock and wait for real lock events
  Sleep(TUNE_DELAY_MS);
  CLockObject lock(m_mutex);
  m_tunedLock = false;

  if (m_lockEvent.Wait(m_mutex, m_tunedLock, GetLockTimeout(transponder.Type()) - TUNE_DELAY_MS))
    dsyslog("Dvb tuner: tuned to channel %u in %d ms", transponder.ChannelNumber(), GetTimeMs() - startMs);
  else
    dsyslog("Dvb tuner: tuning timed out after %d ms", GetTimeMs() - startMs);

  return m_tunedLock;
}

bool cDvbTuner::TuneDevice(const cTransponder& transponder)
{
  CLockObject lock(m_mutex);

  if (!m_bIsOpen)
    return false;

  dsyslog("#####");
  dsyslog("##### Dvb tuner: tuning to channel %u (%u MHz)", transponder.ChannelNumber(), transponder.FrequencyMHz());
  dsyslog("#####");

  cFrontendCommands frontendCmds(m_fileDescriptor);

  // Reset frontend's cache of data
  frontendCmds.AddCommand(DTV_CLEAR);
  if (!frontendCmds.Execute())
    return false;

  // Set common parameters
  frontendCmds.AddCommand(DTV_DELIVERY_SYSTEM, transponder.DeliverySystem());
  frontendCmds.AddCommand(DTV_MODULATION,      transponder.Modulation());
  frontendCmds.AddCommand(DTV_INVERSION,       transponder.Inversion());

  // For satellital delivery systems, DTV_FREQUENCY is measured in kHz. For
  // the other ones, it is measured in Hz.
  // http://linuxtv.org/downloads/v4l-dvb-apis/FE_GET_SET_PROPERTY.html
  if (transponder.IsSatellite())
  {
    unsigned int frequencyHz = GetTunedFrequencyHz(transponder);
    if (frequencyHz == 0) // TODO
      return false;       // TODO
    frontendCmds.AddCommand(DTV_FREQUENCY, frequencyHz / 1000);
  }
  else
  {
    frontendCmds.AddCommand(DTV_FREQUENCY, transponder.FrequencyHz());
  }

  // Set delivery-system-specific parameters
  if (transponder.IsSatellite())
  {
    // DVB-S/DVB-S2 (common parts)
    frontendCmds.AddCommand(DTV_SYMBOL_RATE, transponder.SatelliteParams().SymbolRateHz());
    frontendCmds.AddCommand(DTV_INNER_FEC,   transponder.SatelliteParams().CoderateH());
    if (transponder.DeliverySystem() != SYS_DVBS2)
    {
      // DVB-S
      frontendCmds.AddCommand(DTV_ROLLOFF,   ROLLOFF_35); // DVB-S always has a ROLLOFF of 0.35
    }
    else
    {
      // DVB-S2
      frontendCmds.AddCommand(DTV_PILOT,   PILOT_AUTO);
      frontendCmds.AddCommand(DTV_ROLLOFF, transponder.SatelliteParams().RollOff());
      if (m_apiVersion >= "5.8")
        frontendCmds.AddCommand(DTV_STREAM_ID, transponder.SatelliteParams().StreamId());
    }
  }
  else if (transponder.IsCable())
  {
    // DVB-C
    frontendCmds.AddCommand(DTV_SYMBOL_RATE, transponder.CableParams().SymbolRateHz());
    frontendCmds.AddCommand(DTV_INNER_FEC,   transponder.CableParams().CoderateH());
  }
  else if (transponder.IsTerrestrial())
  {
    // DVB-T/DVB-T2 (common parts)
    frontendCmds.AddCommand(DTV_BANDWIDTH_HZ,      transponder.TerrestrialParams().BandwidthHz()); // Use hertz value, not enum
    frontendCmds.AddCommand(DTV_CODE_RATE_HP,      transponder.TerrestrialParams().CoderateH());
    frontendCmds.AddCommand(DTV_CODE_RATE_LP,      transponder.TerrestrialParams().CoderateL());
    frontendCmds.AddCommand(DTV_TRANSMISSION_MODE, transponder.TerrestrialParams().Transmission());
    frontendCmds.AddCommand(DTV_GUARD_INTERVAL,    transponder.TerrestrialParams().Guard());
    frontendCmds.AddCommand(DTV_HIERARCHY,         transponder.TerrestrialParams().Hierarchy());

    if (transponder.DeliverySystem() == SYS_DVBT2)
    {
      // DVB-T2
      if (m_apiVersion >= "5.8")
        frontendCmds.AddCommand(DTV_STREAM_ID, transponder.TerrestrialParams().StreamId());
      else if (m_apiVersion >= "5.3")
        frontendCmds.AddCommand(DTV_DVBT2_PLP_ID_LEGACY, transponder.TerrestrialParams().StreamId());
    }
  }
  else if (transponder.IsAtsc())
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

  m_transponder = transponder;
  m_tunedSignal = true;
  m_tuneCondition.Signal();

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

void* cDvbTuner::Process(void)
{
  fe_status_t status;
  bool bSuccess, bLocked = false, bLostLock = false;

  while (!IsStopped())
  {
    // Wait for backend to get a tune message
    {
      CLockObject lock(m_mutex);
      if (!m_tuneCondition.Wait(m_mutex, m_tunedSignal, 500))
        continue;
    }

    // According to MythTv, waiting for DVB events fails with several DVB cards.
    // They either don't send the required events or delete them from the event
    // queue before we can read the event. Using a FE_GET_FRONTEND call has also
    // been tried, but returns the old data until a 2 sec timeout elapses on at
    // least one DVB card.

    // If polling fails (which happens on several DVB cards), then at least we
    // have a timeout.
    cPoller poller(m_fileDescriptor, false, true);
    bool bPollSuccess = poller.Poll(TUNER_POLL_TIMEOUT_MS);

    if (IsStopped())
      break;

    // Read all the events off the queue, so we can poll until backend gets
    // another tune message
    struct dvb_frontend_event event;
    while (ioctl(m_fileDescriptor, FE_GET_EVENT, &event) == 0) { Sleep(TUNER_POLL_TIMEOUT_MS); } // empty
    {
      CLockObject lock(m_mutex);
      bSuccess = (ioctl(m_fileDescriptor, FE_READ_STATUS, &status) >= 0);

      if (!bSuccess)
      {
        esyslog("Dvb tuner: failed to read status from tuner: %m");
      }
      else if (bSuccess && m_status != status)
      {
        // Update status, recording lock state before and after status change
        const bool bWasSynced = IsSynced();
        m_status = status;
        const bool bIsSynced = IsSynced();

        dsyslog("frontend %d '%s' status changed to %s", m_device->Index(), Name().c_str(), StatusToString(status).c_str());

        // Report new lock status to observers
        if (!bWasSynced && bIsSynced)
        {
          SetChanged();
          bLocked = true;
          CLockObject lock(m_mutex);
          m_tunedLock = true;
          m_lockEvent.Broadcast();
        }
        else if (bWasSynced && !bIsSynced)
        {
          SetChanged();
          bLostLock = true;
        }

        // Check for frontend re-initialisation
        if (m_status & FE_REINIT)
        {
          // TODO
        }
      }
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

    Sleep(TUNER_POLL_TIMEOUT_MS);
  }

  m_lastDiseqc = NULL;

  return NULL;
}

cTransponder cDvbTuner::GetTransponder(void) const
{
  CLockObject lock(m_mutex);
  return m_transponder;
}

bool cDvbTuner::IsTunedTo(const cTransponder& transponder) const
{
  CLockObject lock(m_mutex);

  if (!HasLock())
    return false;

  return m_transponder == transponder;
}

void cDvbTuner::ClearTransponder(void)
{
  CLockObject lock(m_mutex);
  if (m_tunedSignal)
    m_tunedSignal = false;
  else
    return;

  lock.Unlock();

  SetChanged();
  NotifyObservers(ObservableMessageChannelLostLock);

  lock.Lock();

  m_transponder.Reset();
  m_status = FE_STATUS_UNKNOWN;

  // TODO: When is this needed?
  //ResetToneAndVoltage();

  /*
  if (m_bondedTuner && m_device->IsPrimaryDevice())
    cDeviceManager::Get().PrimaryDevice()->PID()->DelLivePids(); // 'device' is const, so we must do it this way
  */

  m_tunedLock = false;
  m_lockEvent.Broadcast();
}

int cDvbTuner::GetSignalStrength(void) const
{
  CLockObject lock(m_mutex);

  uint16_t Signal;

  while (ioctl(m_fileDescriptor, FE_READ_SIGNAL_STRENGTH, &Signal) < 0)
  {
    if (errno != EINTR)
      return -1;
  }

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

  int s = int(Signal) * 100 / MaxSignal;
  if (s > 100)
    s = 100;

#if DEBUG_SIGNALSTRENGTH
  fprintf(stderr, "FE %d/%d: %08X S = %04X %04X %3d%%\n", m_device->Adapter(), m_device->Frontend(), m_subsystemId, MaxSignal, Signal, s);
#endif

  return s;
}

#define LOCK_THRESHOLD 5 // indicates that all 5 FE_HAS_* flags are set

int cDvbTuner::GetSignalQuality(void) const
{
  CLockObject lock(m_mutex);

  if (m_status != FE_STATUS_UNKNOWN)
  {
    // Actually one would expect these checks to be done from FE_HAS_SIGNAL to FE_HAS_LOCK, but some drivers (like the stb0899) are broken, so FE_HAS_LOCK is the only one that (hopefully) is generally reliable...
    if (!HasLock())
    {
      if ((m_status & FE_HAS_SIGNAL) == 0)
        return 0;
      if ((m_status & FE_HAS_CARRIER) == 0)
        return 1;
      if ((m_status & FE_HAS_VITERBI) == 0)
        return 2;
      if ((m_status & FE_HAS_SYNC) == 0)
        return 3;
      return 4;
    }

#if DEBUG_SIGNALQUALITY
    bool HasSnr = true;
#endif

    uint16_t Snr;
    while (1)
    {
      if (ioctl(m_fileDescriptor, FE_READ_SNR, &Snr) >= 0)
        break;
      if (errno != EINTR)
      {
        Snr = 0xFFFF;
#if DEBUG_SIGNALQUALITY
        HasSnr = false;
#endif
        break;
      }
    }

#if DEBUG_SIGNALQUALITY
    bool HasBer = true;
#endif

    uint32_t Ber;
    while (1)
    {
      if (ioctl(m_fileDescriptor, FE_READ_BER, &Ber) >= 0)
        break;
      if (errno != EINTR)
      {
        Ber = 0;
#if DEBUG_SIGNALQUALITY
        HasBer = false;
#endif
        break;
      }
    }

#if DEBUG_SIGNALQUALITY
    bool HasUnc = true;
#endif

    uint32_t Unc;
    while (1)
    {
      if (ioctl(m_fileDescriptor, FE_READ_UNCORRECTED_BLOCKS, &Unc) >= 0)
        break;
      if (errno != EINTR)
      {
        Unc = 0;
#if DEBUG_SIGNALQUALITY
        HasUnc = false;
#endif
        break;
      }
    }

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

    int a = int(constrain(Snr, MinSnr, MaxSnr)) * 100 / (MaxSnr - MinSnr);
    int b = 100 - (Unc * 10 + (Ber / 256) * 5);
    if (b < 0)
      b = 0;

    int q = LOCK_THRESHOLD + a * b * (100 - LOCK_THRESHOLD) / 100 / 100;
    if (q > 100)
      q = 100;

#if DEBUG_SIGNALQUALITY
    fprintf(stderr, "FE %d/%d: %08X Q = %04X %04X %d %5d %5d %3d%%\n", m_device->Adapter(), m_device->Frontend(), m_subsystemId, MaxSnr, Snr, HasSnr, HasBer ? int(Ber) : -1, HasUnc ? int(Unc) : -1, q);
#endif

    return q;
  }
  return -1;
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
