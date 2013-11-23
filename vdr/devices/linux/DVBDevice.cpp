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

#include "DVBDevice.h"
#include "DVBDeviceProbe.h"
#include "subsystems/DVBAudioSubsystem.h"
#include "subsystems/DVBChannelSubsystem.h"
#include "subsystems/DVBCommonInterfaceSubsystem.h"
#include "subsystems/DVBPIDSubsystem.h"
#include "subsystems/DVBReceiverSubsystem.h"
#include "subsystems/DVBSectionFilterSubsystem.h"
#include "../../devices/subsystems/DeviceImageGrabSubsystem.h"
#include "../../devices/subsystems/DevicePlayerSubsystem.h"
#include "../../devices/subsystems/DeviceSPUSubsystem.h"
#include "../../devices/subsystems/DeviceTrackSubsystem.h"
#include "../../devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/ReadDir.h"
#include "sources/linux/DVBSourceParams.h"
#include "../../utils/StringUtils.h"
#include "../../../dvbci.h"
#include "../../../i18n.h"
#include "../../../tools.h"

#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

using namespace std;

#define MAXFRONTENDCMDS 16

#define SETCMD(c, d) \
{ \
  Frontend[CmdSeq.num].cmd = (c); \
  Frontend[CmdSeq.num].u.data = (d); \
  if (CmdSeq.num++ > MAXFRONTENDCMDS) \
  { \
    esyslog("ERROR: too many tuning commands on frontend %d/%d", m_adapter, m_frontend); \
    return false; \
  } \
}

int    cDvbDevice::m_dvbApiVersion = 0x0000;
cMutex cDvbDevice::m_bondMutex;

const char *DeliverySystemNames[] =
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
  NULL
};

cDvbDevice::cDvbDevice(unsigned int adapter, unsigned int frontend)
 : cDevice(CreateSubsystems(), 0 /* ??? */)
{
  m_adapter = Adapter;
  m_frontend = Frontend;
  m_numDeliverySystems = 0;
  m_numModulations = 0;
  m_bondedDevice = NULL;
  m_bNeedsDetachBondedReceivers = false;

  // Devices that are present on all card types:
  int fd_frontend = DvbOpen(DEV_DVB_FRONTEND, m_adapter, m_frontend, O_RDWR | O_NONBLOCK);

  // Common Interface:
  m_fd_ca = DvbOpen(DEV_DVB_CA, m_adapter, m_frontend, O_RDWR);
  if (m_fd_ca >= 0)
    DvbCommonInterface()->m_ciAdapter = cDvbCiAdapter::CreateCiAdapter(this, m_fd_ca);

  // We only check the devices that must be present - the others will be checked before accessing them://XXX
  if (fd_frontend >= 0)
  {
    if (QueryDeliverySystems(fd_frontend))
      DvbChannel()->m_dvbTuner = new cDvbTuner(this, fd_frontend, m_adapter, m_frontend);
  }
  else
    esyslog("ERROR: can't open DVB device %d/%d", m_adapter, m_frontend);

  SectionFilter()->StartSectionHandler();
}

const cSubsystems &cDvbDevice::CreateSubsystems()
{
  m_subsystems.Audio           = new cDvbAudioSubsystem(this);
  m_subsystems.Channel         = new cDvbChannelSubsystem(this);
  m_subsystems.CommonInterface = new cDvbCommonInterfaceSubsystem(this);
  m_subsystems.PID             = new cDvbPIDSubsystem(this);
  m_subsystems.Receiver        = new cDvbReceiverSubsystem(this);
  m_subsystems.SectionFilter   = new cDvbSectionFilterSubsystem(this);

  m_subsystems.ImageGrab       = new cDeviceImageGrabSubsystem(this);
  m_subsystems.Player          = new cDevicePlayerSubsystem(this);
  m_subsystems.SPU             = new cDeviceSPUSubsystem(this);
  m_subsystems.Track           = new cDeviceTrackSubsystem(this);
  m_subsystems.VideoFormat     = new cDeviceVideoFormatSubsystem(this);

  return m_subsystems;
}

