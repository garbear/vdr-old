/*
 * dvbdevice.c: The DVB device tuner interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: dvbdevice.c 2.88.1.4 2013/10/21 09:01:21 kls Exp $
 */

#include "DVBTuner.h"
#include "DVBDevice.h"
#include "devices/DeviceManager.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DevicePIDSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "filesystem/Poller.h"
#include "sources/linux/DVBSourceParams.h"
#include "utils/StringUtils.h"
#include "dvb/DiSEqC.h"
//#include "utils/Tools.h"

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

using namespace std;
using namespace PLATFORM;

#define DVBS_TUNE_TIMEOUT  9000 //ms
#define DVBS_LOCK_TIMEOUT  2000 //ms
#define DVBC_TUNE_TIMEOUT  9000 //ms
#define DVBC_LOCK_TIMEOUT  2000 //ms
#define DVBT_TUNE_TIMEOUT  9000 //ms
#define DVBT_LOCK_TIMEOUT  2000 //ms
#define ATSC_TUNE_TIMEOUT  9000 //ms
#define ATSC_LOCK_TIMEOUT  2000 //ms

#define SCR_RANDOM_TIMEOUT  500 // ms (add random value up to this when tuning SCR device to avoid lockups)
#define TUNER_POLL_TIMEOUT  10  // ms

#define DEBUG_SIGNALSTRENGTH 1
#define DEBUG_SIGNALQUALITY 1

#define MAXFRONTENDCMDS 16

#define SETCMD(c, d) \
  do \
  { \
    frontend[CmdSeq.num].cmd = (c); \
    frontend[CmdSeq.num].u.data = (d); \
    if (CmdSeq.num++ > MAXFRONTENDCMDS) \
    { \
      esyslog("ERROR: too many tuning commands on frontend %d/%d", Adapter(), Frontend()); \
      return false; \
    } \
  } while (0)

#define INVALID_FD  -1

CMutex cDvbTuner::m_bondMutex;

cDvbTuner::cDvbTuner(cDvbDevice *device)
 : m_device(device),
   m_frontendType(SYS_UNDEFINED),
   m_frontendInfo(),
   m_fileDescriptor(INVALID_FD),
   m_dvbApiVersion(0),
   m_numModulations(0),
   m_tuneTimeout(0),
   m_lockTimeout(0),
   m_lastTimeoutReport(0),
   m_lastDiseqc(NULL),
   m_scr(NULL),
   m_bLnbPowerTurnedOn(false),
   m_tunerStatus(tsIdle),
   m_bondedTuner(NULL),
   m_bBondedMaster(false),
   m_bLocked(false),
   m_bNewSet(false)
{
}

bool cDvbTuner::Open()
{
  try
  {
    m_fileDescriptor = m_device->DvbOpen(DEV_DVB_FRONTEND, O_RDWR | O_NONBLOCK);

    if (m_fileDescriptor == INVALID_FD)
    {
      // TODO: If error is "Device or resource busy", kill the owner process
      if (errno == EBUSY)
        throw "DVB tuner: Frontend is held by another process (try `lsof +D /dev/dvb`)";

      throw "DVB tuner: Can't open frontend";
    }

    if (!QueryDvbApiVersion())
      throw "DVB tuner: Can't get DVB API version";

    if (IoControl(FE_GET_INFO, &m_frontendInfo) == -1 || m_frontendInfo.name == NULL)
      throw "DVB tuner: Can't get frontend info";

    m_deliverySystems = GetDeliverySystems();

    if (m_deliverySystems.empty())
      throw "DVB tuner: No delivery systems";

    m_strName = m_frontendInfo.name;

    vector<string> vecDeliverySystemNames;
    for (vector<fe_delivery_system>::const_iterator it = m_deliverySystems.begin() + 1; it != m_deliverySystems.end(); ++it)
      vecDeliverySystemNames.push_back(m_device->DeliverySystemNames[*it]);
    string deliverySystem = StringUtils::Join(vecDeliverySystemNames, ",");

    vector<string> vecModulations = m_device->GetModulationsFromCaps(m_frontendInfo.caps);
    string modulations = StringUtils::Join(vecModulations, ",");
    if (modulations.empty())
      modulations = "unknown modulations";

    //m_modulations = vecModulations;
    m_numModulations = vecModulations.size();
    isyslog("frontend %d/%d provides %s with %s (\"%s\")", Adapter(), Frontend(), deliverySystem.c_str(), modulations.c_str(), m_frontendInfo.name);

    //SetDescription("tuner on frontend %d/%d", Adapter(), Frontend());
    CreateThread();
  }
  catch (const char* errorMsg)
  {
    esyslog("%s", errorMsg);
    return false;
  }

  return true;
}

