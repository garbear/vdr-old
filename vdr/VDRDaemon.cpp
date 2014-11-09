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

#include "VDRDaemon.h"

#include "channels/ChannelManager.h"
#include "devices/DeviceManager.h"
#include "dvb/DiSEqC.h"
#include "epg/ScheduleManager.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "recordings/RecordingManager.h"
#include "settings/AllowedHosts.h"
#include "settings/Settings.h"
#include "timers/TimerManager.h"
#include "utils/log/Log.h"
#include "utils/Shutdown.h"
#include "vnsi/Server.h"

#include <signal.h> // or #include <bits/signum.h>

namespace VDR
{

#define MANUALSTART          600 // seconds the next timer must be in the future to assume manual start
#define CHANNELSAVEDELTA     600 // seconds before saving channels.conf after automatic modifications
#define DEVICEREADYTIMEOUT    30 // seconds to wait until all devices are ready

using namespace PLATFORM;

cVDRDaemon::cVDRDaemon()
 : m_exitCode(0),
   m_server(NULL),
   m_bConfigLoaded(false)
{
}

cVDRDaemon::~cVDRDaemon()
{
  Stop();
  WaitForShutdown();
}

bool cVDRDaemon::LoadConfig(void)
{
  if (m_bConfigLoaded)
    return true;

  m_bConfigLoaded = true;

  if (!CSpecialProtocol::SetFileBasePath())
    return false;

  CSpecialProtocol::LogPaths();

  // Create directories
  CDirectory::Create("special://home/");
  CDirectory::Create("special://home/epg/");
  CDirectory::Create("special://home/system/");
  CDirectory::Create("special://home/video/");

  cSettings::Get().Load();
  cChannelManager::Get().Load();
  cScheduleManager::Get().Load();
  cRecordingManager::Get().Load();
  CAllowedHosts::Get().Load();

//  if (!Diseqcs.Load("special://home/system/diseqc.conf"))
//    Diseqcs.Load("special://vdr/system/diseqc.conf");

//  if (!Scrs.Load("special://home/system/scr.conf"))
//    Scrs.Load("special://vdr/system/scr.conf");

  return true;
}

bool cVDRDaemon::SaveConfig(void)
{
  cRecordingManager::Get().Save();

  return true;
}

bool cVDRDaemon::Init()
{
  if (!LoadConfig())
    return false;

  if (cDeviceManager::Get().Initialise() == 0)
  {
    esyslog("no devices detected, exiting");
    return false;
  }

  m_server = new cVNSIServer(cSettings::Get().m_ListenPort);

  // Check for timers in automatic start time window:
  ShutdownHandler.CheckManualStart(MANUALSTART);

  // Channel:
  if (!cDeviceManager::Get().WaitForAllDevicesReady(DEVICEREADYTIMEOUT))
    dsyslog("some devices are not ready after %d seconds", DEVICEREADYTIMEOUT);

  cTimerManager::Get().Start();

  return CreateThread(true);
}

void cVDRDaemon::Stop()
{
  StopThread(-1);
  m_sleepEvent.Signal();
}

void *cVDRDaemon::Process()
{
  isyslog("VDR version %s started", VDRVERSION);
  while (!IsStopped())
  {
    m_sleepEvent.Wait();
  }
  DeInit();
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

void cVDRDaemon::DeInit()
{
  cTimerManager::Get().Stop();

  cDeviceManager::Get().Shutdown();

  cChannelManager::Get().Clear();

  SAFE_DELETE(m_server);

  SaveConfig();
}

bool cVDRDaemon::WaitForShutdown(uint32_t iTimeout /* = 0 */)
{
  if (IsRunning())
    return m_exitEvent.Wait(iTimeout);
  return true;
}

}
