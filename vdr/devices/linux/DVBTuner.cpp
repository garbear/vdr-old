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
#include "filesystem/Poller.h"
#include "platform/util/timeutils.h"
#include "settings/Settings.h"
#include "sources/linux/DVBTransponderParams.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

using namespace std;
using namespace PLATFORM;

#define DVB_API_VERSION_UNKNOWN    0x00000000
#define FE_STATUS_UNKNOWN          ((fe_status_t)0x00)

#define STATUS_POLL_PERIOD_MS      100 // Poll frontend status every 100ms

#define INVALID_FD                 -1

#define DVBS_LOCK_TIMEOUT_MS       9000
#define DVBC_LOCK_TIMEOUT_MS       9000
#define DVBT_LOCK_TIMEOUT_MS       9000
#define ATSC_LOCK_TIMEOUT_MS       9000

#define LOCK_REACQUIRE_TIMEOUT_MS  2000 // TODO: use this value somewhere

#define SCR_RANDOM_TIMEOUT_MS      500 // add random value up to this when tuning SCR device to avoid lockups
#define TUNER_POLL_TIMEOUT_MS      10

#define DEBUG_SIGNALSTRENGTH       1
#define DEBUG_SIGNALQUALITY        1

namespace VDR
{

// Delivery systems translation table
static const char* DeliverySystemNames[] =
{
  "",
  "DVB-C",
  "DVB-C",
  "DVB-T",
  "DSS",
  "DVB-S",
  "DVB-S2",
  "DVB-H",
  "ISDBT",
  "ISDBS",
  "ISDBC",
  "ATSC",
  "ATSCMH",
  "DMBTH",
  "CMMB",
  "DAB",
  "DVB-T2",
  "TURBO",
  "DVB-C",
};

void SetCommand(vector<dtv_property>& frontends, uint32_t command, uint32_t data = 0)
{
  dtv_property frontend = { };
  frontend.cmd = command;
  frontend.u.data = data;
  frontends.push_back(frontend);
}

CMutex cDvbTuner::m_bondMutex;

cDvbTuner::cDvbTuner(cDvbDevice *device)
 : m_device(device),
   m_frontendInfo(),
   m_frontendType(SYS_UNDEFINED),
   m_bIsOpen(false),
   m_fileDescriptor(INVALID_FD),
   m_dvbApiVersion(DVB_API_VERSION_UNKNOWN),
   m_numModulations(0),
   m_frontendStatus(FE_STATUS_UNKNOWN),
   m_lastDiseqc(NULL),
   m_scr(NULL),
   m_bLnbPowerTurnedOn(false),
   m_bondedTuner(NULL),
   m_bBondedMaster(false)
{
  assert(m_device);
}

string cDvbTuner::DeviceType(void) const
{
  if (m_bIsOpen)
  {
    if (m_frontendType != SYS_UNDEFINED)
      return DeliverySystemNames[m_frontendType];

    // m_bIsOpen == true guarantees that m_deliverySystems is not empty
    assert(!m_deliverySystems.empty());
    return DeliverySystemNames[m_deliverySystems[0]]; // To have some reasonable default
  }
  return "";
}

bool cDvbTuner::HasDeliverySystem(fe_delivery_system_t deliverySystem) const
{
  return std::find(m_deliverySystems.begin(), m_deliverySystems.end(), deliverySystem) != m_deliverySystems.end();
}

void cDvbTuner::SetChannel(const ChannelPtr& channel)
{
  m_channel        = channel;
  m_frontendType   = GetRequiredDeliverySystem(*channel);
  m_frontendStatus = FE_STATUS_UNKNOWN;
}

bool cDvbTuner::Open(void)
{
  CLockObject lock(m_mutex);

  if (m_bIsOpen)
    return true;

  try
  {
    errno = 0;
    while (INVALID_FD == (m_fileDescriptor = m_device->DvbOpen(DEV_DVB_FRONTEND, O_RDWR | O_NONBLOCK)))
    {
      if (errno == EINTR)
      {
        esyslog("Failed to open frontend: Interrupted system call. Trying again");
        continue;
      }
      else if (errno == EBUSY)
      {
        // TODO: If error is "Device or resource busy", kill the owner process
        throw "DVB tuner: Frontend is held by another process (try `lsof +D /dev/dvb`)";
      }
      else
      {
        throw "DVB tuner: Can't open frontend";
      }
    }

    m_dvbApiVersion = GetDvbApiVersion();
    if (m_dvbApiVersion == DVB_API_VERSION_UNKNOWN)
      throw "DVB tuner: Can't get DVB API version";

    dvb_frontend_info frontendInfo = { };
    if (ioctl(m_fileDescriptor, FE_GET_INFO, &frontendInfo) < 0 || frontendInfo.name == NULL)
      throw "DVB tuner: Can't get frontend info";

    m_deliverySystems = GetDeliverySystems();
    if (m_deliverySystems.empty())
      throw "DVB tuner: No delivery systems";

    m_frontendInfo.strName      = frontendInfo.name + StringUtils::Format(" (%u/%u)", m_device->Adapter(), m_device->Frontend());
    m_frontendInfo.capabilities = frontendInfo.caps;
    m_frontendInfo.type         = frontendInfo.type;
  }
  catch (const char* errorMsg)
  {
    esyslog("%s", errorMsg);
    return false;
  }

  // TODO: Why is only the number of modulations stored?
  vector<string> vecModulations = cDvbTransponderParams::GetModulationsFromCaps(m_frontendInfo.capabilities);
  m_numModulations = vecModulations.size();

  string modulations = StringUtils::Join(vecModulations, ",");
  if (modulations.empty())
    modulations = "unknown modulations";

  isyslog("Opened DVB tuner '%s' on frontend %u/%u",
      m_device->DeviceName().c_str(), m_device->Adapter(), m_device->Frontend());
  isyslog("Tuner provides %s with %s",
      TranslateDeliverySystems(m_deliverySystems).c_str(), modulations.c_str());

  m_bIsOpen = true;

  return true;
}

uint32_t cDvbTuner::GetDvbApiVersion(void) const
{
  uint32_t dvbApiVersion = DVB_API_VERSION_UNKNOWN;

  dtv_property frontend[1] = { };
  frontend[0].cmd = DTV_API_VERSION;

  dtv_properties cmdSeq = { ARRAY_SIZE(frontend), frontend };

  if (ioctl(m_fileDescriptor, FE_GET_PROPERTY, &cmdSeq) >= 0 && frontend[0].u.data != 0)
  {
    dvbApiVersion = frontend[0].u.data;
    isyslog("Detected DVB API version %08x", dvbApiVersion);
  }
  else
  {
    esyslog("Can't get DVB API version (errno=%d)", errno);
  }

  return dvbApiVersion;
}

vector<fe_delivery_system_t> cDvbTuner::GetDeliverySystems(void) const
{
  vector<fe_delivery_system_t> deliverySystems;

  // Determine the types of delivery systems this device provides
  bool bLegacyMode = true;
  if (m_dvbApiVersion >= 0x0505)
  {
    dtv_property frontend[1] = { };
    frontend[0].cmd = DTV_ENUM_DELSYS;

    dtv_properties cmdSeq = { ARRAY_SIZE(frontend), frontend };

    if (ioctl(m_fileDescriptor, FE_GET_PROPERTY, &cmdSeq) >= 0 && frontend[0].u.data != 0)
    {
      for (uint8_t* deliverySystem = frontend[0].u.buffer.data; deliverySystem != frontend[0].u.buffer.data + frontend[0].u.buffer.len; deliverySystem++)
        deliverySystems.push_back(static_cast<fe_delivery_system_t>(*deliverySystem));
      bLegacyMode = false;
    }
    else
    {
      esyslog("ERROR: can't query delivery systems on frontend %d/%d - falling back to legacy mode", m_device->Adapter(), m_device->Frontend());
    }
  }

  if (bLegacyMode)
  {
    // Legacy mode (DVB-API < 5.5):
    switch (m_frontendInfo.type)
    {
    case FE_QPSK:
      deliverySystems.push_back(SYS_DVBS);
      if (HasCapability(FE_CAN_2G_MODULATION))
        deliverySystems.push_back(SYS_DVBS2);
      break;
    case FE_OFDM:
      deliverySystems.push_back(SYS_DVBT);
      if (HasCapability(FE_CAN_2G_MODULATION))
        deliverySystems.push_back(SYS_DVBT2);
      break;
    case FE_QAM:
      deliverySystems.push_back(SYS_DVBC_ANNEX_AC);
      break;
    case FE_ATSC:
      deliverySystems.push_back(SYS_ATSC);
      break;
    default:
      esyslog("ERROR: unknown frontend type %d on frontend %d/%d", m_frontendInfo.type, m_device->Adapter(), m_device->Frontend());
      break;
    }
  }

  return deliverySystems;
}

void cDvbTuner::Close(void)
{
  CLockObject lock(m_mutex);

  if (m_bIsOpen)
  {
    ClearChannel();

    m_bIsOpen = false;

    m_deliverySystems.clear();

    isyslog("DVB tuner: Closing frontend %u/%u", m_device->Adapter(), m_device->Frontend());

    close(m_fileDescriptor);
    m_fileDescriptor = INVALID_FD;

    UnBond();

    /* looks like this irritates the SCR switch, so let's leave it out for now
    if (m_lastDiseqc && m_lastDiseqc->IsScr())
    {
      unsigned int frequency = 0;
      ExecuteDiseqc(m_lastDiseqc, &frequency);
    }
    */
  }
}

void* cDvbTuner::Process(void)
{
  while (!IsStopped())
  {
    fe_status_t newStatus = FE_STATUS_UNKNOWN;

    if (!GetFrontendStatus(newStatus))
      break;

    if (newStatus != m_frontendStatus)
    {
      vector<string> statusFlags;
      if (newStatus == FE_STATUS_UNKNOWN) statusFlags.push_back("STATUS_UNKNOWN");
      if (newStatus & FE_HAS_SIGNAL)      statusFlags.push_back("HAS_SIGNAL");
      if (newStatus & FE_HAS_CARRIER)     statusFlags.push_back("HAS_CARRIER");
      if (newStatus & FE_HAS_VITERBI)     statusFlags.push_back("HAS_VITERBI");
      if (newStatus & FE_HAS_SYNC)        statusFlags.push_back("HAS_SYNC");
      if (newStatus & FE_HAS_LOCK)        statusFlags.push_back("HAS_LOCK");
      if (newStatus & FE_TIMEDOUT)        statusFlags.push_back("TIMEDOUT");
      if (newStatus & FE_REINIT)          statusFlags.push_back("REINIT");
      dsyslog("Frontend %u/%u status updated to %s", m_device->Frontend(), m_device->Adapter(),
          StringUtils::Join(statusFlags, " | ").c_str());

      const bool bHadLock = HasLock();

      m_frontendStatus = newStatus;
      m_statusChangeEvent.Broadcast();

      if (bHadLock && !HasLock())
      {
        // TODO: Handle a lost lock, possibly by invoking a callback
        // We probably want to cancel any blocking DVB table filters
      }

      // Check for frontend re-initialisation
      if (m_frontendStatus & FE_REINIT)
      {
        CLockObject lock(m_mutex);
        if (!m_channel || !SwitchChannel(m_channel))
          break;
      }
    }
    m_sleepEvent.Wait(STATUS_POLL_PERIOD_MS);
  }

  m_lastDiseqc = NULL;

  return NULL;
}

void cDvbTuner::ClearEventQueue(void) const
{
  cPoller poller(m_fileDescriptor);
  if (poller.Poll(TUNER_POLL_TIMEOUT_MS))
  {
    dvb_frontend_event event;
    while (ioctl(m_fileDescriptor, FE_GET_EVENT, &event) >= 0)
      ; // just to clear the event queue - we'll read the actual status below
  }
}

bool cDvbTuner::GetFrontendStatus(fe_status_t &status) const
{
  CLockObject lock(m_mutex);

  // Only need to check the status if we're tuning or locked to a channel
  if (!m_channel)
    return false;

  ClearEventQueue();

  errno = 0;
  do
  {
    if (ioctl(m_fileDescriptor, FE_READ_STATUS, &status) >= 0)
      return true;
  } while (errno == EINTR);

  return false;
}

bool cDvbTuner::SwitchChannel(const ChannelPtr& channel)
{
  assert(channel.get());

  if (m_bondedTuner)
  {
    CLockObject lock(m_bondMutex);
    cDvbTuner *BondedMaster = GetBondedMaster();
    if (BondedMaster == this)
    {
      if (GetBondingParams(*channel) != GetBondingParams())
      {
        // switching to a completely different band, so set all others to idle:
        for (cDvbTuner *t = m_bondedTuner; t && t != this; t = t->m_bondedTuner)
          t->ClearChannel();
      }
    }
    else if (GetBondingParams(*channel) != BondedMaster->GetBondingParams())
      BondedMaster->SetChannel(channel);
  }

  {
    CLockObject lock(m_mutex);

    if (!m_bIsOpen)
      return false;

    if (!SetFrontend(channel))
      return false;

    SetChannel(channel);

    // Create the status-monitoring thread if not already running. Thread stays
    // running even after tuner gets lock until ClearChannel() is called.
    if (!IsRunning())
      CreateThread();
  }

  // Wait for HasLock() to return true
  CTimeout timeout(GetLockTimeout(m_frontendType));
  while (timeout.TimeLeft() > 0)
  {
    if (HasLock())
    {
      //if (m_bondedTuner && m_device->IsPrimaryDevice())
      //  cDeviceManager::Get().PrimaryDevice()->PID()->DelLivePids(); // 'device' is const, so we must do it this way
      return true;
    }

    const uint32_t timeLeftMs = timeout.TimeLeft();
    if (timeLeftMs > 0)
      m_statusChangeEvent.Wait(timeLeftMs);
  }

  return false;
}

ChannelPtr cDvbTuner::GetTransponder(void)
{
  CLockObject lock(m_mutex);
  return m_channel;
}

bool cDvbTuner::IsTunedTo(const cChannel& channel) const
{
  CLockObject lock(m_mutex);

  if (!HasLock())
    return false;

  if (m_channel->Source()                  == channel.Source() &&
      m_channel->TransponderFrequencyMHz() == channel.TransponderFrequencyMHz())
  {
    // Polarization is already checked as part of the Transponder
    // TODO: POLARIZATION SHOULD NOT BE PART OF THE TRANSPONDER! Remove this
    // when cChannel is fixed.
    return m_channel->Parameters() == channel.Parameters();
  }

  return false; // sufficient mismatch
}

void cDvbTuner::ClearChannel(void)
{
  CLockObject lock(m_mutex);

  if (!m_channel)
    return;

  m_channel.reset();

  // TODO: When is this needed?
  //ResetToneAndVoltage();

  /*
  if (m_bondedTuner && m_device->IsPrimaryDevice())
    cDeviceManager::Get().PrimaryDevice()->PID()->DelLivePids(); // 'device' is const, so we must do it this way
  */

  // Don't need the thread to udpate the status anymore
  StopThread(-1);
  m_sleepEvent.Broadcast();
}

bool cDvbTuner::SetFrontend(const ChannelPtr& channel)
{
  // Structure to hold our frontend commands
  vector<dtv_property> frontends;

  // Reset the cache of data specific to the frontend. Does not effect hardware
  SetCommand(frontends, DTV_CLEAR);
  dtv_properties cmdSeq = { frontends.size(), frontends.data() };
  if (ioctl(m_fileDescriptor, FE_SET_PROPERTY, &cmdSeq) < 0)
  {
    esyslog("Frontend %d/%d: %s", m_device->Adapter(), m_device->Frontend(), strerror(errno));
    return false;
  }

  frontends.clear();

  // Determine the required frontend type
  fe_delivery_system_t frontendType = GetRequiredDeliverySystem(*channel);
  if (frontendType == SYS_UNDEFINED)
    return false;
  SetCommand(frontends, DTV_DELIVERY_SYSTEM, frontendType);

  dsyslog("Tuner '%s' tuning to frequency %u", GetName().c_str(), channel->FrequencyHz());

  cDvbTransponderParams dtp(channel->Parameters());

  if (frontendType == SYS_DVBS || frontendType == SYS_DVBS2)
  {
    unsigned int frequencyHz = channel->FrequencyHz();
    if (cSettings::Get().m_bDiSEqC)
    {
      if (const cDiseqc *diseqc = Diseqcs.Get(m_device->CardIndex() + 1, channel->Source(), frequencyHz, dtp.Polarization(), &m_scr))
      {
        frequencyHz -= diseqc->Lof();
        if (diseqc != m_lastDiseqc || diseqc->IsScr())
        {
          if (IsBondedMaster())
          {
            ExecuteDiseqc(diseqc, &frequencyHz);
            if (frequencyHz == 0)
              return false;
          }
          else
            ResetToneAndVoltage();
          m_lastDiseqc = diseqc;
        }
      }
      else
      {
        esyslog("ERROR: no DiSEqC parameters found for channel %d", channel->Number());
        return false;
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
      switch (dtp.Polarization())
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

      if (ioctl(m_fileDescriptor, FE_SET_VOLTAGE, volt) < 0)
        LOG_ERROR;

      if (ioctl(m_fileDescriptor, FE_SET_TONE, tone) < 0)
        LOG_ERROR;
    }

    frequencyHz = ::abs(frequencyHz); // Allow for C-band, where the frequency is less than the LOF

    // DVB-S/DVB-S2 (common parts)

    // For satellital delivery systems, DTV_FREQUENCY is measured in kHz. For
    // the other ones, it is measured in Hz.
    // http://linuxtv.org/downloads/v4l-dvb-apis/FE_GET_SET_PROPERTY.html
    SetCommand(frontends, DTV_FREQUENCY,   frequencyHz * 1000UL);

    SetCommand(frontends, DTV_MODULATION,  dtp.Modulation());
    SetCommand(frontends, DTV_SYMBOL_RATE, channel->Srate() * 1000UL);
    SetCommand(frontends, DTV_INNER_FEC,   dtp.CoderateH());
    SetCommand(frontends, DTV_INVERSION,   dtp.Inversion());
    if (frontendType == SYS_DVBS2)
    {
      // DVB-S2
      SetCommand(frontends, DTV_PILOT,   PILOT_AUTO);
      SetCommand(frontends, DTV_ROLLOFF, dtp.RollOff());
      if (m_dvbApiVersion >= 0x0508)
        SetCommand(frontends, DTV_STREAM_ID, dtp.StreamId());
    }
    else
    {
      // DVB-S
      SetCommand(frontends, DTV_ROLLOFF,   ROLLOFF_35); // DVB-S always has a ROLLOFF of 0.35
    }
  }
  else if (frontendType == SYS_DVBC_ANNEX_AC || frontendType == SYS_DVBC_ANNEX_B)
  {
    // DVB-C
    SetCommand(frontends, DTV_FREQUENCY,   channel->FrequencyHz());
    SetCommand(frontends, DTV_INVERSION,   dtp.Inversion());
    SetCommand(frontends, DTV_SYMBOL_RATE, channel->Srate() * 1000UL);
    SetCommand(frontends, DTV_INNER_FEC,   dtp.CoderateH());
    SetCommand(frontends, DTV_MODULATION,  dtp.Modulation());
  }
  else if (frontendType == SYS_DVBT || frontendType == SYS_DVBT2)
  {
    // DVB-T/DVB-T2 (common parts)
    SetCommand(frontends, DTV_FREQUENCY,         channel->FrequencyHz());
    SetCommand(frontends, DTV_INVERSION,         dtp.Inversion());
    SetCommand(frontends, DTV_BANDWIDTH_HZ,      dtp.BandwidthHz()); // Use hertz value, not enum
    SetCommand(frontends, DTV_CODE_RATE_HP,      dtp.CoderateH());
    SetCommand(frontends, DTV_CODE_RATE_LP,      dtp.CoderateL());
    SetCommand(frontends, DTV_MODULATION,        dtp.Modulation());
    SetCommand(frontends, DTV_TRANSMISSION_MODE, dtp.Transmission());
    SetCommand(frontends, DTV_GUARD_INTERVAL,    dtp.Guard());
    SetCommand(frontends, DTV_HIERARCHY,         dtp.Hierarchy());

    if (frontendType == SYS_DVBT2)
    {
      // DVB-T2
      if (m_dvbApiVersion >= 0x0508)
        SetCommand(frontends, DTV_STREAM_ID, dtp.StreamId());
      else if (m_dvbApiVersion >= 0x0503)
        SetCommand(frontends, DTV_DVBT2_PLP_ID_LEGACY, dtp.StreamId());
    }
  }
  else if (frontendType == SYS_ATSC)
  {
    // ATSC
    SetCommand(frontends, DTV_FREQUENCY,  channel->FrequencyHz());
    SetCommand(frontends, DTV_INVERSION,  dtp.Inversion());
    SetCommand(frontends, DTV_MODULATION, dtp.Modulation());
  }
  else
  {
    esyslog("ERROR: attempt to set channel with unknown DVB frontend type");
    return false;
  }

  SetCommand(frontends, DTV_TUNE);

  dtv_properties cmdSeq2 = { frontends.size(), frontends.data() };

  if (ioctl(m_fileDescriptor, FE_SET_PROPERTY, &cmdSeq2) < 0)
  {
    esyslog("ERROR: frontend %d/%d: %m", m_device->Adapter(), m_device->Frontend());
    return false;
  }
  return true;
}

void cDvbTuner::ResetToneAndVoltage(void)
{
  if (ioctl(m_fileDescriptor, FE_SET_VOLTAGE, m_bondedTuner ? SEC_VOLTAGE_OFF : SEC_VOLTAGE_13) < 0)
    LOG_ERROR;

  if (ioctl(m_fileDescriptor, FE_SET_TONE, SEC_TONE_OFF) < 0)
    LOG_ERROR;
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

  static PLATFORM::CMutex Mutex;

  if (Diseqc->IsScr())
    Mutex.Lock();
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
    Mutex.Unlock();
}

bool cDvbTuner::Bond(cDvbTuner* tuner)
{
  CLockObject lock(m_bondMutex);

  if (!m_bondedTuner)
  {
    ResetToneAndVoltage();
    m_bBondedMaster = false; // makes sure we don't disturb an existing master
    m_bondedTuner = tuner->m_bondedTuner ? tuner->m_bondedTuner : tuner;
    tuner->m_bondedTuner = this;
    dsyslog("tuner %d/%d bonded with tuner %d/%d", m_device->Adapter(), m_device->Frontend(), m_bondedTuner->m_device->Adapter(), m_bondedTuner->m_device->Frontend());
    return true;
  }
  else
    esyslog("ERROR: tuner %d/%d already bonded with tuner %d/%d, can't bond with tuner %d/%d",
        m_device->Adapter(), m_device->Frontend(), m_bondedTuner->m_device->Adapter(), m_bondedTuner->m_device->Frontend(), tuner->m_device->Adapter(), tuner->m_device->Frontend());
  return false;
}

void cDvbTuner::UnBond(void)
{
  CLockObject lock(m_bondMutex);

  if (cDvbTuner *t = m_bondedTuner)
  {
    dsyslog("tuner %d/%d unbonded from tuner %d/%d", m_device->Adapter(), m_device->Frontend(), m_bondedTuner->m_device->Adapter(), m_bondedTuner->m_device->Frontend());
    while (t->m_bondedTuner != this)
      t = t->m_bondedTuner;
    if (t == m_bondedTuner)
      t->m_bondedTuner = NULL;
    else
      t->m_bondedTuner = m_bondedTuner;
    m_bBondedMaster = false; // another one will automatically become master whenever necessary
    m_bondedTuner = NULL;
  }
}

bool cDvbTuner::BondingOk(const cChannel& channel, bool bConsiderOccupied) const
{
  CLockObject lock(m_bondMutex);
  if (cDvbTuner *t = m_bondedTuner)
  {
    string BondingParams = GetBondingParams(channel);
    do
    {
      if (t->m_device->Receiver()->Priority() > IDLEPRIORITY || (bConsiderOccupied && t->m_device->Channel()->Occupied()))
      {
        if (BondingParams != t->GetBondedMaster()->GetBondingParams())
          return false;
      }
      t = t->m_bondedTuner;
    } while (t != m_bondedTuner);
  }
  return true;
}

string cDvbTuner::GetBondingParams(const cChannel& channel) const
{
  cDvbTransponderParams dtp(channel.Parameters());
  if (cSettings::Get().m_bDiSEqC)
  {
    if (const cDiseqc* diseqc = Diseqcs.Get(m_device->CardIndex() + 1, channel.Source(), channel.FrequencyKHz(), dtp.Polarization(), NULL))
      return diseqc->Commands();
  }
  else
  {
    bool ToneOff = channel.FrequencyHz() < cSettings::Get().m_iLnbSLOF;

    bool VoltOff = (dtp.Polarization() == POLARIZATION_VERTICAL ||
                    dtp.Polarization() == POLARIZATION_CIRCULAR_RIGHT);

    return StringUtils::Format("%c %c", ToneOff ? 't' : 'T', VoltOff ? 'v' : 'V');
  }

  return "";
}

cDvbTuner *cDvbTuner::GetBondedMaster(void)
{
  if (!m_bondedTuner)
    return this; // an unbonded tuner is always "master"
  CLockObject lock(m_bondMutex);
  if (m_bBondedMaster)
    return this;
  // This tuner is bonded, but it's not the master, so let's see if there is a master at all:
  if (cDvbTuner *t = m_bondedTuner)
  {
    while (t != this)
    {
      if (t->m_bBondedMaster)
        return t;
      t = t->m_bondedTuner;
    }
  }
  // None of the other bonded tuners is master, so make this one the master:
  m_bBondedMaster = true;
  dsyslog("tuner %d/%d is now bonded master", m_device->Adapter(), m_device->Frontend());
  return this;
}

int cDvbTuner::GetSignalStrength(void) const
{
  ClearEventQueue();
  uint16_t Signal;

  while (ioctl(m_fileDescriptor, FE_READ_SIGNAL_STRENGTH, &Signal) < 0)
  {
    if (errno != EINTR)
      return -1;
  }

  uint16_t MaxSignal = 0xFFFF; // Let's assume the default is using the entire range.

  // Use the subsystemId to identify individual devices in case they need
  // special treatment to map their Signal value into the range 0...0xFFFF.
  switch (m_device->GetSubsystemId())
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
  fprintf(stderr, "FE %d/%d: %08X S = %04X %04X %3d%%\n", m_device->Adapter(), m_device->Frontend(), m_device->GetSubsystemId(), MaxSignal, Signal, s);
#endif

  return s;
}

#define LOCK_THRESHOLD 5 // indicates that all 5 FE_HAS_* flags are set

int cDvbTuner::GetSignalQuality(void) const
{
  fe_status_t Status;
  if (GetFrontendStatus(Status))
  {
    // Actually one would expect these checks to be done from FE_HAS_SIGNAL to FE_HAS_LOCK, but some drivers (like the stb0899) are broken, so FE_HAS_LOCK is the only one that (hopefully) is generally reliable...
    if ((Status & FE_HAS_LOCK) == 0)
    {
      if ((Status & FE_HAS_SIGNAL) == 0)
        return 0;
      if ((Status & FE_HAS_CARRIER) == 0)
        return 1;
      if ((Status & FE_HAS_VITERBI) == 0)
        return 2;
      if ((Status & FE_HAS_SYNC) == 0)
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
    switch (m_device->GetSubsystemId())
    {
    case 0x13C21019: // TT-budget S2-3200 (DVB-S/DVB-S2)
    case 0x1AE40001: // TechniSat SkyStar HD2 (DVB-S/DVB-S2)
      if (m_frontendType == SYS_DVBS2)
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
    fprintf(stderr, "FE %d/%d: %08X Q = %04X %04X %d %5d %5d %3d%%\n", m_device->Adapter(), m_device->Frontend(), m_device->GetSubsystemId(), MaxSnr, Snr, HasSnr, HasBer ? int(Ber) : -1, HasUnc ? int(Unc) : -1, q);
#endif

    return q;
  }
  return -1;
}

fe_delivery_system_t cDvbTuner::GetRequiredDeliverySystem(const cChannel &channel)
{
  cDvbTransponderParams dtp(channel.Parameters());

  fe_delivery_system_t ds = SYS_UNDEFINED;
  if (channel.IsAtsc())
    ds = SYS_ATSC;
  else if (channel.IsCable())
    ds = SYS_DVBC_ANNEX_AC;
  else if (channel.IsSat())
    ds = dtp.System() == DVB_SYSTEM_1 ? SYS_DVBS : SYS_DVBS2;
  else if (channel.IsTerr())
    ds = dtp.System() == DVB_SYSTEM_1 ? SYS_DVBT : SYS_DVBT2;
  else
    esyslog("ERROR: can't determine frontend type for channel %d", channel.Number());
  return ds;
}

string cDvbTuner::TranslateDeliverySystems(const vector<fe_delivery_system_t>& deliverySystems)
{
  vector<string> vecDeliverySystemNames;
  for (vector<fe_delivery_system_t>::const_iterator it = deliverySystems.begin(); it != deliverySystems.end(); ++it)
  {
    fe_delivery_system_t ds = *it;
    if (1 <= ds && ds < ARRAY_SIZE(DeliverySystemNames))
      vecDeliverySystemNames.push_back(DeliverySystemNames[ds]);
  }
  return StringUtils::Join(vecDeliverySystemNames, ",");
}

unsigned int cDvbTuner::GetLockTimeout(fe_delivery_system_t frontendType)
{
  switch (frontendType)
  {
  case SYS_DVBS:
  case SYS_DVBS2:
    return DVBS_LOCK_TIMEOUT_MS;
  case SYS_DVBC_ANNEX_AC:
  case SYS_DVBC_ANNEX_B:
    return DVBC_LOCK_TIMEOUT_MS;
  case SYS_DVBT:
  case SYS_DVBT2:
    return DVBT_LOCK_TIMEOUT_MS;
  case SYS_ATSC:
    return ATSC_LOCK_TIMEOUT_MS;
  default:
    return 0; // Unknown DVB frontend type, don't wait for a lock
  }
}

}
