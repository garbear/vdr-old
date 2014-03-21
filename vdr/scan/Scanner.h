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

#include "Types.h"
#include "ScanConfig.h"

#include <linux/dvb/frontend.h>
#include <platform/threads/threads.h>

namespace VDR
{

class cDevice;
class cScanLimits;
class cSynchronousAbort;

class cScanner : public PLATFORM::CThread
{
public:
  cScanner(cDevice* device, const cScanConfig& setup);
  virtual ~cScanner() { Stop(true); }

  void Start(void) { CreateThread(false); }
  void Stop(bool bWait = false);

protected:
  virtual void* Process(void);

private:
  static bool GetFrontendType(eDvbType dvbType, fe_type& frontendType);

  cDevice*           m_device;
  cScanConfig        m_setup;
  PLATFORM::CMutex   m_mutex;
  cSynchronousAbort* m_abortableJob;
};

}