vector<fe_delivery_system> cDvbTuner::GetDeliverySystems() const
{
  vector<fe_delivery_system> deliverySystems;

  // Determine the types of delivery systems this device provides:
  bool LegacyMode = true;
  if (GetDvbApiVersion() >= 0x0505)
  {
    dtv_property frontend[1];
    dtv_properties CmdSeq;

    memset(&frontend, 0, sizeof(frontend));
    memset(&CmdSeq, 0, sizeof(CmdSeq));
    CmdSeq.props = frontend;

    frontend[CmdSeq.num].cmd = DTV_ENUM_DELSYS;
    if (CmdSeq.num++ > MAXFRONTENDCMDS)
    {
      assert(false); // TODO
    }

    int Result = IoControl(FE_GET_PROPERTY, &CmdSeq);
    if (Result == 0)
    {
      for (uint8_t* deliverySystem = frontend[0].u.buffer.data; deliverySystem != frontend[0].u.buffer.data + frontend[0].u.buffer.len; deliverySystem++)
        deliverySystems.push_back(static_cast<fe_delivery_system>(*deliverySystem));
      LegacyMode = false;
    }
    else
    {
      esyslog("ERROR: can't query delivery systems on frontend %d/%d - falling back to legacy mode", Adapter(), Frontend());
    }
  }

  if (LegacyMode)
  {
    // Legacy mode (DVB-API < 5.5):
    switch (m_frontendInfo.type)
    {
    case FE_QPSK:
      deliverySystems.push_back(SYS_DVBS);
      if (m_frontendInfo.caps & FE_CAN_2G_MODULATION)
        deliverySystems.push_back(SYS_DVBS2);
      break;
    case FE_OFDM:
      deliverySystems.push_back(SYS_DVBT);
      if (m_frontendInfo.caps & FE_CAN_2G_MODULATION)
        deliverySystems.push_back(SYS_DVBT2);
      break;
    case FE_QAM:
      deliverySystems.push_back(SYS_DVBC_ANNEX_AC);
      break;
    case FE_ATSC:
      deliverySystems.push_back(SYS_ATSC);
      break;
    default:
      esyslog("ERROR: unknown frontend type %d on frontend %d/%d", m_frontendInfo.type, Adapter(), Frontend());
      break;
    }
  }

  return deliverySystems;
}

void cDvbTuner::Close()
{
  if (m_fileDescriptor >= 0)
  {
    dsyslog("DVB tuner: Closing frontend");
    close(m_fileDescriptor);
    m_fileDescriptor = INVALID_FD;
  }

  SetTunerStatus(tsIdle);
  m_newSet.Broadcast();
  m_locked.Broadcast();
  StopThread(3000);
  UnBond();
  /* looks like this irritates the SCR switch, so let's leave it out for now
  if (m_lastDiseqc && m_lastDiseqc->IsScr())
  {
    unsigned int frequency = 0;
    ExecuteDiseqc(m_lastDiseqc, &frequency);
  }
  */

  m_deliverySystems.clear();
}

unsigned int cDvbTuner::Adapter() const
{
  return m_device->Adapter();
}

unsigned int cDvbTuner::Frontend() const
{
  return m_device->Frontend();
}

bool cDvbTuner::HasDeliverySystem(fe_delivery_system deliverySystem) const
{
  return std::find(m_deliverySystems.begin(), m_deliverySystems.end(), deliverySystem) != m_deliverySystems.end();
}

