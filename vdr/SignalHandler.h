/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Contains code Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include <map>
#include <signal.h>

class ISignalReceiver
{
public:
  virtual ~ISignalReceiver() { }

  /*!
   * \brief Called when a signal registered via CSignalHandler::SetSignalReceiver()
   *        is fired
   * \param signum The signal, defined in signal.h
   */
  virtual void OnSignal(int signum) = 0;
};

class CSignalHandler
{
public:
  /*!
   * \brief Construction is protected by singleton pattern, as only 1 signal
   *        CSignalHandler can be active at a time
   */
  static CSignalHandler &Get();

  /*!
   * \brief Signal handlers are reset on destruction
   */
  ~CSignalHandler() { ResetSignalReceivers(); }

  /*!
   * \brief Install the signal handler and set the signal callback class that
   *        will receive the signal
   * \param signum The signal, defined in signal.h
   * \param callback The class whose OnSignal() function will be invoked
   */
  void SetSignalReceiver(int signum, ISignalReceiver *callback);

  /*!
   * \brief Uninstall the signal handler and reset the signal callback class for
   *        that signal
   * \param signum The signal, defined in signal.h
   */
  void ResetSignalReceiver(int signum);

  /*!
   * \brief Uninstall all signal handers and reset the signal callback classes
   */
  void ResetSignalReceivers();

  /*!
   * \brief  Dispatch the specified signal to the appropriate signal handler.
   *         Invoked by fired UNIX signals, or directly by the user to avoid
   *         the use of UNIX signals altogether.
   * \param signum The signal, defined in signal.h
   */
  static void Alarm(int signum);

private:
  /*!
   * \brief Private, as all interaction is done through Get()
   */
  CSignalHandler() { }

  /*!
   * \brief Helper function called by Alarm()
   */
  void OnSignal(int signum);

  std::map<int, ISignalReceiver*> m_callbacks;
};
