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

#include "Server.h"
#include "Client.h"
#include "channels/ChannelFilter.h"
#include "channels/ChannelManager.h"
#include "epg/ScheduleManager.h"
#include "recordings/RecordingManager.h"
#include "settings/AllowedHosts.h"
#include "settings/Settings.h"
#include "timers/TimerManager.h"
#include "utils/log/Log.h"
#include "utils/Shutdown.h"
#include "utils/StringUtils.h"
#include "utils/Timer.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace VDR
{

cVNSIServer::cVNSIServer(int listenPort)
{
  m_ServerPort  = listenPort;
  m_IdCnt       = 0;

  //XXX disabled for now
//  VNSIChannelFilter.Load();
//  VNSIChannelFilter.SortChannels();

  m_bChannelsModified = false;
  m_bEventsModified = false;
  m_bTimersModified = false;
  m_bRecordingsModified = false;

  m_ServerFD = socket(AF_INET, SOCK_STREAM, 0);
  if(m_ServerFD == -1)
    return;

  fcntl(m_ServerFD, F_SETFD, fcntl(m_ServerFD, F_GETFD) | FD_CLOEXEC);

  int one = 1;
  setsockopt(m_ServerFD, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

  struct sockaddr_in s;
  memset(&s, 0, sizeof(s));
  s.sin_family = AF_INET;
  s.sin_port = htons(m_ServerPort);

  int x = bind(m_ServerFD, (struct sockaddr *)&s, sizeof(s));
  if (x < 0)
  {
    close(m_ServerFD);
    isyslog("Unable to start VNSI Server, port already in use ?");
    m_ServerFD = -1;
    return;
  }

  listen(m_ServerFD, 10);

  CreateThread();
}

cVNSIServer::~cVNSIServer()
{
  StopThread(-1);
  for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
  {
    delete (*i);
  }
  m_clients.erase(m_clients.begin(), m_clients.end());
  StopThread();
  isyslog("VNSI Server stopped");
}

void cVNSIServer::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageChannelChanged)
  {
    PLATFORM::CLockObject lock(m_mutex);
    m_bChannelsModified = true;
  }
  else if (msg == ObservableMessageEventChanged)
  {
    PLATFORM::CLockObject lock(m_mutex);
    m_bEventsModified = true;
  }
  else if (msg == ObservableMessageTimerChanged)
  {
    PLATFORM::CLockObject lock(m_mutex);
    m_bTimersModified = true;
  }
  else if (msg == ObservableMessageRecordingChanged)
  {
    PLATFORM::CLockObject lock(m_mutex);
    m_bRecordingsModified = true;
  }
}

void cVNSIServer::NewClientConnected(int fd)
{
  char buf[64];
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);

  if (getpeername(fd, (struct sockaddr *)&sin, &len))
  {
    esyslog("getpeername() failed, dropping new incoming connection %d", m_IdCnt);
    close(fd);
    return;
  }

  if (!CAllowedHosts::Get().Acceptable(sin.sin_addr.s_addr))
  {
    isyslog("Address not allowed to connect (%u)", sin.sin_addr.s_addr);
    close(fd);
    return;
  }

  if (fcntl(fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
  {
    esyslog("Error setting control socket to nonblocking mode");
    close(fd);
    return;
  }

  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));

  val = 30;
  setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val));

  val = 15;
  setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val));

  val = 5;
  setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val));

  val = 1;
  setsockopt(fd, SOL_TCP, TCP_NODELAY, &val, sizeof(val));

  isyslog("Client with ID %d connected: %s", m_IdCnt, cxSocket::ip2txt(sin.sin_addr.s_addr, sin.sin_port, buf));
  cVNSIClient *connection = new cVNSIClient(fd, m_IdCnt, cxSocket::ip2txt(sin.sin_addr.s_addr, sin.sin_port, buf));
  PLATFORM::CLockObject lock(m_mutex);
  m_clients.push_back(connection);
  m_IdCnt++;
}

