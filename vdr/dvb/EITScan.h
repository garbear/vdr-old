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
#include "channels/Channel.h"

class cChannelManager;
class cTransponderList;

class cScanData : public cListObject {
private:
  ChannelPtr m_channel;
public:
  cScanData(ChannelPtr channel);
  virtual ~cScanData(void);

  virtual int Compare(const cListObject &ListObject) const;
  int Source(void) const;
  int Transponder(void) const;
  ChannelPtr GetChannel(void) const { return m_channel; }
};

class cScanList : public cList<cScanData>
{
public:
  void AddTransponders(const cChannelManager& Channels);
  void AddTransponders(const cTransponderList& Channels);
  void AddTransponder(ChannelPtr Channel);
};

class cTransponderList;

class cEITScanner {
private:
  enum
  {
    ScanTimeout = 20
   };

  PLATFORM::CTimeout m_nextScan;
  cScanList*         m_scanList;
  cTransponderList*  m_transponderList;

  void CreateScanList(void);
  bool ScanDevice(DevicePtr device);
public:
  cEITScanner(void);
  ~cEITScanner();
  void AddTransponder(ChannelPtr Channel);
  void ForceScan(void);
  void Process(void);
  };

extern cEITScanner EITScanner;

#endif //__EITSCAN_H
