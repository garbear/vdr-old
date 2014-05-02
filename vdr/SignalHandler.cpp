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

#include <signal.h>
#include <stddef.h>

namespace VDR
{

CSignalHandler &CSignalHandler::Get()
{
  static CSignalHandler instance;
  return instance;
}

void CSignalHandler::SetSignalReceiver(int signum, ISignalReceiver *callback)
{
  if (signal(signum,  CSignalHandler::Alarm) == SIG_IGN)
    signal(signum,  SIG_IGN);
  else
    m_callbacks[signum] = callback;
}

void CSignalHandler::IgnoreSignal(int signum)
{
  ResetSignalReceiver(signum);
  signal(signum, SIG_IGN);
}

void CSignalHandler::ResetSignalReceiver(int signum)
{
  std::map<int, ISignalReceiver*>::iterator it = m_callbacks.find(signum);
  if (it != m_callbacks.end())
  {
    signal(signum, SIG_DFL);
    m_callbacks.erase(it);
  }
}

void CSignalHandler::ResetSignalReceivers()
{
  for (std::map<int, ISignalReceiver*>::iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it)
    signal(it->first, SIG_DFL);
  m_callbacks.clear();
}

void CSignalHandler::Alarm(int signum)
{
  CSignalHandler::Get().OnSignal(signum);
  signal(signum, CSignalHandler::Alarm);
}

void CSignalHandler::OnSignal(int signum)
{
  std::map<int, ISignalReceiver*>::iterator it = m_callbacks.find(signum);
  if (it != m_callbacks.end())
    it->second->OnSignal(signum);
}

}
