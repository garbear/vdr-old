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

#include "SignalHandler.h"
#include "VDRDaemon.h"
#include "settings/Settings.h"

#include <signal.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  cVDRDaemon vdr;

  if (!cSettings::Get().LoadFromCmdLine(argc, argv))
    return -2;

  if (vdr.Init())
  {
    CSignalHandler::Get().SetSignalReceiver(SIGHUP, &vdr);
    CSignalHandler::Get().SetSignalReceiver(SIGINT, &vdr);
    CSignalHandler::Get().SetSignalReceiver(SIGKILL, &vdr);
    CSignalHandler::Get().SetSignalReceiver(SIGTERM, &vdr);
    CSignalHandler::Get().IgnoreSignal(SIGPIPE);

    vdr.WaitForShutdown();
  }

  CSignalHandler::Get().ResetSignalReceivers();

  int signum = vdr.GetExitCode();
  if (signum == EXIT_CODE_OFFSET + SIGHUP ||
      signum == EXIT_CODE_OFFSET + SIGINT ||
      signum == EXIT_CODE_OFFSET + SIGKILL ||
      signum == EXIT_CODE_OFFSET + SIGTERM)
    kill(getpid(), signum - EXIT_CODE_OFFSET);

  return signum;
}
