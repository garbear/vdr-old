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
#include "devices/linux/DVBDevice.h"
#include "filesystem/Directory.h"

#include <signal.h> // or #include <bits/signum.h>

using namespace PLATFORM;

cVDRDaemon::cVDRDaemon()
 : m_exitCode(0)
{
}

cVDRDaemon::~cVDRDaemon()
{
  Stop();
  WaitForShutdown();
}

bool cVDRDaemon::Init()
{
  // Create directories
  cDirectory::Create("special://home/system/");

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

  if (!m_channelManager.LoadConf("special://home/system/channels.conf"))
    m_channelManager.LoadConf("special://vdr/system/channels.conf");

  cDvbDevice::Initialize();
//  cDvbDevice::BondDevices(Setup.DeviceBondings);

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
  m_channelManager.Clear();
}

bool cVDRDaemon::WaitForShutdown(uint32_t iTimeout /* = 0 */)
{
  if (IsRunning())
    return m_exitEvent.Wait(iTimeout);
  return true;
}