bool cDvbTuner::QueryDvbApiVersion()
{
  m_dvbApiVersion = 0x00000000;

  dtv_property frontend[1] = { };
  dtv_properties cmdSeq = { };

  memset(&frontend, 0, sizeof(frontend));
  memset(&cmdSeq, 0, sizeof(cmdSeq));

  cmdSeq.props = frontend;
  frontend[cmdSeq.num].cmd = DTV_API_VERSION;
  frontend[cmdSeq.num].u.data = 0;
  if (cmdSeq.num++ > MAXFRONTENDCMDS)
  {
    // Too many tuning commands on frontend
    return false;
  }

  if (IoControl(FE_GET_PROPERTY, &cmdSeq) == 0 && frontend[0].u.data != 0)
  {
    m_dvbApiVersion = frontend[0].u.data;
    isyslog("Detected DVB API version %08x", m_dvbApiVersion);
  }
  else
  {
    esyslog("Can't get DVB API version");
    return false;
  }

  return true;
}

void cDvbTuner::SetTunerStatus(eTunerStatus status)
{
  if (m_tunerStatus != status)
  {
    m_tunerStatus = status;
    const char* strStatus;

    switch (m_tunerStatus)
    {
    case tsIdle:
      strStatus = "idle";
      break;
    case tsSet:
      strStatus = "set frequency";
      break;
    case tsTuned:
      strStatus = "tuned";
      break;
    case tsLocked:
      strStatus = "locked";
      break;
    default:
      strStatus = "unknown";
      break;
    }

    dsyslog("tuner status of device '%s' changed to '%s'", Name().c_str(), strStatus);
  }
}

void *cDvbTuner::Process()
{
  cTimeMs Timer;
  bool LostLock = false;
  fe_status_t Status = (fe_status_t)0;

  dsyslog("tuner thread started for tuner '%s'", Name().c_str());
  while (!IsStopped())
  {
    fe_status_t NewStatus;
    if (GetFrontendStatus(NewStatus))
      Status = NewStatus;

    CLockObject lock(m_mutex);
    int WaitTime = 1000;
    m_bNewSet = false;
    switch (m_tunerStatus)
    {
    case tsIdle:
      break;
    case tsSet:
      SetTunerStatus(SetFrontend() ? tsTuned : tsIdle);
      Timer.Set(m_tuneTimeout + (m_scr ? rand() % SCR_RANDOM_TIMEOUT : 0));
      continue;
    case tsTuned:
      if (Timer.TimedOut())
      {
        SetTunerStatus(tsSet);
        m_lastDiseqc = NULL;
        if (time(NULL) - m_lastTimeoutReport > 60) // let's not get too many of these
        {
          isyslog("frontend %d/%d timed out while tuning to channel %d, tp %d", Adapter(), Frontend(), m_channel.Number(), m_channel.Transponder());
          m_lastTimeoutReport = time(NULL);
        }
        continue;
      }
      WaitTime = 100; // allows for a quick change from tsTuned to tsLocked
      // no break (TODO: Garrett's comment, not original)
    case tsLocked:
      if (Status & FE_REINIT)
      {
        SetTunerStatus(tsSet);
        m_lastDiseqc = NULL;
        isyslog("frontend %d/%d was reinitialized", Adapter(), Frontend());
        m_lastTimeoutReport = 0;
        continue;
      }
      else if (Status & FE_HAS_LOCK)
      {
        if (LostLock)
        {
          isyslog("frontend %d/%d regained lock on channel %d, tp %d", Adapter(), Frontend(), m_channel.Number(), m_channel.Transponder());
          LostLock = false;
        }
        SetTunerStatus(tsLocked);
        m_bLocked = m_tunerStatus >= tsLocked;
        m_locked.Broadcast();
        m_lastTimeoutReport = 0;
      }
      else if (m_tunerStatus == tsLocked)
      {
        LostLock = true;
        isyslog("frontend %d/%d lost lock on channel %d, tp %d", Adapter(), Frontend(), m_channel.Number(), m_channel.Transponder());
        SetTunerStatus(tsTuned);
        Timer.Set(m_lockTimeout);
        m_lastTimeoutReport = 0;
        continue;
      }
      break;
    default:
      esyslog("ERROR: unknown tuner status %d", m_tunerStatus);
      break;
    }
    m_newSet.Wait(m_mutex, m_bNewSet, WaitTime);
  }

  dsyslog("tuner thread exiting for tuner '%s'", Name().c_str());
  return NULL;
}

