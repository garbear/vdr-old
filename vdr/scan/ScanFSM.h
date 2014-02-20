/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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
#pragma once

#include "ScanConfig.h" // for eScanFlags
#include "SWReceiver.h"
#include "scanners/EITScanner.h"
#include "scanners/NITScanner.h"
#include "scanners/PATScanner.h"
#include "scanners/PMTScanner.h"
#include "scanners/SDTScanner.h"
#include "channels/Channel.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceReceiverSubsystem.h"
#include "devices/subsystems/DeviceSectionFilterSubsystem.h"
#include "utils/FSM.h"
#include "utils/SynchronousAbort.h"

#include <libsi/si_ext.h>

#define LOCK_TIMEOUT_MS       4000
#define NIT_SCAN_TIMEOUT_MS  12000
#define PAT_SCAN_TIMEOUT_MS   4000 // (increased from 1000ms in wirbelscan)
#define SDT_SCAN_TIMEOUT_MS   4000
#define EIT_SCAN_TIMEOUT_MS  10000

namespace SCAN_FSM
{
enum eScanStatus
{
  eStart,           // init
  eTune,            // tune, check for lock
  eNextTransponder, // next unsearched transponder from NewTransponders
  eDetachReceiver,  // detach all receivers
  eScanNit,         // nit scan
  eScanPat,         // pat/pmt scan
  eScanSdt,         // sdt scan
  eScanEit,         // eit scan
  eAddChannels,     // adding results
  eStop,            // cleanup and leave loop
};
}

class cScanFsm : protected iNitScannerCallback, protected iPatScannerCallback, protected iSdtScannerCallback
{
public:
  typedef STATELIST10(SCAN_FSM::eStart,
                      SCAN_FSM::eTune,
                      SCAN_FSM::eNextTransponder,
                      SCAN_FSM::eDetachReceiver,
                      SCAN_FSM::eScanNit,
                      SCAN_FSM::eScanPat,
                      SCAN_FSM::eScanSdt,
                      SCAN_FSM::eAddChannels,
                      SCAN_FSM::eScanEit,
                      SCAN_FSM::eStop) StateType;

  cScanFsm(cDevice* device, cChannelManager* channelManager, const ChannelPtr& channel, cSynchronousAbort* abortableJob = NULL);
  virtual ~cScanFsm() { }

  // Default template versions
  template <int iStatus> int  Event() { return SCAN_FSM::eStop; }
  template <int> void Enter() { }
  template <int> void Exit()  { }

  // Scanner callbacks
  virtual void NitFoundTransponder(ChannelPtr transponder, bool bOtherTable, int originalNid, int originalTid);
  virtual void PatFoundChannel(ChannelPtr channel, int pid);
  virtual ChannelPtr SdtFoundService(ChannelPtr channel, int nid, int tid, int sid);
          ChannelPtr SdtGetByService(int source, int tid, int sid);
  virtual void SdtFoundChannel(ChannelPtr channel, int nid, int tid, int sid, char* name, char* shortName, char* provider);

private:
  static void ReportState(const char* state) { dsyslog("ScanFSM entered state: %s", state); }

  bool IsKnownInitialTransponder(const cChannel& newChannel, bool bAutoAllowed, bool bIncludeUnscannedTransponders = true) const;
  static bool IsNearlySameFrequency(const cChannel& channel1, const cChannel& channel2, unsigned int maxDeltaHz = 2001);
  static bool IsDifferentTransponderDeepScan(const cChannel& channel1, const cChannel& channel2, bool autoAllowed);
  ChannelPtr GetScannedTransponderByParams(const cChannel& newTransponder);
  static ChannelPtr GetByTransponder(ChannelVector& channels, const cChannel& transponder);

  cDevice*                  m_device;
  cChannelManager*          m_channelManager;
  ChannelPtr                m_currentTransponder; // Transponder currently being scanned
  cSynchronousAbort*        m_abortableJob;

  cSwReceiver               m_swReceiver;
  std::vector<cPmtScanner*> m_pmtScanners;

  ChannelVector             m_transponders;
  ChannelVector             m_scannedTransponders;
  ChannelVector             m_newChannels;
  PLATFORM::CMutex          m_mutex;
};
