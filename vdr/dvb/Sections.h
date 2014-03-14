/*
 * sections.h: Section data handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sections.h 2.0 2005/08/13 11:23:55 kls Exp $
 */

#ifndef __SECTIONS_H
#define __SECTIONS_H

#include "Types.h"
#include <time.h>
#include "Filter.h"
#include "channels/Channel.h"
#include "utils/Tools.h"
#include "utils/DateTime.h"
#include "platform/threads/threads.h"

class cDevice;
class cChannel;
class cFilterHandle;
class cSectionHandlerPrivate;

class cSectionHandler : public PLATFORM::CThread
{
  friend class cFilter;
private:
  cSectionHandlerPrivate* m_shp;
  cDevice*                m_device;
  int                     m_iStatusCount;
  bool                    m_bOn, m_bWaitForLock;
  CDateTime               m_lastIncompleteSection;
  cList<cFilter>          m_filters;
  cList<cFilterHandle>    m_filterHandles;
  PLATFORM::CMutex        m_mutex;

  void Add(const cFilterData *FilterData);
  void Del(const cFilterData *FilterData);
  virtual void* Process(void);
public:
  cSectionHandler(cDevice *Device);
  virtual ~cSectionHandler();
  int Source(void);
  int Transponder(void);
  ChannelPtr Channel(void);
  void Attach(cFilter *Filter);
  void Detach(cFilter *Filter);
  void SetChannel(ChannelPtr Channel);
  void SetStatus(bool On);
  };

#endif //__SECTIONS_H