bool cDvbTuner::Bond(cDvbTuner *tuner)
{
  CLockObject lock(m_bondMutex);
  if (!m_bondedTuner)
  {
    ResetToneAndVoltage();
    m_bBondedMaster = false; // makes sure we don't disturb an existing master
    m_bondedTuner = tuner->m_bondedTuner ? tuner->m_bondedTuner : tuner;
    tuner->m_bondedTuner = this;
    dsyslog("tuner %d/%d bonded with tuner %d/%d", Adapter(), Frontend(), m_bondedTuner->Adapter(), m_bondedTuner->Frontend());
    return true;
  }
  else
    esyslog("ERROR: tuner %d/%d already bonded with tuner %d/%d, can't bond with tuner %d/%d",
        Adapter(), Frontend(), m_bondedTuner->Adapter(), m_bondedTuner->Frontend(), tuner->Adapter(), tuner->Frontend());
  return false;
}

void cDvbTuner::UnBond()
{
  CLockObject lock(m_bondMutex);
  if (cDvbTuner *t = m_bondedTuner)
  {
    dsyslog("tuner %d/%d unbonded from tuner %d/%d", Adapter(), Frontend(), m_bondedTuner->Adapter(), m_bondedTuner->Frontend());
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

string cDvbTuner::GetBondingParams(const cChannel &channel) const
{
  cDvbTransponderParams dtp(channel.Parameters());
  if (Setup.DiSEqC)
  {
    if (const cDiseqc *diseqc = Diseqcs.Get(m_device->CardIndex() + 1, channel.Source(), channel.Frequency(), dtp.Polarization(), NULL))
      return diseqc->Commands();
  }
  else
  {
    bool ToneOff = channel.Frequency() < Setup.LnbSLOF;
    bool VoltOff = dtp.Polarization() == 'V' || dtp.Polarization() == 'R';
    return StringUtils::Format("%c %c", ToneOff ? 't' : 'T', VoltOff ? 'v' : 'V');
  }
  return "";
}

bool cDvbTuner::BondingOk(const cChannel &channel, bool bConsiderOccupied) const
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

cDvbTuner *cDvbTuner::GetBondedMaster()
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
  dsyslog("tuner %d/%d is now bonded master", Adapter(), Frontend());
  return this;
}

bool cDvbTuner::IsTunedTo(const cChannel &channel) const
{
  if (m_tunerStatus == tsIdle)
    return false; // not tuned to
  if (m_channel.Source() != channel.Source() || m_channel.Transponder() != channel.Transponder())
    return false; // sufficient mismatch

  // Polarization is already checked as part of the Transponder.
  return strcmp(m_channel.Parameters().c_str(), channel.Parameters().c_str()) == 0;
}

void cDvbTuner::SetChannel(const cChannel &channel)
{
  if (m_bondedTuner)
  {
    CLockObject lock(m_bondMutex);
    cDvbTuner *BondedMaster = GetBondedMaster();
    if (BondedMaster == this)
    {
      if (GetBondingParams(channel) != GetBondingParams())
      {
        // switching to a completely different band, so set all others to idle:
        for (cDvbTuner *t = m_bondedTuner; t && t != this; t = t->m_bondedTuner)
          t->ClearChannel();
      }
    }
    else if (GetBondingParams(channel) != BondedMaster->GetBondingParams())
      BondedMaster->SetChannel(channel);
  }
  CLockObject lock(m_mutex);
  if (!IsTunedTo(channel))
    SetTunerStatus(tsSet);
  m_channel = channel;
  m_lastTimeoutReport = 0;
  m_bNewSet = true;
  m_newSet.Broadcast();

//  if (m_bondedTuner && m_device->IsPrimaryDevice())
//    cDeviceManager::Get().PrimaryDevice()->PID()->DelLivePids(); // 'device' is const, so we must do it this way
}

void cDvbTuner::ClearChannel()
{
  CLockObject lock(m_mutex);
  SetTunerStatus(tsIdle);
  ResetToneAndVoltage();

//  if (m_bondedTuner && m_device->IsPrimaryDevice())
//    cDeviceManager::Get().PrimaryDevice()->PID()->DelLivePids(); // 'device' is const, so we must do it this way
}

bool cDvbTuner::Locked(int timeoutMs)
{
  bool isLocked = (m_tunerStatus >= tsLocked);
  if (isLocked || !timeoutMs)
    return isLocked;

  CLockObject lock(m_mutex);

  m_bLocked = m_tunerStatus >= tsLocked;
  if (timeoutMs && m_tunerStatus < tsLocked)
    m_locked.Wait(m_mutex, m_bLocked, timeoutMs);
  return m_tunerStatus >= tsLocked;
}

void cDvbTuner::ClearEventQueue() const
{
  cPoller Poller(m_fileDescriptor);
  if (Poller.Poll(TUNER_POLL_TIMEOUT))
  {
    dvb_frontend_event Event;
    while (IoControl(FE_GET_EVENT, &Event) == 0)
      ; // just to clear the event queue - we'll read the actual status below
  }
}

bool cDvbTuner::GetFrontendStatus(fe_status_t &Status) const
{
  ClearEventQueue();

  do
  {
    if (IoControl(FE_READ_STATUS, &Status) != -1)
      return true;
  } while (errno == EINTR);

  return false;
}

int cDvbTuner::GetSignalStrength() const
{
  ClearEventQueue();
  uint16_t Signal;

  while (IoControl(FE_READ_SIGNAL_STRENGTH, &Signal) == -1)
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
  fprintf(stderr, "FE %d/%d: %08X S = %04X %04X %3d%%\n", Adapter(), Frontend(), m_device->GetSubsystemId(), MaxSignal, Signal, s);
#endif

  return s;
}

#define LOCK_THRESHOLD 5 // indicates that all 5 FE_HAS_* flags are set

int cDvbTuner::GetSignalQuality() const
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
      if (IoControl(FE_READ_SNR, &Snr) != -1)
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
      if (IoControl(FE_READ_BER, &Ber) != -1)
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
      if (IoControl(FE_READ_UNCORRECTED_BLOCKS, &Unc) != -1)
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
    fprintf(stderr, "FE %d/%d: %08X Q = %04X %04X %d %5d %5d %3d%%\n", Adapter(), Frontend(), m_device->GetSubsystemId(), MaxSnr, Snr, HasSnr, HasBer ? int(Ber) : -1, HasUnc ? int(Unc) : -1, q);
#endif

    return q;
  }
  return -1;
}

