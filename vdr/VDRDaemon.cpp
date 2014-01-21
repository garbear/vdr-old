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
#include "dvb/DiSEqC.h"
#include "epg/EPG.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "recordings/Recording.h"
#include "recordings/Timers.h"
#include "settings/Settings.h"
#include "utils/Shutdown.h"
#include "vnsi/Server.h"
#include "dvb/EITScan.h"

#include <signal.h> // or #include <bits/signum.h>

#define MANUALSTART          600 // seconds the next timer must be in the future to assume manual start
#define CHANNELSAVEDELTA     600 // seconds before saving channels.conf after automatic modifications
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

  CSpecialProtocol::LogPaths();

  // Create directories
  CDirectory::Create("special://home/epg/");
  CDirectory::Create("special://home/system/");
  CDirectory::Create("special://home/video/");

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
  cSchedules::SetEpgDataFileName("special://home/epg/epg.data");

  m_EpgDataReader = new cEpgDataReader;
  m_EpgDataReader->CreateThread();

  cDeviceManager::Get().Initialise();

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
    EITScanner.Process();

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
  cDeviceManager::Get().Shutdown();

  cChannelManager::Get().Clear();
}

bool cVDRDaemon::WaitForShutdown(uint32_t iTimeout /* = 0 */)
{
  if (IsRunning())
    return m_exitEvent.Wait(iTimeout);
  return true;
}
