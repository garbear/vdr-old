/*
 * sections.c: Section data handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sections.c 2.2 2012/10/04 12:21:59 kls Exp $
 */

#include "Sections.h"
#include <unistd.h>
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceSectionFilterSubsystem.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"

using namespace PLATFORM;

// --- cFilterHandle----------------------------------------------------------

class cFilterHandle : public cListObject {
public:
  cFilterData filterData;
  int handle;
  int used;
  cFilterHandle(const cFilterData &FilterData);
  };

cFilterHandle::cFilterHandle(const cFilterData &FilterData)
{
  filterData = FilterData;
  handle = -1;
  used = 0;
}

// --- cSectionHandlerPrivate ------------------------------------------------

class cSectionHandlerPrivate
{
public:
  cChannel channel;
};

// --- cSectionHandler -------------------------------------------------------

cSectionHandler::cSectionHandler(cDevice *Device)
{
  m_shp = new cSectionHandlerPrivate;
  m_device = Device;
  m_iStatusCount = 0;
  m_bOn = false;
  m_bWaitForLock = false;
  m_lastIncompleteSection = 0;
  CreateThread();
}

cSectionHandler::~cSectionHandler()
{
  StopThread(3000);
  cFilter *fi;
  while ((fi = m_filters.First()) != NULL)
    Detach(fi);
  delete m_shp;
}

int cSectionHandler::Source(void)
{
  return m_shp->channel.Source();
}

int cSectionHandler::Transponder(void)
{
  return m_shp->channel.Transponder();
}

const cChannel *cSectionHandler::Channel(void)
{
  return &m_shp->channel;
}

void
cSectionHandler::Add(const cFilterData *FilterData)
{
  CLockObject lock(m_mutex);

  m_iStatusCount++;
  cFilterHandle *fh;
  for (fh = m_filterHandles.First(); fh; fh = m_filterHandles.Next(fh))
  {
    if (fh->filterData.Is(FilterData->pid, FilterData->tid, FilterData->mask))
      break;
  }
  if (!fh)
  {
    int handle = m_device->SectionFilter()->OpenFilter(FilterData->pid, FilterData->tid, FilterData->mask);
    if (handle >= 0)
    {
      fh = new cFilterHandle(*FilterData);
      fh->handle = handle;
      m_filterHandles.Add(fh);
    }
  }
  if (fh)
    fh->used++;
}

void
cSectionHandler::Del(const cFilterData *FilterData)
{
  CLockObject lock(m_mutex);

  m_iStatusCount++;
  cFilterHandle *fh;
  for (fh = m_filterHandles.First(); fh; fh = m_filterHandles.Next(fh))
  {
    if (fh->filterData.Is(FilterData->pid, FilterData->tid, FilterData->mask))
    {
      if (--fh->used <= 0)
      {
        m_device->SectionFilter()->CloseFilter(fh->handle);
        m_filterHandles.Del(fh);
        break;
      }
    }
  }
}

void
cSectionHandler::Attach(cFilter *Filter)
{
  CLockObject lock(m_mutex);

  m_iStatusCount++;
  m_filters.Add(Filter);
  Filter->sectionHandler = this;
  if (m_bOn)
    Filter->SetStatus(true);
}

void cSectionHandler::Detach(cFilter *Filter)
{
  CLockObject lock(m_mutex);

  m_iStatusCount++;
  Filter->SetStatus(false);
  Filter->sectionHandler = NULL;
  m_filters.Del(Filter, false);
}

void cSectionHandler::SetChannel(const cChannel *Channel)
{
  CLockObject lock(m_mutex);
  m_shp->channel = Channel ? *Channel : cChannel();
}

void
cSectionHandler::SetStatus(bool On)
{
  CLockObject lock(m_mutex);
  if (m_bOn != On)
  {
    if (!On || m_device->Channel()->HasLock())
    {
      m_iStatusCount++;
      for (cFilter *fi = m_filters.First(); fi; fi = m_filters.Next(fi))
      {
        fi->SetStatus(false);
        if (On)
          fi->SetStatus(true);
      }
      m_bOn = On;
      m_bWaitForLock = false;
    }
    else
      m_bWaitForLock = On;
  }
}

void*
cSectionHandler::Process(void)
{
  while (!IsStopped())
  {
    CLockObject lock(m_mutex);

    if (m_bWaitForLock)
      SetStatus(true);
    int NumFilters = m_filterHandles.Count();
    pollfd pfd[NumFilters];
    for (cFilterHandle *fh = m_filterHandles.First(); fh; fh = m_filterHandles.Next(fh))
    {
      int i = fh->Index();
      pfd[i].fd = fh->handle;
      pfd[i].events = POLLIN;
      pfd[i].revents = 0;
    }
    int oldStatusCount = m_iStatusCount;
    lock.Unlock();

    if (poll(pfd, NumFilters, 1000) > 0)
    {
      bool DeviceHasLock = m_device->Channel()->HasLock();
      if (!DeviceHasLock)
        cCondWait::SleepMs(100);
      for (int i = 0; i < NumFilters; i++)
      {
        if (pfd[i].revents & POLLIN)
        {
          cFilterHandle *fh = NULL;
          lock.Lock();
          if (m_iStatusCount != oldStatusCount)
            break;
          for (fh = m_filterHandles.First(); fh; fh = m_filterHandles.Next(fh))
          {
            if (pfd[i].fd == fh->handle)
              break;
          }
          if (fh)
          {
            // Read section data:
            unsigned char buf[4096]; // max. allowed size for any EIT section
            int r = m_device->SectionFilter()->ReadFilter(fh->handle, buf,
                sizeof(buf));
            if (!DeviceHasLock)
              continue; // we do the read anyway, to flush any data that might have come from a different transponder
            if (r > 3)
            { // minimum number of bytes necessary to get section length
              int len = (((buf[1] & 0x0F) << 8) | (buf[2] & 0xFF)) + 3;
              if (len == r)
              {
                // Distribute data to all attached filters:
                int pid = fh->filterData.pid;
                int tid = buf[0];
                for (cFilter *fi = m_filters.First(); fi; fi = m_filters.Next(fi))
                {
                  if (fi->Matches(pid, tid))
                    fi->Process(pid, tid, buf, len);
                }
              }
              else if (time(NULL) - m_lastIncompleteSection > 10)
              { // log them only every 10 seconds
                dsyslog("read incomplete section - len = %d, r = %d", len, r);
                m_lastIncompleteSection = time(NULL);
              }
            }
          }
        }
      }
    }
  }

  return NULL;
}
