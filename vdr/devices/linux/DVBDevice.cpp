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
#include "DVBTuner.h"
#include "devices/DeviceManager.h"
#include "devices/linux/commoninterface/DVBCIAdapter.h"
#include "devices/linux/subsystems/DVBChannelSubsystem.h"
#include "devices/linux/subsystems/DVBCommonInterfaceSubsystem.h"
#include "devices/linux/subsystems/DVBPIDSubsystem.h"
#include "devices/linux/subsystems/DVBReceiverSubsystem.h"
#include "devices/linux/subsystems/DVBSectionFilterSubsystem.h"
#include "devices/subsystems/DeviceImageGrabSubsystem.h"
#include "devices/subsystems/DevicePlayerSubsystem.h"
#include "devices/subsystems/DeviceSPUSubsystem.h"
#include "devices/subsystems/DeviceTrackSubsystem.h"
#include "devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "filesystem/ReadDir.h"
#include "sources/linux/DVBSourceParams.h"
#include "utils/StringUtils.h"
#include "utils/I18N.h"
#include "utils/Tools.h"
#include "utils/url/URLUtils.h"
#include "settings/Settings.h"

#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include "shared_ptr/shared_ptr.hpp"
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

using namespace std;
using namespace PLATFORM;
using namespace VDR; // for shared_ptr

#define MAXFRONTENDCMDS 16

CMutex cDvbDevice::m_bondMutex;

const char* cDvbDevice::DeliverySystemNames[] =
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
 : cDevice(CreateSubsystems(this), 0 /* ??? */),
   m_adapter(adapter),
   m_frontend(frontend),
   m_bondedDevice(NULL),
   m_bNeedsDetachBondedReceivers(false)
{
  // Common Interface:
  m_fd_ca = DvbOpen(DEV_DVB_CA, O_RDWR);
  if (m_fd_ca >= 0)
    DvbCommonInterface()->m_ciAdapter = cDvbCiAdapter::CreateCiAdapter(this, m_fd_ca);

  cDvbTuner *tuner = new cDvbTuner(this);
  if (tuner && tuner->IsValid())
    DvbChannel()->SetTuner(tuner);
  else
  {
    isyslog("Could not open tuner. Continuing without.");
    delete tuner;
  }

  SectionFilter()->StartSectionHandler();
}

cSubsystems cDvbDevice::CreateSubsystems(cDvbDevice* device)
{
  cSubsystems subsystems;

  subsystems.Channel         = new cDvbChannelSubsystem(device);
  subsystems.CommonInterface = new cDvbCommonInterfaceSubsystem(device);
  subsystems.PID             = new cDvbPIDSubsystem(device);
  subsystems.Receiver        = new cDvbReceiverSubsystem(device);
  subsystems.SectionFilter   = new cDvbSectionFilterSubsystem(device);

  subsystems.ImageGrab       = new cDeviceImageGrabSubsystem(device);
  subsystems.Player          = new cDevicePlayerSubsystem(device);
  subsystems.SPU             = new cDeviceSPUSubsystem(device);
  subsystems.Track           = new cDeviceTrackSubsystem(device);
  subsystems.VideoFormat     = new cDeviceVideoFormatSubsystem(device);

  return subsystems;
}

cDvbDevice::~cDvbDevice()
{
  SectionFilter()->StopSectionHandler();
  UnBond();
  // We're not explicitly closing any device files here, since this sometimes
  // caused segfaults. Besides, the program is about to terminate anyway...
}

vector<string> cDvbDevice::GetModulationsFromCaps(fe_caps_t caps)
{
  vector<string> modulations;

  if (caps & FE_CAN_QPSK)      { modulations.push_back(cDvbTransponderParams::TranslateModulation(QPSK)); }
  if (caps & FE_CAN_QAM_16)    { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_16)); }
  if (caps & FE_CAN_QAM_32)    { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_32)); }
  if (caps & FE_CAN_QAM_64)    { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_64)); }
  if (caps & FE_CAN_QAM_128)   { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_128)); }
  if (caps & FE_CAN_QAM_256)   { modulations.push_back(cDvbTransponderParams::TranslateModulation(QAM_256)); }
  if (caps & FE_CAN_8VSB)      { modulations.push_back(cDvbTransponderParams::TranslateModulation(VSB_8)); }
  if (caps & FE_CAN_16VSB)     { modulations.push_back(cDvbTransponderParams::TranslateModulation(VSB_16)); }
  if (caps & FE_CAN_TURBO_FEC) { modulations.push_back("TURBO_FEC"); }

  return modulations;
}

DeviceVector cDvbDevice::InitialiseDevices()
{
  //gSourceParams['A'] = cDvbSourceParams('A', "ATSC");
  //gSourceParams['C'] = cDvbSourceParams('C', "DVB-C");
  //gSourceParams['S'] = cDvbSourceParams('S', "DVB-S");
  //gSourceParams['T'] = cDvbSourceParams('T', "DVB-T");

  DeviceVector devices;

  // Enumerate /dev/dvb
  DirectoryListing items;
  if (CDirectory::GetDirectory(DEV_DVB_BASE, items))
  {
    for (DirectoryListing::const_iterator itemIt = items.begin(); itemIt != items.end(); ++itemIt)
    {
      string adapterName = itemIt->Name();

      // Adapter node must begin with "adapter"
      if (!StringUtils::StartsWith(adapterName, DEV_DVB_ADAPTER))
        continue;

      const int INVALID = -1;

      // Get adapter index from directory name (e.g. "adapter0")
      long adapter = StringUtils::IntVal(adapterName.substr(strlen(DEV_DVB_ADAPTER)), INVALID);
      if (adapter == INVALID)
        continue;

      // Enumerate /dev/dvb/adapterN
      DirectoryListing adapterDirItems;
      if (!CDirectory::GetDirectory(URLUtils::AddFileToFolder(DEV_DVB_BASE, adapterName), adapterDirItems))
        continue;

      for (DirectoryListing::const_iterator itemIt2 = adapterDirItems.begin(); itemIt2 != adapterDirItems.end(); ++itemIt2)
      {
        string frontendName = itemIt2->Name();
        // Frontend node must begin with "frontend"
        if (!StringUtils::StartsWith(frontendName, DEV_DVB_FRONTEND))
          continue;

        // Get frontend index from directory name (e.g. "frontend0")
        long frontend = StringUtils::IntVal(frontendName.substr(strlen(DEV_DVB_FRONTEND)), INVALID);
        if (frontend == INVALID)
          continue;

        shared_ptr<cDvbDevice> device = shared_ptr<cDvbDevice>(new cDvbDevice(adapter, frontend));
        if (device->DvbChannel()->GetTuner() && device->DvbChannel()->GetTuner()->IsValid())
          devices.push_back(static_pointer_cast<cDevice>(device));
      }
    }
  }

  return devices;
}

