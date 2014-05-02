/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2003-2006 Petri Hintukainen
 *      Portions Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Portions Copyright (C) 2011 Alexander Pipelka
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

/*
 * Socket wrapper classes
 *
 * Code is taken from xineliboutput plugin.
 *
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "Tools.h"
#include "XSocket.h"
#include "filesystem/Poller.h"
#include "utils/log/Log.h"

#include <errno.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace PLATFORM;

namespace VDR
{

cxSocket::~cxSocket()
{
  close();
  delete m_pollerRead;
  delete m_pollerWrite;
}

void cxSocket::close() {
  CLockObject lock(m_MutexWrite);
  if(m_fd >= 0) { 
    ::close(m_fd);
    m_fd=-1;
    delete m_pollerRead;
    m_pollerRead = NULL;
    delete m_pollerWrite;
    m_pollerWrite = NULL;
  }
}

void cxSocket::Shutdown()
{
  CLockObject lock(m_MutexWrite);
  if(m_fd >= 0)
  {
    ::shutdown(m_fd, SHUT_RD);
  }
}

void cxSocket::LockWrite()
{
  m_MutexWrite.Lock();
}

void cxSocket::UnlockWrite()
{
  m_MutexWrite.Unlock();
}

ssize_t cxSocket::write(const void *buffer, size_t size, int timeout_ms, bool more_data)
{
  CLockObject lock(m_MutexWrite);

  if(m_fd == -1)
    return -1;

  ssize_t written = (ssize_t)size;
  const unsigned char *ptr = (const unsigned char *)buffer;

  while (size > 0)
  {
    if(!m_pollerWrite->Poll(timeout_ms))
    {
      esyslog("cxSocket::write: poll() failed");
      return written-size;
    }

    ssize_t p = ::send(m_fd, ptr, size, (more_data ? MSG_MORE : 0));

    if (p <= 0)
    {
      if (errno == EINTR || errno == EAGAIN)
      {
        dsyslog("cxSocket::write: EINTR during write(), retrying");
        continue;
      }
      else if (errno != EPIPE)
        esyslog("cxSocket::write: write() error");
      return p;
    }

    ptr  += p;
    size -= p;
  }

  return written;
}

ssize_t cxSocket::read(void *buffer, size_t size, int timeout_ms)
{
  int retryCounter = 0;

  if(m_fd == -1)
    return -1;

  ssize_t missing = (ssize_t)size;
  unsigned char *ptr = (unsigned char *)buffer;

  while (missing > 0)
  {
    if(!m_pollerRead->Poll(timeout_ms))
    {
      esyslog("cxSocket::read: poll() failed at %d/%d", (int)(size-missing), (int)size);
      return size-missing;
    }

    ssize_t p = ::read(m_fd, ptr, missing);

    if (p < 0)
    {
      if (retryCounter < 10 && (errno == EINTR || errno == EAGAIN))
      {
        dsyslog("cxSocket::read: EINTR/EAGAIN during read(), retrying");
        retryCounter++;
        continue;
      }
      esyslog("cxSocket::read: read() error at %d/%d", (int)(size-missing), (int)size);
      return 0;
    }
    else if (p == 0)
    {
      isyslog("cxSocket::read: eof, connection closed");
      close();
      return 0;
    }

    retryCounter = 0;
    ptr  += p;
    missing -= p;
  }

  return size;
}

void cxSocket::SetHandle(int h) {
  CLockObject lock(m_MutexWrite);
  if(h != m_fd) {
    close();

    m_fd          = h;
    m_pollerRead  = new cPoller(m_fd);
    m_pollerWrite = new cPoller(m_fd, true);
  }
}

char *cxSocket::ip2txt(uint32_t ip, unsigned int port, char *str)
{
  // inet_ntoa is not thread-safe (?)
  if(str) {
    unsigned int iph =(unsigned int)ntohl(ip);
    unsigned int porth =(unsigned int)ntohs(port);
    if(!porth)
      sprintf(str, "%d.%d.%d.%d",
	      ((iph>>24)&0xff), ((iph>>16)&0xff),
	      ((iph>>8)&0xff), ((iph)&0xff));
    else
      sprintf(str, "%u.%u.%u.%u:%u",
	      ((iph>>24)&0xff), ((iph>>16)&0xff),
	      ((iph>>8)&0xff), ((iph)&0xff),
	      porth);
  }
  return str;
}

}
