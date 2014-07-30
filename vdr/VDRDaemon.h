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
#pragma once

#include "SignalHandler.h"

#include "lib/platform/threads/mutex.h"
#include "lib/platform/threads/threads.h"

namespace VDR
{

class cVNSIServer;

class cVDRDaemon : protected PLATFORM::CThread, public ISignalReceiver
{
public:
  /*!
   * \brief Create a new, unitialized daemon
   */
  cVDRDaemon();

  /*!
   * \brief Destruct the daemon. Blocks until daemon has finished cleaning up
   */
  virtual ~cVDRDaemon();

  /*!
   * \brief Load/save the configuration
   */
  bool LoadConfig(void);
  bool SaveConfig(void);

  /*!
   * \brief Initialize and start the daemon
   */
  bool Init();

  /*!
   * \brief Stop the daemon and perform any clean-up related tasks
   */
  void Stop();

  bool IsRunning() { return CThread::IsRunning(); }

  /*!
   * \brief Block until the daemon has finished cleaning up
   * \param iTimeout Maximum wait time in ms, or 0 for infinite
   */
  bool WaitForShutdown(uint32_t iTimeout = 0);

  /*!
   * \brief Get the exit code (to be returned by main())
   */
  int GetExitCode() { return m_exitCode; }

protected:
  void *Process();

  /*!
   * \brief Inherited from ISignalReceiver
   */
  virtual void OnSignal(int signum);

private:
  /*!
   * \brief Perform all clean-up tasks that should be executed upon finish
   */
  void DeInit();

  /*!
   * Two events are used for thread synchronisation: a sleep event and an exit
   * event. The sleep event allows VDR to awaken from its event loop immediately
   * when stopped via Stop(). The exit event is triggered when the event loop
   * terminates, which allows users to block until WaitForShutdown() returns.
   */
  ::PLATFORM::CEvent m_sleepEvent;
  ::PLATFORM::CEvent m_exitEvent;

  int              m_exitCode;
  cVNSIServer*     m_server;
  bool             m_bConfigLoaded;
};

}
