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
  //cDirectory::Create("special://home/config");

  // TODO: Implement protocols special:// (handled like XBMC) and vfs:// (or xbmc://)
  //       (which routes URI through VFS api calls)

  // Directories:
  // - special://home - Main writable directory, use FHS as a linux app (in which case it maps to e.g. ~/.vdr)
  //                    or uaddon_data/addon.id/ as an xbmc add-on (in which case it maps to
  //                    vfs://special%3A%2F%2Fprofile%2Faddon_data%2Faddon.id possibly)
  //
  // - special://vdr - URI of source directory (special://vdr/system is repo's /system folder)

//  cDvbDevice::Initialize();
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
}

bool cVDRDaemon::WaitForShutdown(uint32_t iTimeout /* = 0 */)
{
  if (IsRunning())
    return m_exitEvent.Wait(iTimeout);
  return true;
}