cDvbDevice::~cDvbDevice()
{
  SectionFilter()->StopSectionHandler();
  UnBond();
  // We're not explicitly closing any device files here, since this sometimes
  // caused segfaults. Besides, the program is about to terminate anyway...
}

bool cDvbDevice::Initialize()
{
  gSourceParams['A'] = cDvbSourceParams('A', "ATSC");
  gSourceParams['C'] = cDvbSourceParams('C', "DVB-C");
  gSourceParams['S'] = cDvbSourceParams('S', "DVB-S");
  gSourceParams['T'] = cDvbSourceParams('T', "DVB-T");

  vector<string> vecNodes;

  // Enumerate /dev/dvb
  //cDirectoryListing items;
  vector<string> vecNodes;

  //if (cDirectory::GetDirectory(DEV_DVB_BASE, items))
  cReadDir DvbDir(DEV_DVB_BASE);
  if (DvbDir.Ok())
  {
    //for (cDirectoryListing::const_iterator itemIt = items.begin(); itemIt != items.end(); ++itemIt)
    struct dirent *a;
    while ((a = DvbDir.Next()) != NULL)
    {
      // Adapter node must begin with "adapter"
      //if (!itemIt->Name().find(DEV_DVB_ADAPTER) != string::npos)
      if (strstr(a->d_name, DEV_DVB_ADAPTER) != a->d_name)
        continue;

      // Get adapter index from directory name
      long adapter;
      //if (!StringUtils::IntVal(itemIt->Name().substr(strlen(DEV_DVB_ADAPTER)), adapter))
      //  continue;
      adapter = strtol(a->d_name + strlen(DEV_DVB_ADAPTER), NULL, 10);

      // Enumerate /dev/dvb/adapterN
      //cDirectoryListing items2;
      // TODO: Need AddFileToDirectory() function
      //if (!cDirectory::GetDirectory(DEV_DVB_BASE "/" + itemIt->Name(), items))
      cReadDir AdapterDir(AddDirectory(DEV_DVB_BASE, a->d_name));
      if (AdapterDir.Ok())
      {
        //for (cDirectoryListing::const_iterator itemIt2 = items2.begin(); itemIt2 != items2.end(); ++itemIt2)
        struct dirent *f;
        while ((f = AdapterDir.Next()) != NULL)
        {
          // Frontend node must begin with "frontend"
          //if (!itemIt->Name().find(DEV_DVB_FRONTEND) != string::npos)
          if (strstr(f->d_name, DEV_DVB_FRONTEND) != f->d_name)
            continue;

          // Get frontend index from directory name
          long frontend;
          //if (!StringUtils::IntVal(itemIt2->Name().substr(strlen(DEV_DVB_FRONTEND)), frontend))
          //  continue;
          frontend = strtol(f->d_name + strlen(DEV_DVB_FRONTEND), NULL, 10);

          // Got our (adapter, frontend) pair, add it to the array
          vecNodes.push_back(StringUtils::Format("%2d %2d", adapter, frontend));
        }
      }
    }
  }

  unsigned int found = 0;
  if (!vecNodes.empty())
  {
    std::sort(vecNodes.begin(), vecNodes.end());
    for (vector<string>::const_iterator nodeIt = vecNodes.begin(); nodeIt != vecNodes.end(); ++nodeIt)
    {
      int adapter;
      int frontend;
      if (2 == sscanf(nodeIt->c_str(), "%d %d", &adapter, &frontend))
      {
        if (Exists(adapter, frontend))
        {
          if (cDeviceManager::Get().UseDevice(0/*cDeviceManager::Get().NextCardIndex()*/))
          {
            if (Probe(adapter, frontend))
              found++;
          }
          else
            cDeviceManager::Get().AdvanceCardIndex(1); // skips this one
        }
      }
    }
  }

  //cDeviceManager::Get().NextCardIndex(MAXDVBDEVICES - Checked); // skips the rest

  isyslog("Found %d DVB device%s", found, found == 1 ? "" : "s");

  return found > 0;
}

bool cDvbDevice::Ready()
{
  if (DvbCommonInterface()->m_ciAdapter)
    return DvbCommonInterface()->m_ciAdapter->Ready();
  return true;
}

