/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#include "devices/DeviceSubsystem.h"
#include "dvb/filters/Filter.h"
#include "lib/platform/threads/mutex.h"
#include "lib/platform/threads/threads.h"
#include "transponders/Transponder.h"
#include "utils/Observer.h"

namespace VDR
{
class cEventScanner;
class cPat;
class cSdt;
class cPsipMgt;
class cPsipEit;
class cPsipStt;
class cPsipVct;

class cDeviceScanSubsystem : protected cDeviceSubsystem,
                             public    Observer,
                             public    iFilterCallback
{
public:
  cDeviceScanSubsystem(cDevice* device);
  virtual ~cDeviceScanSubsystem(void);

  void StartScan(void);
  void StopScan(void);

  bool WaitForTransponderScan(void);
  bool WaitForEPGScan(void);
  bool AttachReceivers(void);
  void DetachReceivers(void);

  virtual void Notify(const Observable &obs, const ObservableMessage msg);

  virtual void OnChannelPropsScanned(const ChannelPtr& channel);
  virtual void OnChannelNamesScanned(const ChannelPtr& channel);
  virtual void OnEventScanned(const EventPtr& event);

  cPat* PAT(void) const { return m_pat; }
  cSdt* SDT(void) const { return m_sdt; }
  cPsipMgt* MGT(void) const { return m_mgt; }
  cPsipStt* STT(void) const { return m_stt; }
  cPsipVct* VCT(void) const { return m_vct; }
  cPsipEit* PSIPEIT(void) const { return m_psipeit; }
private:
  cPat*                 m_pat;
  cSdt*                 m_sdt;
  cPsipMgt*             m_mgt;
  cPsipEit*             m_psipeit;
  cPsipStt*             m_stt;
  cPsipVct*             m_vct;
  bool                  m_receiversAttached;
};

}
