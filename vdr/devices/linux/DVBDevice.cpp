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
#include "devices/subsystems/DeviceScanSubsystem.h"
#include "devices/subsystems/DeviceSPUSubsystem.h"
#include "devices/subsystems/DeviceTrackSubsystem.h"
#include "devices/subsystems/DeviceVideoFormatSubsystem.h"
#include "filesystem/Directory.h"
#include "filesystem/ReadDir.h"
#include "utils/CommonMacros.h" // for ARRAY_SIZE()
#include "utils/I18N.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
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

namespace VDR
{

#define MAXFRONTENDCMDS 16

#define INVALID_FD  -1

cDvbDevice::cDvbDevice(unsigned int adapter, unsigned int frontend)
 : cDevice(CreateSubsystems(this)),
   m_adapter(adapter),
   m_frontend(frontend),
   m_dvbTuner(this),
   m_fd_ca(INVALID_FD)
{
}

cSubsystems cDvbDevice::CreateSubsystems(cDvbDevice* device)
{
  cSubsystems subsystems = { };

  subsystems.Channel         = new cDvbChannelSubsystem(device);
  subsystems.CommonInterface = new cDvbCommonInterfaceSubsystem(device);
  subsystems.PID             = new cDvbPIDSubsystem(device);
  subsystems.Receiver        = new cDvbReceiverSubsystem(device);
  subsystems.SectionFilter   = new cDvbSectionFilterSubsystem(device);

  subsystems.ImageGrab       = new cDeviceImageGrabSubsystem(device);
  subsystems.Player          = new cDevicePlayerSubsystem(device);
  subsystems.Scan            = new cDeviceScanSubsystem(device);
  subsystems.SPU             = new cDeviceSPUSubsystem(device);
  subsystems.Track           = new cDeviceTrackSubsystem(device);
  subsystems.VideoFormat     = new cDeviceVideoFormatSubsystem(device);

  return subsystems;
}

cDvbDevice::~cDvbDevice(void)
{
  SectionFilter()->StopSectionHandler();

  // We're not explicitly closing any device files here, since this sometimes
  // caused segfaults. Besides, the program is about to terminate anyway...
}

string cDvbDevice::DvbPath(const char *dvbDeviceName) const
{
#if defined(TARGET_ANDROID)
  return StringUtils::Format("%s/%s%d.%s%d", DEV_DVB_BASE, DEV_DVB_ADAPTER, m_adapter, dvbDeviceName, m_frontend);
#else
  return StringUtils::Format("%s/%s%d/%s%d", DEV_DVB_BASE, DEV_DVB_ADAPTER, m_adapter, dvbDeviceName, m_frontend);
#endif
}

string cDvbDevice::ID() const
{
  return DvbPath(DEV_DVB_FRONTEND);
}

string cDvbDevice::Name() const
{
  return m_dvbTuner.Name();
}

DeviceVector cDvbDevice::FindDevicesMdev(void)
{
  DeviceVector devices;

  // Enumerate /dev
  DirectoryListing items;
  if (CDirectory::GetDirectory(DEV_DVB_BASE, items, "", DEFAULTS, true))
  {
    for (DirectoryListing::const_iterator itemIt = items.begin(); itemIt != items.end(); ++itemIt)
    {
      string adapterName = itemIt->Name();

      // Adapter node must begin with "dvb"
      if (!StringUtils::StartsWith(adapterName, DEV_DVB_ADAPTER))
        continue;

      const int INVALID = -1;

      vector<string> splitString;
      StringUtils::Split(adapterName, ".", splitString);
      if (splitString.size() != 2)
        continue;

      // Get adapter index from directory name (e.g. "dvb0")
      long adapter = StringUtils::IntVal(splitString[0].substr(strlen(DEV_DVB_ADAPTER)), INVALID);
      if (adapter == INVALID)
        continue;

      // Frontend node must begin with "frontend"
      if (!StringUtils::StartsWith(splitString[1], DEV_DVB_FRONTEND))
        continue;

      // Get frontend index from directory name (e.g. "frontend0")
      long frontend = StringUtils::IntVal(splitString[1].substr(strlen(DEV_DVB_FRONTEND)), INVALID);
      if (frontend == INVALID)
        continue;

      devices.push_back(DevicePtr(new cDvbDevice(adapter, frontend)));
    }
  }

  return devices;
}

DeviceVector cDvbDevice::FindDevices()
{
#if defined(TARGET_ANDROID)
  return FindDevicesMdev();
#endif

  DeviceVector devices;

  // Enumerate /dev/dvb
  DirectoryListing items;
  if (CDirectory::GetDirectory(DEV_DVB_BASE, items, "", DEFAULTS, true))
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
      if (!CDirectory::GetDirectory(URLUtils::AddFileToFolder(DEV_DVB_BASE, adapterName), adapterDirItems, "", DEFAULTS, true))
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

        devices.push_back(DevicePtr(new cDvbDevice(adapter, frontend)));
      }
    }
  }

  return devices;
}

bool cDvbDevice::Initialise(unsigned int index)
{
  if (!cDevice::Initialise(index))
    return false;

  if (!m_dvbTuner.Open())
    return false;

  // Common Interface:
  m_fd_ca = open(DvbPath(DEV_DVB_CA).c_str(), O_RDWR);
  if (m_fd_ca >= 0)
    DvbCommonInterface()->m_ciAdapter = cDvbCiAdapter::CreateCiAdapter(this, m_fd_ca);

  SectionFilter()->StartSectionHandler();

  if (!Ready())
  {
    dsyslog("Device %u: waiting for the CA slot to initialise", Index());
    return false;
  }

  return true;
}

void cDvbDevice::Notify(const Observable &obs, const ObservableMessage msg)
{
  //XXX implement "CA ready"
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

}