string cDvbDevice::DeviceType() const
{
  if (DvbChannel()->m_dvbTuner)
  {
    if (DvbChannel()->m_dvbTuner->FrontendType() != SYS_UNDEFINED)
      return DeliverySystemNames[DvbChannel()->m_dvbTuner->FrontendType()];
    if (m_numDeliverySystems)
      return DeliverySystemNames[m_deliverySystems[0]]; // to have some reasonable default
  }
  return "";
}

string cDvbDevice::DeviceName() const
{
  return m_frontendInfo.name;
}

bool cDvbDevice::Bond(cDvbDevice *device)
{
  cMutexLock MutexLock(&m_bondMutex);

  if (!m_bondedDevice)
  {
    if (device != this)
    {
      if ((DvbChannel()->ProvidesDeliverySystem(SYS_DVBS) || DvbChannel()->ProvidesDeliverySystem(SYS_DVBS2)) &&
          (device->DvbChannel()->ProvidesDeliverySystem(SYS_DVBS) || device->DvbChannel()->ProvidesDeliverySystem(SYS_DVBS2)))
      {
        if (DvbChannel()->m_dvbTuner && device->DvbChannel()->m_dvbTuner &&
            DvbChannel()->m_dvbTuner->Bond(device->DvbChannel()->m_dvbTuner))
        {
          m_bondedDevice = device->m_bondedDevice ? device->m_bondedDevice : device;
          device->m_bondedDevice = this;
          dsyslog("device %d bonded with device %d", CardIndex() + 1, m_bondedDevice->CardIndex() + 1);
          return true;
        }
      }
      else
        esyslog("ERROR: can't bond device %d with device %d (only DVB-S(2) devices can be bonded)", CardIndex() + 1, device->CardIndex() + 1);
    }
    else
      esyslog("ERROR: can't bond device %d with itself", CardIndex() + 1);
  }
  else
    esyslog("ERROR: device %d already bonded with device %d, can't bond with device %d", CardIndex() + 1, m_bondedDevice->CardIndex() + 1, device->CardIndex() + 1);

  return false;
}

void cDvbDevice::UnBond()
{
  cMutexLock MutexLock(&m_bondMutex);
  if (cDvbDevice *d = m_bondedDevice)
  {
    if (DvbChannel()->m_dvbTuner)
      DvbChannel()->m_dvbTuner->UnBond();
    dsyslog("device %d unbonded from device %d", CardIndex() + 1, m_bondedDevice->CardIndex() + 1);
    while (d->m_bondedDevice != this)
      d = d->m_bondedDevice;
    if (d == m_bondedDevice)
      d->m_bondedDevice = NULL;
    else
      d->m_bondedDevice = m_bondedDevice;
    m_bondedDevice = NULL;
  }
}

bool cDvbDevice::BondingOk(const cChannel &channel, bool bConsiderOccupied) const
{
  cMutexLock MutexLock(&m_bondMutex);
  if (m_bondedDevice)
    return DvbChannel()->m_dvbTuner && DvbChannel()->m_dvbTuner->BondingOk(&channel, bConsiderOccupied);
  return true;
}

bool cDvbDevice::BondDevices(const char *bondings)
{
  UnBondDevices();
  if (bondings)
  {
    cSatCableNumbers SatCableNumbers(MAXDEVICES, bondings);
    for (int i = 0; i < cDeviceManager::Get().NumDevices(); i++)
    {
      int d = SatCableNumbers.FirstDeviceIndex(i);
      if (d >= 0)
      {
        int ErrorDevice = 0;
        if (cDevice *Device1 = cDeviceManager::Get().GetDevice(i))
        {
          if (cDevice *Device2 = cDeviceManager::Get().GetDevice(d))
          {
            if (cDvbDevice *DvbDevice1 = dynamic_cast<cDvbDevice *>(Device1))
            {
              if (cDvbDevice *DvbDevice2 = dynamic_cast<cDvbDevice *>(Device2))
              {
                if (!DvbDevice1->Bond(DvbDevice2))
                  return false; // Bond() has already logged the error
              }
              else
                ErrorDevice = d + 1;
            }
            else
              ErrorDevice = i + 1;

            if (ErrorDevice)
            {
              esyslog("ERROR: device '%d' in device bondings '%s' is not a cDvbDevice", ErrorDevice, bondings);
              return false;
            }
          }
          else
            ErrorDevice = d + 1;
        }
        else
          ErrorDevice = i + 1;

        if (ErrorDevice)
        {
          esyslog("ERROR: unknown device '%d' in device bondings '%s'", ErrorDevice, bondings);
          return false;
        }
      }
    }
  }
  return true;
}

