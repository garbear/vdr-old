/*
 * eitscan.h: EIT scanner
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: eitscan.h 2.1 2012/03/07 13:54:16 kls Exp $
 */

#ifndef __EITSCAN_H
#define __EITSCAN_H

#include <time.h>
#include "Config.h"
#include "devices/Device.h"

class cChannel;
class cChannelManager;
class cTransponderList;

class cScanData : public cListObject {
private:
  cChannel* m_channel;
public:
  cScanData(const cChannel* channel);
  virtual ~cScanData(void);

  virtual int Compare(const cListObject &ListObject) const;
  int Source(void) const;
  int Transponder(void) const;
  const cChannel *GetChannel(void) const { return m_channel; }
};

class cScanList : public cList<cScanData>
{
public:
  void AddTransponders(const cChannelManager& Channels);
  void AddTransponders(const cTransponderList& Channels);
  void AddTransponder(const cChannel *Channel);
};

class cTransponderList;

class cEITScanner {
private:
  enum { ActivityTimeout = 60,
         ScanTimeout = 20
       };
  time_t lastScan, lastActivity;
  int currentChannel;
  cScanList *scanList;
  cTransponderList *transponderList;
public:
  cEITScanner(void);
  ~cEITScanner();
  bool Active(void) { return lastActivity == 0; }
  void AddTransponder(cChannel *Channel);
  void ForceScan(void);
  void Activity(void);
  void Process(void);
  };

extern cEITScanner EITScanner;

#endif //__EITSCAN_H