bool cDvbDevice::Exists(unsigned int adapter, unsigned int frontend)
{
  return CFile::Exists(DvbName(DEV_DVB_FRONTEND, adapter, frontend));
}

unsigned int cDvbDevice::GetSubsystemId() const
{
  static unsigned int subsystemId = 0;

  // Only attempt once
  static unsigned int attempts = 0;
  if (attempts++ == 0)
  {
    string dvbName = DvbName(DEV_DVB_FRONTEND, m_adapter, m_frontend);
    struct __stat64 st;
    if (CFile::Stat(dvbName, &st) == 0)
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
            break;
          }
          else if (file.Open(StringUtils::Format("/sys/class/dvb/%s/device/subsystem_vendor", name.c_str())) && file.ReadLine(line))
          {
            subsystemId = StringUtils::IntVal(line) << 16;
            break;
          }
        }
      }
    }
  }

  return subsystemId;
}

bool cDvbDevice::Ready()
{
  if (DvbCommonInterface()->m_ciAdapter)
    return DvbCommonInterface()->m_ciAdapter->Ready();
  return true;
}

string cDvbDevice::DeviceName() const
{
  return DvbChannel()->GetTuner()->Name();
}

string cDvbDevice::DeviceType() const
{
  if (DvbChannel()->GetTuner())
  {
    if (DvbChannel()->GetTuner()->FrontendType() != SYS_UNDEFINED)
      return DeliverySystemNames[DvbChannel()->GetTuner()->FrontendType()];
    if (!DvbChannel()->GetTuner()->m_deliverySystems.empty())
      return DeliverySystemNames[DvbChannel()->GetTuner()->m_deliverySystems[0]]; // to have some reasonable default
  }
  return "";
}

bool cDvbDevice::Bond(cDvbDevice *device)
{
  CLockObject lock(m_bondMutex);

  if (!m_bondedDevice)
  {
    if (device != this)
    {
      if ((DvbChannel()->ProvidesDeliverySystem(SYS_DVBS) || DvbChannel()->ProvidesDeliverySystem(SYS_DVBS2)) &&
          (device->DvbChannel()->ProvidesDeliverySystem(SYS_DVBS) || device->DvbChannel()->ProvidesDeliverySystem(SYS_DVBS2)))
      {
        if (DvbChannel()->GetTuner() && device->DvbChannel()->GetTuner() &&
            DvbChannel()->GetTuner()->Bond(device->DvbChannel()->GetTuner()))
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
  CLockObject lock(m_bondMutex);
  if (cDvbDevice *d = m_bondedDevice)
  {
    if (DvbChannel()->GetTuner())
      DvbChannel()->GetTuner()->UnBond();
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
  CLockObject lock(m_bondMutex);
  if (m_bondedDevice)
    return DvbChannel()->GetTuner() && DvbChannel()->GetTuner()->BondingOk(channel, bConsiderOccupied);
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

int cDvbDevice::DvbOpen(const char *name, int mode) const
{
  string dvbName = DvbName(name, m_adapter, m_frontend);

  int fd = open(dvbName.c_str(), mode);

  if (fd >= 0)
    isyslog("DVB: Opening %s", dvbName.c_str());
  else
    esyslog("DVB: Failed to open %s (%s)", dvbName.c_str(), strerror(errno));

  return fd;
}

cDvbChannelSubsystem *cDvbDevice::DvbChannel() const
{
  cDvbChannelSubsystem *channel = dynamic_cast<cDvbChannelSubsystem*>(Channel());
  assert(channel);
  return channel;
}

cDvbCommonInterfaceSubsystem *cDvbDevice::DvbCommonInterface() const
{
  cDvbCommonInterfaceSubsystem *commonInterface = dynamic_cast<cDvbCommonInterfaceSubsystem*>(CommonInterface());
  assert(commonInterface);
  return commonInterface;
}

cDvbPIDSubsystem *cDvbDevice::DvbPID() const
{
  cDvbPIDSubsystem *pid = dynamic_cast<cDvbPIDSubsystem*>(PID());
  assert(pid);
  return pid;
}

cDvbReceiverSubsystem *cDvbDevice::DvbReceiver() const
{
  cDvbReceiverSubsystem *receiver = dynamic_cast<cDvbReceiverSubsystem*>(Receiver());
  assert(receiver);
  return receiver;
}

cDvbSectionFilterSubsystem *cDvbDevice::DvbSectionFilter() const
{
  cDvbSectionFilterSubsystem *sectionFilter = dynamic_cast<cDvbSectionFilterSubsystem*>(SectionFilter());
  assert(sectionFilter);
  return sectionFilter;
}