void cDvbDevice::UnBondDevices()
{
  for (int i = 0; i < cDeviceManager::Get().NumDevices(); i++)
  {
    cDvbDevice *device = dynamic_cast<cDvbDevice*>(cDeviceManager::Get().GetDevice(i));
    if (device)
      device->UnBond();
  }
}

string cDvbDevice::DvbName(const char *name, unsigned int adapter, unsigned int frontend)
{
  return StringUtils::Format("%s/%s%d/%s%d", DEV_DVB_BASE, DEV_DVB_ADAPTER, adapter, name, frontend);
}

int cDvbDevice::DvbOpen(const char *name, unsigned int adapter, unsigned int frontend, int mode, bool bReportError /* = false */)
{
  string fileName = DvbName(name, adapter, frontend);

  int fd = open(fileName.c_str(), mode);
  if (fd < 0 && bReportError)
    LOG_ERROR_STR(fileName.c_str());

  return fd;
}

bool cDvbDevice::Exists(unsigned int adapter, unsigned int frontend)
{
  cFile frontendNode;
  return frontendNode.Open(DvbName(DEV_DVB_FRONTEND, adapter, frontend));
}

bool cDvbDevice::Probe(unsigned int adapter, unsigned int frontend)
{
  string fileName = DvbName(DEV_DVB_FRONTEND, adapter, frontend);
  dsyslog("probing %s", fileName.c_str());

  for (cDvbDeviceProbe *dp = DvbDeviceProbes.First(); dp; dp = DvbDeviceProbes.Next(dp))
  {
    if (dp->Probe(adapter, frontend))
      return true; // a plugin has created the actual device
  }

  dsyslog("creating cDvbDevice");
  new cDvbDevice(adapter, frontend); // it's a "budget" device
  return true;
}

