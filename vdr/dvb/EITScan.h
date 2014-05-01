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

#include "Config.h"
#include "devices/Device.h"
#include "devices/DeviceTypes.h"
#include "channels/ChannelTypes.h"
#include "platform/threads/threads.h"

#include <time.h>

namespace VDR
{

class cScanData
{
public:
  cScanData(const ChannelPtr& channel) : m_channel(channel), m_bScanned(false) { }
  virtual ~cScanData(void) { }

  ChannelPtr GetChannel(void) const { return m_channel; }

  /*
  bool       IsScanned(void)  const { return m_bScanned; }

  void       SetScanned(bool bScanned = true)  { m_bScanned = bScanned; }
  */

private:
  ChannelPtr m_channel;
  bool       m_bScanned;
};

class cScanList
{
public:
  void AddTransponder(const ChannelPtr& channel);

  bool IsEmpty(void) const { return m_list.empty(); }
  void Clear(void) { m_list.clear(); }

public: // TODO
  std::vector<cScanData> m_list;
};

class cEITScanner : public PLATFORM::CThread
{
public:
  cEITScanner(void);
  virtual ~cEITScanner(void) { }

  static cEITScanner& Get();

  void ForceScan(void);

protected:
  virtual void* Process(void);

private:
  /*!
   * If m_scanList is empty, this will populate it with channels from
   * m_transponderList and channels being tracked by cChannelManager.
   */
  void CreateScanList(void);

  /*!
   * Scan device using the provided scan data. If the scan is successful, this
   * calls cScanData::SetScanned() and returns true.
   */
  bool Scan(const DevicePtr& device, cScanData& scanData);

  void SaveEPGData(void);

  /**
   * Scan the next unscanned member of m_scanList. Attempts to use all devices
   * known to cDeviceManager. Returns false if there are no more channels to
   * scan, or all devices fail to tune to the channel.
   */
  bool ScanTransponder(cScanData& scanData);

  DevicePtr          m_device;
  PLATFORM::CTimeout m_nextFullScan;
  cScanList          m_scanList;
  ChannelVector      m_transponderList;
  bool               m_bScanFinished;
};

extern cEITScanner EITScanner;
}

#endif //__EITSCAN_H
