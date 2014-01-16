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

#include "VDRDaemon.h"

#include "devices/DeviceManager.h"
#include "devices/linux/DVBDevice.h"
#include "dvb/DiSEqC.h"
#include "epg/EPG.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "recordings/Recording.h"
#include "recordings/Timers.h"
#include "settings/Settings.h"
#include "utils/Shutdown.h"
#include "vnsi/Server.h"

#include <signal.h> // or #include <bits/signum.h>

#define MANUALSTART          600 // seconds the next timer must be in the future to assume manual start
#define DEVICEREADYTIMEOUT    30 // seconds to wait until all devices are ready

using namespace PLATFORM;

cVDRDaemon::cVDRDaemon()
 : m_exitCode(0),
   m_server(NULL),
   m_EpgDataReader(NULL)
{
}

cVDRDaemon::~cVDRDaemon()
{
  Stop();
  WaitForShutdown();
  delete m_server;
  m_server = NULL;
  delete m_EpgDataReader;
  m_EpgDataReader = NULL;
}

bool cVDRDaemon::Init()
{
  if (!CSpecialProtocol::SetFileBasePath())
    return false;

  // Create directories
  CDirectory::Create("special://home/system/");

  // TODO: Implement protocols special:// (handled like XBMC)

  // Directories:
  // - special://home - Main writable directory, use FHS as a linux app (in which case it maps to e.g. ~/.vdr)
  //                    or special://profile/addon_data/addon.id as an xbmc add-on (note: this is routed through
  //                    XBMC's VFS api)
  //
  // - special://xbmchome - Routed through XBMC's VFS api as special://home
  //
  // - special://vdr - URI of source directory (special://vdr/system is repo's /system folder)
  //
  // - special://*** (everything else) - routed through VFS api calls

  if (!Setup.Load("special://home/system/setup.conf"))
    Setup.Load("special://vdr/system/setup.conf");

//  if (!Sources.Load("special://home/system/sources.conf"))
//    Sources.Load("special://vdr/system/sources.conf");

  if (!Diseqcs.Load("special://home/system/diseqc.conf"))
    Diseqcs.Load("special://vdr/system/diseqc.conf");

  if (!Scrs.Load("special://home/system/scr.conf"))
    Scrs.Load("special://vdr/system/scr.conf");

  if (!cChannelManager::Get().LoadConf("special://home/system/channels.conf"))
    cChannelManager::Get().LoadConf("special://vdr/system/channels.conf");

  if (!Timers.Load("special://home/system/timers.conf"))
    Timers.Load("special://vdr/system/timers.conf");

  if (!Commands.Load("special://home/system/commands.conf"))
    Commands.Load("special://vdr/system/commands.conf");

  if (!RecordingCommands.Load("special://home/system/reccmds.conf"))
    RecordingCommands.Load("special://vdr/system/reccmds.conf");

  if (!SVDRPhosts.Load("special://home/system/svdrphosts.conf"))
    SVDRPhosts.Load("special://vdr/system/svdrphosts.conf");

  if (!Folders.Load("special://home/system/folders.conf"))
    Folders.Load("special://vdr/system/folders.conf");

  // Recordings:
  Recordings.Update();
  DeletedRecordings.Update();

  // EPG data:
  if (!cSettings::Get().m_EpgDataFileName.empty())
  {
    std::string EpgDirectory;
    if (CDirectory::CanWrite(cSettings::Get().m_EpgDataFileName))
    {
      EpgDirectory = cSettings::Get().m_EpgDataFileName;
      cSettings::Get().m_EpgDataFileName = DEFAULTEPGDATAFILENAME;
    }
    else if (cSettings::Get().m_EpgDataFileName.at(0) != '/' && strcmp(".", cSettings::Get().m_EpgDataFileName.c_str()))
    {
      EpgDirectory = cSettings::Get().m_CacheDirectory;
    }

    if (!EpgDirectory.empty())
    {
      std::string EpgFile = EpgDirectory;
      if (strcmp(EpgFile.substr(EpgFile.length() - 1, 1).c_str(), "/"))
        EpgFile.append("/");
      EpgFile.append(cSettings::Get().m_EpgDataFileName.c_str());
      cSchedules::SetEpgDataFileName(EpgFile.c_str());
    }
    else
    {
      cSchedules::SetEpgDataFileName(cSettings::Get().m_EpgDataFileName.c_str());
    }

    m_EpgDataReader = new cEpgDataReader;
    m_EpgDataReader->Start();
  }

  cDvbDevice::InitialiseDevices();
  cDvbDevice::BondDevices(Setup.DeviceBondings);

  m_server = new cVNSIServer(cSettings::Get().m_ListenPort);

  // Check for timers in automatic start time window:
  ShutdownHandler.CheckManualStart(MANUALSTART);

  // Channel:
  if (!cDeviceManager::Get().WaitForAllDevicesReady(DEVICEREADYTIMEOUT))
    dsyslog("some devices are not ready after %d seconds", DEVICEREADYTIMEOUT);

  return CreateThread(true);
}

void cVDRDaemon::Stop()
{
  StopThread(false);
  m_sleepEvent.Signal();
}

void *cVDRDaemon::Process()
{
  while (!IsStopped())
  {
    m_sleepEvent.Wait(100);
  }
  Cleanup();
  m_exitEvent.Broadcast();
  return NULL;
}

void cVDRDaemon::OnSignal(int signum)
{
  switch (signum)
  {
  case SIGHUP:
  case SIGKILL:
  case SIGINT:
  case SIGTERM:
    m_exitCode = EXIT_CODE_OFFSET + signum;
    Stop();
    break;
  default:
    break;
  }
}

void cVDRDaemon::Cleanup()
{
  cChannelManager::Get().Clear();
}

bool cVDRDaemon::WaitForShutdown(uint32_t iTimeout /* = 0 */)
{
  if (IsRunning())
    return m_exitEvent.Wait(iTimeout);
  return true;
}
