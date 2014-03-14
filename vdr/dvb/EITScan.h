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

#include "Types.h"
#include <time.h>
#include "Config.h"
#include "devices/Device.h"
#include "channels/ChannelManager.h"

class cTransponderList;

class cScanData
{
private:
  ChannelPtr m_channel;
  bool       m_bScanned;
public:
  cScanData(ChannelPtr channel);
  virtual ~cScanData(void);

  int Source(void) const;
  int Transponder(void) const;
  ChannelPtr GetChannel(void) const { return m_channel; }
  void SetScanned(void) { m_bScanned = true; }
  bool Scanned(void) const { return m_bScanned; }
};

class cScanList
{
public:
  void AddTransponders(const cChannelManager& Channels);
  void AddTransponders(const cTransponderList& Channels);
  void AddTransponder(ChannelPtr Channel);

  cScanData* Next(void);
  size_t UnscannedTransponders(void) const;
  size_t TotalTransponders(void) const;
private:
  std::vector<cScanData> m_list;
};

class cTransponderList;

class cEITScanner {
public:
  cEITScanner(void);
  virtual ~cEITScanner();
  void AddTransponder(ChannelPtr Channel);
  void ForceScan(void);
  void Process(void);

private:
  void CreateScanList(void);
  bool ScanDevice(DevicePtr device);
  void SaveEPGData(void);
  bool SwitchNextTransponder(void);

  PLATFORM::CTimeout m_nextTransponderScan;
  PLATFORM::CTimeout m_nextFullScan;
  cScanList*         m_scanList;
  cTransponderList*  m_transponderList;
  bool               m_bScanFinished;
};

extern cEITScanner EITScanner;

#endif //__EITSCAN_H
