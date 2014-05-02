/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2007 Chris Tallon
 *      Portions Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Portions Copyright (C) 2010, 2011 Alexander Pipelka
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "lib/platform/threads/threads.h"
#include "utils/Observer.h"

#include <list>

namespace VDR
{

class cVNSIClient;

class cVNSIServer : public PLATFORM::CThread, public Observer
{
public:
  cVNSIServer(int listenPort);
  virtual ~cVNSIServer(void);

  void Notify(const Observable &obs, const ObservableMessage msg);
protected:
  typedef std::list<cVNSIClient*> ClientList;

  void* Process(void);
  void NewClientConnected(int fd);

  int              m_ServerPort;
  int              m_ServerFD;
  ClientList       m_clients;
  unsigned int     m_IdCnt;
  bool             m_bChannelsModified;
  PLATFORM::CMutex m_mutex;
};

}