void* cVNSIServer::Process(void)
{
  fd_set fds;
  struct timeval tv;

  cTimeMs chanTimer(0);
  cTimeMs epgTimer(0);
  cTimeMs timerTimer(0);
  cTimeMs recordingTimer(0);

  // delete old timeshift file
  std::string cmd;
  struct stat sb;
  std::string strTimeshiftBufferDir = cSettings::Get().m_TimeshiftBufferDir;
  if (!strTimeshiftBufferDir.empty() && stat(strTimeshiftBufferDir.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
  {
    if (strTimeshiftBufferDir.at(strTimeshiftBufferDir.length() - 1) == '/')
      cmd = StringUtils::Format("rm -f %s*.vnsi", strTimeshiftBufferDir.c_str());
    else
      cmd = StringUtils::Format("rm -f %s/*.vnsi", strTimeshiftBufferDir.c_str());
  }
  else
  {
    cmd = StringUtils::Format("rm -f %s/*.vnsi", cSettings::Get().m_VideoDirectory.c_str());
  }
  int ret = system(cmd.c_str());

  cChannelManager::Get().RegisterObserver(this);
  cScheduleManager::Get().RegisterObserver(this);
  cTimerManager::Get().RegisterObserver(this);
  cRecordingManager::Get().RegisterObserver(this);

  isyslog("VNSI Server started on port %d", m_ServerPort);
  isyslog("Channel streaming timeout: %i seconds", cSettings::Get().m_StreamTimeout);

  while (!IsStopped())
  {
    FD_ZERO(&fds);
    FD_SET(m_ServerFD, &fds);

    tv.tv_sec = 0;
    tv.tv_usec = 250*1000;

    int r = select(m_ServerFD + 1, &fds, NULL, NULL, &tv);
    if (r == -1)
    {
      //esyslog("failed during select");
      continue;
    }
    if (r == 0)
    {
      PLATFORM::CLockObject lock(m_mutex);

      // remove disconnected clients
      for (ClientList::iterator i = m_clients.begin(); i != m_clients.end();)
      {
        if (!(*i)->IsRunning())
        {
          isyslog("Client with ID %u seems to be disconnected, removing from client list", (*i)->GetID());
          delete (*i);
          i = m_clients.erase(i);
        }
        else {
          i++;
        }
      }

      // trigger clients to reload the modified channel list
      if(m_clients.size() > 0 && chanTimer.TimedOut())
      {
        if (m_bChannelsModified)
        {
          m_bChannelsModified = false;
          isyslog("Requesting clients to reload channel list");
          for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
            (*i)->ChannelChange();
        }
        chanTimer.Set(5000);
      }

      // reset inactivity timeout as long as there are clients connected
      if(m_clients.size() > 0) {
        ShutdownHandler.SetUserInactiveTimeout();
      }

      // update recordings
      if(m_clients.size() > 0 && recordingTimer.TimedOut())
      {
        if (m_bRecordingsModified)
        {
          m_bRecordingsModified = false;
          isyslog("Recordings state changed, requesting clients to reload recordings");
          for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
            (*i)->RecordingsChange();
        }
        recordingTimer.Set(5000);
      }

      // update timers
      if(m_clients.size() > 0 && timerTimer.TimedOut())
      {
        if (m_bTimersModified)
        {
          m_bTimersModified = false;
          isyslog("Timers state changed, requesting clients to reload timers");
          for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
            (*i)->TimerChange();
        }
        timerTimer.Set(5000);
      }

      // update epg
      if(m_clients.size() > 0 && epgTimer.TimedOut())
      {
        if (m_bEventsModified)
        {
          m_bEventsModified = false;
          isyslog("Schedule changed, requesting clients to reload events");
          for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
            (*i)->EpgChange();
        }
        epgTimer.Set(5000);
      }
      continue;
    }

    int fd = accept(m_ServerFD, 0, 0);
    if (fd >= 0)
    {
      NewClientConnected(fd);
    }
    else
    {
      esyslog("accept failed");
    }
  }

  cChannelManager::Get().UnregisterObserver(this);
  cScheduleManager::Get().UnregisterObserver(this);
  cTimerManager::Get().UnregisterObserver(this);
  cRecordingManager::Get().UnregisterObserver(this);

  return NULL;
}

}