fe_delivery_system cDvbTuner::GetRequiredDeliverySystem(const cChannel &channel, const cDvbTransponderParams *dtp)
{
  fe_delivery_system ds = SYS_UNDEFINED;
  if (channel.IsAtsc())
    ds = SYS_ATSC;
  else if (channel.IsCable())
    ds = SYS_DVBC_ANNEX_AC;
  else if (channel.IsSat())
    ds = dtp->System() == DVB_SYSTEM_1 ? SYS_DVBS : SYS_DVBS2;
  else if (channel.IsTerr())
    ds = dtp->System() == DVB_SYSTEM_1 ? SYS_DVBT : SYS_DVBT2;
  else
    esyslog("ERROR: can't determine frontend type for channel %d", channel.Number());
  return ds;
}

int cDvbTuner::IoControl(unsigned long int request, void* param) const
{
  if (m_fileDescriptor < 0)
    return -1;

  return ioctl(m_fileDescriptor, request, param);
}

void cDvbTuner::ExecuteDiseqc(const cDiseqc *Diseqc, unsigned int *Frequency)
{
  if (!m_bLnbPowerTurnedOn)
  {
    CHECK(ioctl(m_fileDescriptor, FE_SET_VOLTAGE, SEC_VOLTAGE_13)); // must explicitly turn on LNB power
    m_bLnbPowerTurnedOn = true;
  }

  static cMutex Mutex;

  if (Diseqc->IsScr())
    Mutex.Lock();
  struct dvb_diseqc_master_cmd cmd;
  const char *CurrentAction = NULL;

  for (;;)
  {
    cmd.msg_len = sizeof(cmd.msg);
    cDiseqc::eDiseqcActions da = Diseqc->Execute(&CurrentAction, cmd.msg, &cmd.msg_len, m_scr, Frequency);
    if (da == cDiseqc::daNone)
      break;
    switch (da)
    {
    case cDiseqc::daToneOff:   CHECK(ioctl(m_fileDescriptor, FE_SET_TONE, SEC_TONE_OFF)); break;
    case cDiseqc::daToneOn:    CHECK(ioctl(m_fileDescriptor, FE_SET_TONE, SEC_TONE_ON)); break;
    case cDiseqc::daVoltage13: CHECK(ioctl(m_fileDescriptor, FE_SET_VOLTAGE, SEC_VOLTAGE_13)); break;
    case cDiseqc::daVoltage18: CHECK(ioctl(m_fileDescriptor, FE_SET_VOLTAGE, SEC_VOLTAGE_18)); break;
    case cDiseqc::daMiniA:     CHECK(ioctl(m_fileDescriptor, FE_DISEQC_SEND_BURST, SEC_MINI_A)); break;
    case cDiseqc::daMiniB:     CHECK(ioctl(m_fileDescriptor, FE_DISEQC_SEND_BURST, SEC_MINI_B)); break;
    case cDiseqc::daCodes:     CHECK(IoControl(FE_DISEQC_SEND_MASTER_CMD, &cmd)); break;
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

void cDvbTuner::ResetToneAndVoltage()
{
  CHECK(ioctl(m_fileDescriptor, FE_SET_VOLTAGE, m_bondedTuner ? SEC_VOLTAGE_OFF : SEC_VOLTAGE_13));
  CHECK(ioctl(m_fileDescriptor, FE_SET_TONE, SEC_TONE_OFF));
}

bool cDvbTuner::SetFrontend()
{
  dtv_property frontend[MAXFRONTENDCMDS];
  memset(&frontend, 0, sizeof(frontend));
  dtv_properties CmdSeq;
  memset(&CmdSeq, 0, sizeof(CmdSeq));
  CmdSeq.props = frontend;
  SETCMD(DTV_CLEAR, 0);
  if (IoControl(FE_SET_PROPERTY, &CmdSeq) < 0)
  {
    esyslog("ERROR: frontend %d/%d: %m", Adapter(), Frontend());
    return false;
  }
  CmdSeq.num = 0;

  cDvbTransponderParams dtp(m_channel.Parameters());

  // Determine the required frontend type:
  m_frontendType = GetRequiredDeliverySystem(m_channel, &dtp);
  if (m_frontendType == SYS_UNDEFINED)
    return false;

  dsyslog("tuner '%s' tuning to frequency %u", Name().c_str(), m_channel.Frequency());
  SETCMD(DTV_DELIVERY_SYSTEM, m_frontendType);
  if (m_frontendType == SYS_DVBS || m_frontendType == SYS_DVBS2)
  {
    unsigned int frequency = m_channel.Frequency();
    if (Setup.DiSEqC)
    {
      if (const cDiseqc *diseqc = Diseqcs.Get(m_device->CardIndex() + 1, m_channel.Source(), frequency, dtp.Polarization(), &m_scr))
      {
        frequency -= diseqc->Lof();
        if (diseqc != m_lastDiseqc || diseqc->IsScr())
        {
          if (IsBondedMaster())
          {
            ExecuteDiseqc(diseqc, &frequency);
            if (frequency == 0)
              return false;
          }
          else
            ResetToneAndVoltage();
          m_lastDiseqc = diseqc;
        }
      }
      else
      {
        esyslog("ERROR: no DiSEqC parameters found for channel %d", m_channel.Number());
        return false;
      }
    }
    else
    {
      int tone = SEC_TONE_OFF;
      if (frequency < (unsigned int)Setup.LnbSLOF)
      {
        frequency -= Setup.LnbFrequLo;
        tone = SEC_TONE_OFF;
      }
      else
      {
        frequency -= Setup.LnbFrequHi;
        tone = SEC_TONE_ON;
      }
      int volt = (dtp.Polarization() == 'V' || dtp.Polarization() == 'R') ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
      if (!IsBondedMaster())
      {
        tone = SEC_TONE_OFF;
        volt = SEC_VOLTAGE_13;
      }
      CHECK(ioctl(m_fileDescriptor, FE_SET_VOLTAGE, volt));
      CHECK(ioctl(m_fileDescriptor, FE_SET_TONE, tone));
    }

    frequency = ::abs(frequency); // Allow for C-band, where the frequency is less than the LOF

    // DVB-S/DVB-S2 (common parts)
    SETCMD(DTV_FREQUENCY, frequency * 1000UL);
    SETCMD(DTV_MODULATION, dtp.Modulation());
    SETCMD(DTV_SYMBOL_RATE, m_channel.Srate() * 1000UL);
    SETCMD(DTV_INNER_FEC, dtp.CoderateH());
    SETCMD(DTV_INVERSION, dtp.Inversion());
    if (m_frontendType == SYS_DVBS2)
    {
      // DVB-S2
      SETCMD(DTV_PILOT, PILOT_AUTO);
      SETCMD(DTV_ROLLOFF, dtp.RollOff());
      if (GetDvbApiVersion() >= 0x0508)
        SETCMD(DTV_STREAM_ID, dtp.StreamId());
    }
    else
    {
      // DVB-S
      SETCMD(DTV_ROLLOFF, ROLLOFF_35); // DVB-S always has a ROLLOFF of 0.35
    }

    m_tuneTimeout = DVBS_TUNE_TIMEOUT;
    m_lockTimeout = DVBS_LOCK_TIMEOUT;
  }
  else if (m_frontendType == SYS_DVBC_ANNEX_AC || m_frontendType == SYS_DVBC_ANNEX_B)
  {
    // DVB-C
    SETCMD(DTV_FREQUENCY, FrequencyToHz(m_channel.Frequency()));
    SETCMD(DTV_INVERSION, dtp.Inversion());
    SETCMD(DTV_SYMBOL_RATE, m_channel.Srate() * 1000UL);
    SETCMD(DTV_INNER_FEC, dtp.CoderateH());
    SETCMD(DTV_MODULATION, dtp.Modulation());

    m_tuneTimeout = DVBC_TUNE_TIMEOUT;
    m_lockTimeout = DVBC_LOCK_TIMEOUT;
  }
  else if (m_frontendType == SYS_DVBT || m_frontendType == SYS_DVBT2)
  {
    // DVB-T/DVB-T2 (common parts)
    SETCMD(DTV_FREQUENCY, FrequencyToHz(m_channel.Frequency()));
    SETCMD(DTV_INVERSION, dtp.Inversion());
    SETCMD(DTV_BANDWIDTH_HZ, dtp.Bandwidth());
    SETCMD(DTV_CODE_RATE_HP, dtp.CoderateH());
    SETCMD(DTV_CODE_RATE_LP, dtp.CoderateL());
    SETCMD(DTV_MODULATION, dtp.Modulation());
    SETCMD(DTV_TRANSMISSION_MODE, dtp.Transmission());
    SETCMD(DTV_GUARD_INTERVAL, dtp.Guard());
    SETCMD(DTV_HIERARCHY, dtp.Hierarchy());
    if (m_frontendType == SYS_DVBT2)
    {
      // DVB-T2
      if (GetDvbApiVersion() >= 0x0508)
        SETCMD(DTV_STREAM_ID, dtp.StreamId());
      else if (GetDvbApiVersion() >= 0x0503)
        SETCMD(DTV_DVBT2_PLP_ID_LEGACY, dtp.StreamId());
    }

    m_tuneTimeout = DVBT_TUNE_TIMEOUT;
    m_lockTimeout = DVBT_LOCK_TIMEOUT;
  }
  else if (m_frontendType == SYS_ATSC)
  {
    // ATSC
    SETCMD(DTV_FREQUENCY, FrequencyToHz(m_channel.Frequency()));
    SETCMD(DTV_INVERSION, dtp.Inversion());
    SETCMD(DTV_MODULATION, dtp.Modulation());

    m_tuneTimeout = ATSC_TUNE_TIMEOUT;
    m_lockTimeout = ATSC_LOCK_TIMEOUT;
  }
  else
  {
    esyslog("ERROR: attempt to set channel with unknown DVB frontend type");
    return false;
  }

  SETCMD(DTV_TUNE, 0);

  if (IoControl(FE_SET_PROPERTY, &CmdSeq) < 0)
  {
    esyslog("ERROR: frontend %d/%d: %m", Adapter(), Frontend());
    return false;
  }
  return true;
}

unsigned int cDvbTuner::FrequencyToHz(unsigned int f)
{
  while (f && f < 1000000)
    f *= 1000;
  return f;
}
