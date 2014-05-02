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

#include "SignalHandler.h"
#include "VDRDaemon.h"

#include "gtest/gtest.h"

#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace std;

// Make this longer than cVDRDaemon takes to clean up
#define MAX_WAIT_CLEANUP  2000 // ms

namespace VDR
{

TEST(VDRDaemonTest, Init)
{
  cVDRDaemon vdr;
  EXPECT_TRUE(vdr.Init());
  EXPECT_TRUE(vdr.IsRunning());
  EXPECT_FALSE(vdr.WaitForShutdown(10));
  vdr.Stop();
  EXPECT_TRUE(vdr.WaitForShutdown(MAX_WAIT_CLEANUP));
  EXPECT_FALSE(vdr.IsRunning());
  EXPECT_EQ(vdr.GetExitCode(), 0);
}

// This test kills Eclipse's debugger, uncomment and run tests from command line
/*
TEST(VDRDaemonTest, Signals)
{
  cVDRDaemon vdr;

  // We want to assert here so that our kill() signal doesn't end the test cases
  ASSERT_TRUE(vdr.Init());
  ASSERT_TRUE(vdr.IsRunning());

  CSignalHandler::Get().SetSignalReceiver(SIGINT, &vdr);

  // Simulate ctrl+C
  cout << "Sending SIGINT. If the process is killed, something went wrong" << endl;
  kill(getpid(), SIGINT);

  EXPECT_TRUE(vdr.WaitForShutdown(MAX_WAIT_CLEANUP));
  EXPECT_FALSE(vdr.IsRunning());
  EXPECT_EQ(vdr.GetExitCode(), SIGINT + EXIT_CODE_OFFSET);

  CSignalHandler::Get().ResetSignalReceivers();
}
*/

}