bool cDvbDevice::QueryDeliverySystems(int fd_frontend)
{
  m_numDeliverySystems = 0;
  if (ioctl(fd_frontend, FE_GET_INFO, &m_frontendInfo) < 0)
  {
    LOG_ERROR;
    return false;
  }

  if (!FindDvbApiVersion(fd_frontend))
    return false;

  dtv_property Frontend[1];
  dtv_properties CmdSeq;

  // Determine the types of delivery systems this device provides:
  bool LegacyMode = true;
  if (m_dvbApiVersion >= 0x0505)
  {
    memset(&Frontend, 0, sizeof(Frontend));
    memset(&CmdSeq, 0, sizeof(CmdSeq));
    CmdSeq.props = Frontend;
    SETCMD(DTV_ENUM_DELSYS, 0);
    int Result = ioctl(fd_frontend, FE_GET_PROPERTY, &CmdSeq);
    if (Result == 0)
    {
      for (uint i = 0; i < Frontend[0].u.buffer.len; i++)
      {
        if (m_numDeliverySystems >= MAXDELIVERYSYSTEMS)
        {
          esyslog("ERROR: too many delivery systems on frontend %d/%d", m_adapter, m_frontend);
          break;
        }
        m_deliverySystems[m_numDeliverySystems++] = Frontend[0].u.buffer.data[i];
      }
      LegacyMode = false;
    }
    else
    {
      esyslog("ERROR: can't query delivery systems on frontend %d/%d - falling back to legacy mode", m_adapter, m_frontend);
    }
  }

  if (LegacyMode)
  {
    // Legacy mode (DVB-API < 5.5):
    switch (m_frontendInfo.type)
    {
    case FE_QPSK:
      m_deliverySystems[m_numDeliverySystems++] = SYS_DVBS;
      if (m_frontendInfo.caps & FE_CAN_2G_MODULATION)
        m_deliverySystems[m_numDeliverySystems++] = SYS_DVBS2;
      break;
    case FE_OFDM:
      m_deliverySystems[m_numDeliverySystems++] = SYS_DVBT;
      if (m_frontendInfo.caps & FE_CAN_2G_MODULATION)
        m_deliverySystems[m_numDeliverySystems++] = SYS_DVBT2;
      break;
    case FE_QAM:
      m_deliverySystems[m_numDeliverySystems++] = SYS_DVBC_ANNEX_AC;
      break;
    case FE_ATSC:
      m_deliverySystems[m_numDeliverySystems++] = SYS_ATSC;
      break;
    default:
      esyslog("ERROR: unknown frontend type %d on frontend %d/%d", m_frontendInfo.type, m_adapter, m_frontend);
      break;
    }
  }

  if (m_numDeliverySystems > 0)
  {
    string ds;
    for (int i = 0; i < m_numDeliverySystems; i++)
      ds = StringUtils::Format("%s%s%s", ds.c_str(), i ? "," : "", DeliverySystemNames[m_deliverySystems[i]]);

    string ms;
    if (m_frontendInfo.caps & FE_CAN_QPSK)      { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", cDvbTransponderParams::TranslateModulation(QPSK)); }
    if (m_frontendInfo.caps & FE_CAN_QAM_16)    { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", cDvbTransponderParams::TranslateModulation(QAM_16)); }
    if (m_frontendInfo.caps & FE_CAN_QAM_32)    { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", cDvbTransponderParams::TranslateModulation(QAM_32)); }
    if (m_frontendInfo.caps & FE_CAN_QAM_64)    { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", cDvbTransponderParams::TranslateModulation(QAM_64)); }
    if (m_frontendInfo.caps & FE_CAN_QAM_128)   { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", cDvbTransponderParams::TranslateModulation(QAM_128)); }
    if (m_frontendInfo.caps & FE_CAN_QAM_256)   { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", cDvbTransponderParams::TranslateModulation(QAM_256)); }
    if (m_frontendInfo.caps & FE_CAN_8VSB)      { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", cDvbTransponderParams::TranslateModulation(VSB_8)); }
    if (m_frontendInfo.caps & FE_CAN_16VSB)     { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", cDvbTransponderParams::TranslateModulation(VSB_16)); }
    if (m_frontendInfo.caps & FE_CAN_TURBO_FEC) { m_numModulations++; ms += StringUtils::Format("%s%s", !ms.empty() ? "," : "", "TURBO_FEC"); }
    if (ms.empty())
      ms = "unknown modulations";

    isyslog("frontend %d/%d provides %s with %s (\"%s\")", m_adapter, m_frontend, ds.c_str(), ms.c_str(), m_frontendInfo.name);
    return true;
  }
  else
    esyslog("ERROR: frontend %d/%d doesn't provide any delivery systems", m_adapter, m_frontend);

  return false;
}

bool cDvbDevice::FindDvbApiVersion(int fd_frontend)
{
  if (!m_dvbApiVersion)
  {
    dtv_property Frontend[1];
    dtv_properties CmdSeq;

    memset(&Frontend, 0, sizeof(Frontend));
    memset(&CmdSeq, 0, sizeof(CmdSeq));

    CmdSeq.props = Frontend;
    Frontend[CmdSeq.num].cmd = DTV_API_VERSION;
    Frontend[CmdSeq.num].u.data = 0;
    if (CmdSeq.num++ > MAXFRONTENDCMDS)
    {
      esyslog("ERROR: too many tuning commands on frontend %d/%d", m_adapter, m_frontend);
      return false;
    }

    if (ioctl(fd_frontend, FE_GET_PROPERTY, &CmdSeq) != 0)
    {
      LOG_ERROR;
      return false;
    }
    m_dvbApiVersion = Frontend[0].u.data;
    isyslog("DVB API version is 0x%04X (VDR was built with 0x%04X)", m_dvbApiVersion, DVBAPIVERSION);
  }
  return true;
}
